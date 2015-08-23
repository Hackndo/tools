#ifndef MISC_H__
#define MISC_H__


#include <stdarg.h>

#define __printf_fmt(fmt,args) __attribute__((format (printf,fmt,args)))
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(*a))

/*
 * Calls perror(msg) and abort()
 */
void system_error(const char *msg);


/*
 * Print the message on stderr
 */
__printf_fmt(1,2) void custom_warn(const char *format, ...);


/*
 * Print the message on stderr and abort()
 */
__printf_fmt(1,2) void custom_error(const char *format, ...);


/*
 * sprintf that allocate the buffer to write in
 * The buffer must be freed with free()
 */
__printf_fmt(2,3) int asprintf(char **str, const char *format, ...);

/*
 * vsprintf that allocate the buffer to write in
 * The buffer must be freed with free()
 */
int avsprintf(char **str, const char *format, va_list ap);

#endif
