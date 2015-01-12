#ifndef MANIFEST_H
#define MANIFEST_H
#undef inline // because we are compiling with -std=c99
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#ifdef __ANDROID__
# include <sys/atomics.h>
#endif
#ifdef __MACH__
# include <mach/mach.h>
# include <mach/task.h> /* defines PAGE_SIZE to 4096 */
#endif
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic error "-Wunused-function"
#pragma GCC diagnostic error "-Wuninitialized"
#if !defined(DEBUG) || defined(NDEBUG)  /* because some vars are used only in trace() or assert() */
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#endif /* __GNUC__ */

#if __STDC_VERSION__ >= 199901L /* C99 or later code */
#undef inline // because we are compiling with -std=c99 "inline" is compiler keyword
#define __C99__
#endif

#define LOG_TAG "code.playground"
#define null NULL
#define countof(a) (sizeof(a) / sizeof((a)[0]))
typedef unsigned char byte;

int numberOfBits32(unsigned int bitset);
int numberOfBits64(unsigned long long bitset);
int numberOfLeadingZeros(unsigned long long  x);

// when/if android ever implements pthread_mutex_timedlock remove this:
int pthread_mutex_timedlock(pthread_mutex_t *mutex, struct timespec *timeout);

/* convert 0xE to -> "1110" with or without leading zeros */
const char* i2b(uint64_t v, char* buf /* must be at least 66 bytes in size */, bool leadingZeros);

enum { NANOSECONDS_IN_MILLISECOND = 1000000, NANOSECONDS_IN_SECOND = 1000000000 };


#ifdef __cplusplus
extern "C" {
#endif


#include "trace.h"
#include "backtrace.h"
#include "timestamp.h"
#include "mem.h"
#include "mapsl.h"
#include "mapll.h"

#ifdef __MACH__
static inline int gettid() { unsigned long long tid; pthread_threadid_np(NULL, &tid); return (pid_t)tid; }
#endif

unsigned long long get_cpu_mask();
int setaffinity(pid_t pid, unsigned long long  cpu_mask);
int getaffinity(pid_t pid, unsigned long long* cpu_mask);
void setaffinity_and_inc(volatile int* core_id);

#ifdef __cplusplus
}
#endif

#endif /* MANIFEST_H */
