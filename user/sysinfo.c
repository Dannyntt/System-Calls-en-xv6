// sysinfo.c: programa de usuario para probar la syscall sysinfo().
//
// Uso:
//   sysinfo
//
// Imprime memoria libre, paginas usadas/disponibles y procesos RUNNABLE
// reportados por el kernel.

#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct sysinfo info;

  if (sysinfo(&info) < 0) {
    fprintf(2, "sysinfo: la syscall fallo\n");
    exit(1);
  }

  printf("Free Memory: %d MB\n", (int)(info.freemem / (1024 * 1024)));
  printf("Used Pages: %d\n", (int)info.usedpages);
  printf("Available Pages: %d\n", (int)info.freepages);
  printf("Runnable Processes: %d\n", info.nrunnable);

  exit(0);
}
