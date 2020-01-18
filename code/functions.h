#ifndef __FUNCTIONS__
#define __FUNCTIONS__

#include "structs.h"

#define MALLOC_OR_DIE(pointer, size, fd) ({   \
            pointer = malloc(size);           \
            if (pointer == NULL)              \
            {                                 \
              perror("malloc() error");       \
              close(fd);                      \
              return -1;                      \
            }                                 \
            memset(pointer, 0, size);         \
          })


#define WRITE_OR_DIE(fd, pointer, size) ({                \
            ssize_t retval = write(fd, pointer, size);    \
            if (retval != size)                           \
            {                                             \
              perror("write() error");                    \
              free(pointer);                              \
              close(fd);                                  \
              return -1;                                  \
            }                                             \
          })


int cfs_create(char* cfs_filename, size_t bs, size_t fns, size_t cfs, uint mdfn);
int cfs_touch(const char buffer[], int fd);
int cfs_read(char* cfs_filename, int fd);

#endif
