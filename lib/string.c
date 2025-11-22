/**
 * String and Memory Utility Functions Implementation
 */

#include <kernel/string.h>
#include <stdint.h>
#include <stddef.h>

/**
 * Set memory to a specific value
 */
void *memset(void *dest, int val, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    uint8_t v = (uint8_t)val;

    while (n--) {
        *d++ = v;
    }

    return dest;
}

/**
 * Copy memory from source to destination
 */
void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    while (n--) {
        *d++ = *s++;
    }

    return dest;
}

/**
 * Move memory (handles overlapping regions)
 */
void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d < s) {
        // Copy forward
        while (n--) {
            *d++ = *s++;
        }
    } else {
        // Copy backward
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }

    return dest;
}

/**
 * Compare two memory regions
 */
int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }

    return 0;
}

/**
 * Get string length
 */
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/**
 * Copy string
 */
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

/**
 * Copy string with length limit
 */
char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

/**
 * Compare two strings
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const uint8_t *)s1 - *(const uint8_t *)s2;
}

/**
 * Compare two strings with length limit
 */
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(const uint8_t *)s1 - *(const uint8_t *)s2;
}

/**
 * Concatenate strings
 */
char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) {
        d++;
    }
    while ((*d++ = *src++));
    return dest;
}

/**
 * Find character in string
 */
char *strchr(const char *str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

/**
 * Find last occurrence of character in string
 */
char *strrchr(const char *str, int c) {
    const char *last = NULL;
    while (*str) {
        if (*str == (char)c) {
            last = str;
        }
        str++;
    }
    return (char *)last;
}

/**
 * Simple snprintf implementation
 * Supports: %s, %d, %u, %x, %X, %c, %%
 */
int snprintf(char *str, size_t size, const char *format, ...) {
    if (!str || size == 0) {
        return 0;
    }

    __builtin_va_list args;
    __builtin_va_start(args, format);

    size_t written = 0;
    const char *p = format;

    while (*p && written < size - 1) {
        if (*p == '%') {
            p++;
            if (*p == '%') {
                str[written++] = '%';
            } else if (*p == 's') {
                const char *s = __builtin_va_arg(args, const char *);
                if (!s) s = "(null)";
                while (*s && written < size - 1) {
                    str[written++] = *s++;
                }
            } else if (*p == 'c') {
                char c = (char)__builtin_va_arg(args, int);
                str[written++] = c;
            } else if (*p == 'd' || *p == 'i') {
                int num = __builtin_va_arg(args, int);
                char buf[32];
                int i = 0;
                int is_negative = (num < 0);
                if (is_negative) num = -num;

                do {
                    buf[i++] = '0' + (num % 10);
                    num /= 10;
                } while (num > 0);

                if (is_negative && written < size - 1) {
                    str[written++] = '-';
                }

                while (i > 0 && written < size - 1) {
                    str[written++] = buf[--i];
                }
            } else if (*p == 'u') {
                unsigned int num = __builtin_va_arg(args, unsigned int);
                char buf[32];
                int i = 0;

                do {
                    buf[i++] = '0' + (num % 10);
                    num /= 10;
                } while (num > 0);

                while (i > 0 && written < size - 1) {
                    str[written++] = buf[--i];
                }
            } else if (*p == 'x' || *p == 'X') {
                unsigned int num = __builtin_va_arg(args, unsigned int);
                char buf[32];
                int i = 0;
                const char *hex_digits = (*p == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";

                do {
                    buf[i++] = hex_digits[num % 16];
                    num /= 16;
                } while (num > 0);

                while (i > 0 && written < size - 1) {
                    str[written++] = buf[--i];
                }
            }
            p++;
        } else {
            str[written++] = *p++;
        }
    }

    str[written] = '\0';
    __builtin_va_end(args);
    return written;
}
