#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdarg.h>

typedef struct IO_data IO;

IO *io_create(int fd);
int io_set_timeout(IO *io, int seconds);
int io_close(IO *io);
int io_getline(IO *io, char **line, size_t *linesize);
int io_flush(IO *io);
int io_write(IO *io, const void *ptr, size_t len, size_t *wrote);
int io_printf(IO *io, const char *format, ...);
int io_vprintf(IO *io, const char *format, va_list ap);
int io_fileno(IO *io);

#endif /* IO_H */

