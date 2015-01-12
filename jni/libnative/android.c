#include "android.h"
#include <cpu-features.h>
#include <sys/syscall.h>
#include <unistd.h>

int numberOfBits32(unsigned int bitset) { // http://en.wikipedia.org/wiki/Hamming_weight
    bitset = bitset - ((bitset >> 1) & 0x55555555U);
    bitset = (bitset & 0x33333333U) + ((bitset >> 2) & 0x33333333U);
    return (((bitset + (bitset >> 4)) & 0x0F0F0F0FU) * 0x01010101U) >> 24;
}

int numberOfBits64(unsigned long long bitset) {
    unsigned int lo = bitset & 0xFFFFFFFF;
    unsigned int hi = bitset >> 32;
    return numberOfBits32(lo) + numberOfBits32(hi);
}

const char* i2b(unsigned long long v, char* buf /* must be at least 66 */, bool leadingZeros) {
    int n = sizeof(unsigned long long) * 8;
    if (!leadingZeros) {
        n = n - numberOfLeadingZeros(v) + 1;
    }
    for (int i = 0; i < n; i++) {
        buf[n - 1 - i] = (1ULL << i) & v ? '0' : '1';
    }
    buf[n] = 0;
    return buf;
}

int numberOfLeadingZeros(unsigned long long x) {
    // if not defined use: http://www.hackersdelight.org/hdcodetxt/nlz.c.txt
    return x = 0 ? sizeof(x) * 8 : __builtin_clzll(x);
}

int setaffinity(pid_t pid, unsigned long long cpuset) {
    return syscall(__NR_sched_setaffinity, pid, sizeof(unsigned long long), &cpuset);
}

int getaffinity(pid_t pid, unsigned long long* cpuset) {
    int r = syscall(__NR_sched_getaffinity, pid, sizeof(unsigned long long), cpuset);
    return r > 0 && errno == 0 ? 0 : r; // seems to be a bug in __NR_sched_getaffinity returning 4 and errno == 0
}

unsigned long long get_cpu_mask() {
    return (1ULL << android_getCpuCount()) - 1;
}

void setaffinity_and_inc(volatile int* core_id) {
    unsigned long long mask = get_cpu_mask();
    int n = numberOfBits64(mask);
    if (n > 1) {
        int next = __sync_fetch_and_add(core_id, 1) % n;
        mask = (1ULL << next);
        int r = setaffinity(gettid(), mask);
        posix_info(r);
        if (r == 0) {
            unsigned long long now = 0;
            r = getaffinity(gettid(), &now);
            posix_info(r);
            if (r == 0 && now != mask) {
                char buf[128];
                trace("thread %d affinity: %s (MSBF)", gettid(), i2b(mask, buf, false));
            }
        }
    }
}

// once Android folks implement it, remove this crude version:

int pthread_mutex_timedlock(pthread_mutex_t *mutex, struct timespec *timeout) {
    int64_t to = NANOSECONDS_IN_SECOND * (int64_t)timeout->tv_sec + timeout->tv_nsec;
    for (;;) {
        int r = pthread_mutex_trylock(mutex);
        if (r != EBUSY) {
            return r;
        }
        if (walltime() > to) {
            return ETIMEDOUT;
        }
        nsleep(NANOSECONDS_IN_MILLISECOND); /* 1ms */
    }
}
