#include <config.h>
#include "io.h"
#include "utils.h"
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 64

struct IO_data {
  int fd;
  int timeout;
  char input[BUFFER_SIZE];
  size_t available;
  size_t inptr;
  char output[BUFFER_SIZE];
  size_t written;
};

IO *io_create(int fd) {
  IO *io = xmalloc(sizeof *io);
  io->fd = fd;
  io->timeout = 0;
  io->available = 0;
  io->inptr = 0;
  io->written = 0;
  return io;
}

int io_set_timeout(IO *io, int timeout) {
  int flags = fcntl(io->fd, F_GETFL);
  io->timeout = timeout;
  if(timeout) {
    if(!(flags & O_NONBLOCK))
      if(fcntl(io->fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
        return errno;
  } else {
    if((flags & O_NONBLOCK))
      if(fcntl(io->fd, F_SETFL, flags ^ O_NONBLOCK) < 0)
        return errno;
  }
  return 0;
}

int io_close(IO *io) {
  int errno_value = io_flush(io);
  close(io->fd);
  free(io);
  return errno_value;
}

static int io__wait(IO *io, time_t limit, int events) {
  time_t now;
  struct pollfd fds;
  int rc;
  for(;;) {
    time(&now);
    if(io->timeout && now >= limit)
      return ETIMEDOUT;
    fds.fd = io->fd;
    fds.events = events;
    rc = poll(&fds, 1, io->timeout ? 1000 * (limit - now) : -1);
    if(rc < 0) {
      if(errno != EINTR)
        return errno;
    } else if(fds.revents & events)
      return 0;
  }
}

static int io__flush(IO *io, time_t limit) {
  int errno_value = 0;
  size_t written_so_far = 0;
  ssize_t write_count;
  while(written_so_far < io->written) {
    if((errno_value = io__wait(io, limit, POLLOUT)))
      break;
    if((write_count = write(io->fd,
                            io->output + written_so_far,
                            io->written - written_so_far)) < 0) {
      if(errno != EINTR && errno != EAGAIN) {
        errno_value = errno;
        break;
      }
    } else
      written_so_far += write_count;
  }
  memmove(io->output, io->output + written_so_far, io->written - written_so_far);
  io->written -= written_so_far;
  return errno_value;
}

int io_flush(IO *io) {
  return io__flush(io, time(NULL) + io->timeout);
}

int io_write(IO *io, const void *ptr, size_t len, size_t *wrote) {
  int errno_value = 0;
  size_t written_so_far = 0, write_this_time;
  time_t limit = time(NULL) + io->timeout;
  while(written_so_far < len) {
    if(io->written == sizeof(io->output)) {
      if((errno_value = io__flush(io, limit))) {
        *wrote = written_so_far;
        break;
      }
    }
    write_this_time = len - written_so_far;
    if(write_this_time > (sizeof io->output) - io->written)
      write_this_time = (sizeof io->output) - io->written;
    memcpy(io->output + io->written, (char *)ptr + written_so_far,
           write_this_time);
    io->written += write_this_time;
    written_so_far += write_this_time;
  }
  if(wrote)
    *wrote = written_so_far;
  return errno_value;
}

int io_vprintf(IO *io, const char *format, va_list ap) {
  char *buffer;
  int rc;

  rc = vasprintf(&buffer, format, ap);
  if(rc < 0)
    return errno;
  rc = io_write(io, buffer, rc, NULL);
  free(buffer);
  return rc;
}

int io_printf(IO *io, const char *format, ...) {
  va_list ap;
  int errno_value;

  va_start(ap, format);
  errno_value = io_vprintf(io, format, ap);
  va_end(ap);
  return errno_value;
}

static int io__fill(IO *io, time_t limit) {
  int errno_value;
  int bytes_read;

  io->inptr = 0;
  io->available = 0;
  for(;;) {
    if((errno_value = io__wait(io, limit, POLLIN)))
      return errno_value;
    bytes_read = read(io->fd, io->input, sizeof io->input);
    if(bytes_read < 0) {
      if(errno != EINTR && errno != EAGAIN)
        return errno;
    } else if(bytes_read == 0)
      return -1;
    else {
      io->available = bytes_read;
      return 0;
    }
  }
}

static int io__readbyte(IO *io, time_t limit, char *cp) {
  int errno_value;
  if(io->inptr == io->available)
    if((errno_value = io__fill(io, limit)))
      return errno_value;
  *cp = io->input[io->inptr++];
  return 0;
}

int io_getline(IO *io, char **line, size_t *linesize) {
  time_t limit = time(NULL) + io->timeout;
  char *buffer = *line;
  size_t used = 0;
  char c;
  int errno_value = 0;

  while(!(errno_value = io__readbyte(io, limit, &c))) {
    if(used >= *linesize) {
      *linesize = *linesize ? 2 * *linesize : 64;
      buffer = xrealloc(buffer, *linesize);
    }
    buffer[used++] = c;
    if(c == '\n')
      break;
  }
  if(errno_value == 0
     || (errno_value < 0 && used > 0)) {
    if(used >= *linesize) {
      *linesize = *linesize ? 2 * *linesize : 64;
      buffer = xrealloc(buffer, *linesize);
    }
    buffer[used] = 0;
    errno_value = 0;              /* suppress EOF if a partial line was read */
  } else {
    free(buffer);
    buffer = NULL;
  }
  *line = buffer;
  return errno_value;
}

int io_fileno(IO *io) {
  return io->fd;
}
