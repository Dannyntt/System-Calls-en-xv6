// trace.c: programa de usuario para probar la syscall trace().
//
// Uso:
//   trace <nombre_syscall> comando [args...]
//
// Ejemplo:
//   trace sys_kill kill 999          -> monitorea sys_kill mientras corre "kill 999"
//   trace sys_write echo hola        -> monitorea sys_write mientras corre "echo hola"
//
// El programa traduce el nombre de la syscall a su numero (usando las mismas
// constantes SYS_* que usa el kernel), activa el trace en si mismo con la
// syscall trace(), y luego hace exec() del comando indicado. Como el flag de
// trace vive en la estructura del proceso (struct proc) y no en su memoria,
// sobrevive al exec y tambien se hereda por los hijos que ese comando cree
// (fork copia p->traceid).

#include "kernel/types.h"
#include "kernel/syscall.h"
#include "user/user.h"

struct {
  char *name;
  int num;
} syscall_table[] = {
  {"sys_fork", SYS_fork},     {"sys_exit", SYS_exit},
  {"sys_wait", SYS_wait},     {"sys_pipe", SYS_pipe},
  {"sys_read", SYS_read},     {"sys_kill", SYS_kill},
  {"sys_exec", SYS_exec},     {"sys_fstat", SYS_fstat},
  {"sys_chdir", SYS_chdir},   {"sys_dup", SYS_dup},
  {"sys_getpid", SYS_getpid}, {"sys_sbrk", SYS_sbrk},
  {"sys_pause", SYS_pause},   {"sys_uptime", SYS_uptime},
  {"sys_open", SYS_open},     {"sys_write", SYS_write},
  {"sys_mknod", SYS_mknod},   {"sys_unlink", SYS_unlink},
  {"sys_link", SYS_link},     {"sys_mkdir", SYS_mkdir},
  {"sys_close", SYS_close},   {"sys_sync", SYS_sync},
  {"sys_trace", SYS_trace},   {"sys_sysinfo", SYS_sysinfo},
};

int
name2num(char *name)
{
  int i;
  for (i = 0; i < (int)(sizeof(syscall_table) / sizeof(syscall_table[0]));
       i++) {
    if (strcmp(syscall_table[i].name, name) == 0)
      return syscall_table[i].num;
  }
  return -1;
}

int
main(int argc, char *argv[])
{
  int num;

  if (argc < 3) {
    fprintf(2, "uso: trace <nombre_syscall> comando [args...]\n");
    exit(1);
  }

  num = name2num(argv[1]);
  if (num < 0) {
    fprintf(2, "trace: syscall desconocida '%s'\n", argv[1]);
    exit(1);
  }

  if (trace(num) < 0) {
    fprintf(2, "trace: fallo al activar el trace\n");
    exit(1);
  }

  exec(argv[2], &argv[2]);
  // solo llegamos aqui si exec falla
  fprintf(2, "trace: exec %s fallo\n", argv[2]);
  exit(1);
}
