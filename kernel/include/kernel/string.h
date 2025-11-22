/**
 * String and Memory Utility Functions
 *
 * Provides standard string and memory manipulation functions
 * for use in the kernel.
 */

#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include <stdint.h>
#include <stddef.h>

/**
 * Set memory to a specific value
 *
 * @param dest Destination pointer
 * @param val Value to set
 * @param n Number of bytes to set
 * @return Destination pointer
 */
void *memset(void *dest, int val, size_t n);

/**
 * Copy memory from source to destination
 *
 * @param dest Destination pointer
 * @param src Source pointer
 * @param n Number of bytes to copy
 * @return Destination pointer
 */
void *memcpy(void *dest, const void *src, size_t n);

/**
 * Move memory (handles overlapping regions)
 *
 * @param dest Destination pointer
 * @param src Source pointer
 * @param n Number of bytes to move
 * @return Destination pointer
 */
void *memmove(void *dest, const void *src, size_t n);

/**
 * Compare two memory regions
 *
 * @param s1 First memory region
 * @param s2 Second memory region
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int memcmp(const void *s1, const void *s2, size_t n);

/**
 * Get string length
 *
 * @param str String to measure
 * @return Length of string (not including null terminator)
 */
size_t strlen(const char *str);

/**
 * Copy string
 *
 * @param dest Destination buffer
 * @param src Source string
 * @return Destination pointer
 */
char *strcpy(char *dest, const char *src);

/**
 * Copy string with length limit
 *
 * @param dest Destination buffer
 * @param src Source string
 * @param n Maximum number of characters to copy
 * @return Destination pointer
 */
char *strncpy(char *dest, const char *src, size_t n);

/**
 * Compare two strings
 *
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strcmp(const char *s1, const char *s2);

/**
 * Compare two strings with length limit
 *
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum number of characters to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strncmp(const char *s1, const char *s2, size_t n);

/**
 * Concatenate strings
 *
 * @param dest Destination buffer
 * @param src Source string
 * @return Destination pointer
 */
char *strcat(char *dest, const char *src);

/**
 * Find character in string
 *
 * @param str String to search
 * @param c Character to find
 * @return Pointer to first occurrence, or NULL if not found
 */
char *strchr(const char *str, int c);

/**
 * Find last occurrence of character in string
 *
 * @param str String to search
 * @param c Character to find
 * @return Pointer to last occurrence, or NULL if not found
 */
char *strrchr(const char *str, int c);

/**
 * Format string with size limit (simplified version)
 *
 * @param str Destination buffer
 * @param size Maximum size of buffer
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of characters written (excluding null terminator)
 */
int snprintf(char *str, size_t size, const char *format, ...);

#endif // KERNEL_STRING_H
