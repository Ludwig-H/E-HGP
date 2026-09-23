#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ucontext.h>
#include <stdint.h>
#define MAXS (1u<<24)
static uintptr_t *pcs; static volatile unsigned long cnt;
static void handler(int sig, siginfo_t *si, void *uc_) {
  (void)sig; (void)si; ucontext_t *uc = (ucontext_t*)uc_;
  unsigned long i = __atomic_fetch_add(&cnt, 1, __ATOMIC_RELAXED);
  if (i < MAXS) pcs[i] = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
}
__attribute__((constructor)) static void start(void) {
  pcs = calloc(MAXS, sizeof(uintptr_t));
  struct sigaction sa; memset(&sa, 0, sizeof sa); sa.sa_sigaction = handler; sa.sa_flags = SA_SIGINFO | SA_RESTART;
  sigaction(SIGPROF, &sa, 0);
  struct itimerval it = {{0, 1000}, {0, 1000}}; setitimer(ITIMER_PROF, &it, 0);
}
__attribute__((destructor)) static void stop(void) {
  struct itimerval it = {{0,0},{0,0}}; setitimer(ITIMER_PROF, &it, 0);
  const char *out = getenv("SAMPLER_OUT"); if (!out) return;
  FILE *f = fopen(out, "w"); if (!f) return;
  FILE *m = fopen("/proc/self/maps", "r"); char line[4096];
  while (m && fgets(line, sizeof line, m)) fprintf(f, "M %s", line);
  if (m) fclose(m);
  unsigned long n = cnt < MAXS ? cnt : MAXS;
  for (unsigned long i = 0; i < n; ++i) fprintf(f, "P %lx\n", (unsigned long)pcs[i]);
  fclose(f);
}
