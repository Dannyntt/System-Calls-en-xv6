# Proyecto 2 — System Calls en xv6

## Información General

- **Curso:** Sistemas Operativos 2026-2 — Universidad EAFIT
- **Docente:** José Luis Montoya Pareja
- **Integrantes:**
  - Daniela Giraldo Salas — GitHub: @dannyntt
  - Matías Gil Montoya — GitHub: @matgimon18a
  - Gisel Jaramillo Carmona - Github: @gljaramilloc 

## Descripción de la Solución

### Resumen de la implementación

Se extendió el kernel de **xv6-riscv** con dos nuevas llamadas al sistema:

- **`trace(int syscallnum)`**: activa el monitoreo de una syscall específica
  para el proceso que la invoca. Cuando ese proceso (o cualquier hijo suyo,
  ya que el flag se hereda vía `fork`) ejecuta la syscall vigilada, el kernel
  imprime por consola el PID del proceso, el nombre de la syscall, su valor
  de retorno y un conjunto de registros relevantes del procesador RISC-V
  (`s0`, `s1`, `a0`, `a1`).
- **`sysinfo(struct sysinfo *info)`**: recopila y copia al espacio de usuario
  (mediante `copyout`) un snapshot del estado del sistema: memoria libre en
  bytes, número de páginas físicas usadas, número de páginas físicas libres
  y número de procesos en estado `RUNNABLE`.

Se incluyen además dos programas de usuario (`user/trace.c` y
`user/sysinfo.c`) que ejercitan ambas syscalls, pedidos explícitamente por
RF-03 y RF-04.

### Diseño de `trace`

En lugar de que `trace` reciba directamente el nombre de la syscall como
cadena en el kernel, el programa de usuario `trace` (`user/trace.c`) hace la
traducción nombre → número usando las mismas constantes `SYS_*` que usa el
kernel (`kernel/syscall.h`), y llama a la syscall `trace(int)` pasando el
número. Esto evita tener que copiar y comparar strings dentro del kernel en
cada syscall, que es una ruta caliente de ejecución.

El número de la syscall que se está monitoreando se guarda en un nuevo campo
`traceid` de `struct proc` (`kernel/proc.h`). Este campo:

- Se inicializa en `0` (sin trace) cuando se crea un proceso (`allocproc`,
  `kernel/proc.c`).
- Se hereda del padre al hijo en `kfork()` (`kernel/proc.c`), de modo que
  `trace sys_kill kill 5` pueda seguir monitoreando aunque internamente haya
  un `fork`/`exec`.
- **Sobrevive a `exec`**, porque vive en la estructura del proceso
  (`struct proc`) y no en su espacio de memoria, que es lo único que `exec`
  reemplaza. Por eso el patrón de uso es
  `trace <nombre_syscall> <comando> [args...]`: el propio programa `trace`
  activa el flag en sí mismo y luego hace `exec` del comando a observar.

La impresión ocurre dentro de `syscall()` (`kernel/syscall.c`), justo
después de invocar la función real de la syscall (`syscalls[num]()`), para
poder mostrar el valor de retorno ya calculado. Los registros de argumentos
(`s0`, `s1`, `a0`, `a1`) se guardan **antes** de invocar la función, porque
`a0` es sobreescrito con el valor de retorno.

### Diseño de `sysinfo`

- **Memoria libre / páginas libres**: se agregó `freepagecount()` en
  `kernel/kalloc.c`, que recorre la free list del asignador de páginas
  (`kmem.freelist`) bajo el lock correspondiente y cuenta cuántas páginas de
  4096 bytes hay disponibles. La memoria libre en bytes es
  `freepagecount() * PGSIZE`.
- **Páginas usadas**: se calcula como el total de páginas físicas
  administradas por el kernel (`(PHYSTOP - KERNBASE) / PGSIZE`) menos las
  páginas libres.
- **Procesos RUNNABLE**: se agregó `countrunnable()` en `kernel/proc.c`, que
  recorre la tabla de procesos (`proc[]`) tomando el lock de cada proceso y
  cuenta cuántos están en estado `RUNNABLE`.
- **Transferencia a usuario**: `sys_sysinfo()` (`kernel/sysproc.c`) arma una
  `struct sysinfo` (definida en el nuevo header `kernel/sysinfo.h`, incluido
  tanto por el kernel como por `user/sysinfo.c`) y la copia al puntero que
  recibe como argumento usando `copyout(p->pagetable, p->sz, dst, ...)`.

### Verificación de integración entre `trace` y `sysinfo`

Antes de la entrega se verificó explícitamente que las dos syscalls
nuevas conviven sin interferirse: se activó `trace` sobre distintas
syscalls (`kill`, `write`, `sysinfo`) y se confirmó en cada caso que:

- El reporte de `trace` incluye el PID correcto, el nombre de la syscall,
  el valor de retorno y los cuatro registros pedidos.
- `trace off` desactiva el rastreo de forma inmediata (verificado
  llamando de nuevo a la syscall vigilada y confirmando que ya no
  imprime nada).
- `sysinfo` reporta valores coherentes (memoria libre, páginas usadas/
  disponibles y procesos `RUNNABLE`) tanto con `trace` activo como
  inactivo, sin verse afectada por el mecanismo de rastreo.
- El resto del sistema (`ls`, `echo`, `cat`, el shell del Proyecto 1)
  sigue funcionando exactamente igual después de integrar ambas
  syscalls.

## Archivos Modificados

| Archivo | Descripción del cambio |
|---|---|
| `kernel/syscall.h` | Se agregan `SYS_trace` (23) y `SYS_sysinfo` (24). |
| `kernel/sysinfo.h` *(nuevo)* | Define `struct sysinfo`, compartida entre kernel y usuario. |
| `kernel/proc.h` | Se agrega el campo `traceid` a `struct proc`. |
| `kernel/proc.c` | Inicialización y reset de `traceid` en `allocproc`/`freeproc`, herencia en `kfork`, y nueva función `countrunnable()`. |
| `kernel/kalloc.c` | Nueva función `freepagecount()` que recorre la free list. |
| `kernel/defs.h` | Prototipos de `freepagecount()` y `countrunnable()`. |
| `kernel/sysproc.c` | Implementación de `sys_trace()` y `sys_sysinfo()`. |
| `kernel/syscall.c` | Registro de ambas syscalls en la tabla `syscalls[]`, tabla `syscallnames[]` para poder imprimir el nombre, y lógica de impresión en `syscall()` cuando la syscall ejecutada coincide con `p->traceid`. |
| `user/user.h` | Prototipos `int trace(int)` y `int sysinfo(struct sysinfo *)`. |
| `user/usys.pl` | Genera los stubs ensamblador (`ecall`) para `trace` y `sysinfo`. |
| `user/trace.c` *(nuevo)* | Programa de prueba: traduce nombre de syscall a número y ejecuta un comando bajo monitoreo (RF-03). |
| `user/sysinfo.c` *(nuevo)* | Programa de prueba: imprime la información devuelta por `sysinfo()` (RF-04). |
| `Makefile` | Se agregan `_trace` y `_sysinfo` a `UPROGS` para que se incluyan en `fs.img`. |

## Compilación

Requisitos: un toolchain cruzado para RISC-V (`riscv64-unknown-elf-*` o,
alternativamente, `riscv64-linux-gnu-*`) y `qemu-system-riscv64`.

En Ubuntu/Debian, si no se cuenta con el toolchain `elf` oficial, puede
usarse el toolchain `linux-gnu`, ya que también sirve para este propósito:

```bash
sudo apt-get install gcc-riscv64-linux-gnu qemu-system-riscv64
```

Para compilar el kernel y el sistema de archivos:

```bash
make TOOLPREFIX=riscv64-linux-gnu-
```

(Si se cuenta con `riscv64-unknown-elf-gcc`, el `Makefile` lo detecta
automáticamente y no es necesario pasar `TOOLPREFIX`.)

## Ejecución

```bash
make TOOLPREFIX=riscv64-linux-gnu- qemu
```

Una vez dentro del shell de xv6:

```
$ sysinfo
Free Memory: 127 MB
Used Pages: 236
Available Pages: 32532
Runnable Processes: 0

$ trace sys_write echo hola_mundo
hola_mundoPID: 4
SYSCALL: sys_write
RETURN: 10
s0: 0x3fb0
s1: 0x3fc8
a0: 0x1
a1: 0x3fe0

PID: 4
SYSCALL: sys_write
RETURN: 1
s0: 0x3fb0
s1: 0x3fc8
a0: 0x1
a1: 0x908
```

(Este es el resultado real obtenido al probar la solución en QEMU. `echo`
hace dos llamadas a `sys_write` — el contenido y el salto de línea — por eso
se ven dos bloques de trace.)

Para salir de QEMU: `Ctrl-a` seguido de `x`.

### Prueba adicional: `trace` y `sysinfo` en conjunto

Para confirmar que ambas syscalls funcionan correctamente cuando una
observa a la otra, se activó el rastreo sobre `sysinfo` mismo:

```
$ trace sysinfo
trace: rastreando 'sysinfo' (syscall num 24)
$ sysinfo
PID: 4
SYSCALL: sysinfo
RETURN: 0
s0: 0x3fd0
s1: 0x14f50
a0: 0x0
a1: 0x3fe0
Free Memory: 127 MB
Used Pages: 233
Available Pages: 32535
Runnable Processes: 0
```

**Nota sobre el orden de la salida:** el bloque de `trace`
(`PID/SYSCALL/RETURN/registros`) aparece impreso *antes* que el resultado
de `sysinfo` (`Free Memory/...`), aunque en el código `trace` imprime
*después* de que `sysinfo` ya calculó y devolvió todos sus datos. Esto se
debe a que el reporte de `trace` se imprime con `printk` (escritura
directa a la consola desde el kernel, sin buffer), mientras que
`user/sysinfo.c` usa `printf` de espacio de usuario, cuyo buffer se vacía
a la pantalla un instante después. No indica ningún error de lógica ni de
orden de ejecución real, solo una diferencia de timing entre la
impresión del kernel y la del proceso de usuario.

## Uso de IA

Se utilizó Claude (Anthropic) como herramienta de IA como apoyo
para: ayudar a la comprensión la estructura del kernel de xv6-riscv, optimizar el código de
kernel y de los programas de usuario, Todo el código entregado fue revisado y es comprendido por los integrantes
del equipo :D
