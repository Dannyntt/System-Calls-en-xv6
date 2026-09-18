// Estructura devuelta por la syscall sysinfo().
// Se comparte entre kernel y espacio de usuario, por eso vive en su propio
// header en lugar de en proc.h/user.h directamente.
struct sysinfo {
  uint64 freemem;   // memoria libre disponible, en bytes
  uint64 usedpages; // numero de paginas fisicas actualmente en uso
  uint64 freepages; // numero de paginas fisicas libres
  int nrunnable;    // numero de procesos en estado RUNNABLE
};
