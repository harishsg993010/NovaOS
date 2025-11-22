/**
 * User-Space System Call Library
 *
 * Provides C wrappers for system calls.
 */

#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

// System call numbers (must match kernel)
#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_READ        2
#define SYS_OPEN        3
#define SYS_CLOSE       4
#define SYS_GETPID      5
#define SYS_SLEEP       6
#define SYS_YIELD       7
#define SYS_FORK        8
#define SYS_EXEC        9
#define SYS_WAIT        10
#define SYS_MALLOC      11
#define SYS_FREE        12
#define SYS_TIME        13
#define SYS_GETCHAR     14
#define SYS_PUTCHAR     15

// Generic syscall function
static inline int64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2,
                               uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    int64_t ret;
    register uint64_t r10 __asm__("r10") = arg4;
    register uint64_t r8 __asm__("r8") = arg5;

    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );

    return ret;
}

// System call wrappers

static inline void exit(int code) {
    syscall(SYS_EXIT, code, 0, 0, 0, 0);
    while (1);  // Never returns
}

static inline int write(int fd, const char *buf, size_t count) {
    return (int)syscall(SYS_WRITE, fd, (uint64_t)buf, count, 0, 0);
}

static inline int read(int fd, char *buf, size_t count) {
    return (int)syscall(SYS_READ, fd, (uint64_t)buf, count, 0, 0);
}

static inline int getpid(void) {
    return (int)syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}

static inline void sleep_ms(uint64_t ms) {
    syscall(SYS_SLEEP, ms, 0, 0, 0, 0);
}

static inline void yield(void) {
    syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

static inline uint64_t get_time(void) {
    return (uint64_t)syscall(SYS_TIME, 0, 0, 0, 0, 0);
}

static inline void putchar(char c) {
    syscall(SYS_PUTCHAR, c, 0, 0, 0, 0);
}

static inline int getchar(void) {
    return (int)syscall(SYS_GETCHAR, 0, 0, 0, 0, 0);
}

// Helper functions

static inline void puts(const char *str) {
    while (*str) {
        putchar(*str++);
    }
}

static inline void print_num(int num) {
    if (num < 0) {
        putchar('-');
        num = -num;
    }

    if (num == 0) {
        putchar('0');
        return;
    }

    char buf[20];
    int i = 0;

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i > 0) {
        putchar(buf[--i]);
    }
}

#endif // USER_SYSCALL_H
