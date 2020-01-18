#define MALLOC_OR_DIE(pointer, size, fd)      \
        ({                                    \
          pointer = malloc(size);             \
          if (pointer == NULL)                \
          {                                   \
            perror("malloc() error");         \
            close(fd);                        \
            return -1;                        \
          }                                   \
          memset(pointer, 0, size);           \
        })



#define WRITE_OR_DIE(fd, pointer, size)                  \
        ({                                               \
          ssize_t retval = write(fd, pointer, size);     \
          if (retval != size)                            \
          {                                              \
            perror("write() error");                     \
            free(pointer);                               \
            close(fd);                                   \
            return -1;                                   \
          }                                              \
        })



#define DIE_IF_NULL(pointer)                           \
        ({                                             \
          if (pointer == NULL)                         \
          {                                            \
            printf("Error: NULL pointer given.\n");    \
            return 0;                                  \
          }                                            \
        })



#define LSEEK_OR_DIE(fd, size, start_position)                       \
        ({                                                           \
          off_t new_position = lseek(fd, size, start_position);      \
          if (new_position == (off_t) -1)                            \
          {                                                          \
            perror("lseek() error");                                 \
            return 0;                                                \
          }                                                          \
        })




#define WRITE_OR_DIE_2(fd, pointer, size)                 \
        ({                                                \
          ssize_t retval = write(fd, pointer, size);      \
          if (retval != size)                             \
          {                                               \
            perror("Unexpected behaviour of write()");    \
            return 0;                                     \
          }                                               \
        })
