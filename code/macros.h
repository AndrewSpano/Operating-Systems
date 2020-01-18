/* malloc() macro used to avoid hard code checks */
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


/* same as above, just returns NULL instead of -1 on failure */
#define MALLOC_OR_DIE_2(pointer, size)         \
        ({                                     \
          pointer = malloc(size);              \
          if (pointer == NULL)                 \
          {                                    \
            perror("malloc() error");          \
            return NULL;                       \
          }                                    \
          memset(pointer, 0, size);            \
        })


/* write() macro used to avoid hard code checks */
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


/* same as above, just returns 0 instead of -1 on failure and does not free(), close() */
#define WRITE_OR_DIE_2(fd, pointer, size)                 \
        ({                                                \
          ssize_t retval = write(fd, pointer, size);      \
          if (retval != size)                             \
          {                                               \
            perror("Unexpected behaviour of write()");    \
            return 0;                                     \
          }                                               \
        })




/* lseek() macro used to avoid hard code checks */
#define LSEEK_OR_DIE(fd, size, start_position)                       \
        ({                                                           \
          off_t new_position = lseek(fd, size, start_position);      \
          if (new_position == (off_t) -1)                            \
          {                                                          \
            perror("lseek() error");                                 \
            return 0;                                                \
          }                                                          \
        })


/* same as above, just returns NULL instead of -1 on failure and frees a pointer */
#define LSEEK_OR_DIE_2(fd, size, start_position, pointer)            \
        ({                                                           \
          off_t new_position = lseek(fd, size, start_position);      \
          if (new_position == (off_t) -1)                            \
          {                                                          \
            perror("lseek() error");                                 \
            free(pointer);                                           \
            return NULL;                                             \
          }                                                          \
        })


/* read() macro used to avoid hard code checks */
#define READ_OR_DIE(fd, pointer, size)                    \
        ({                                                \
          ssize_t retval = read(fd, pointer, size);       \
          if (retval != size)                             \
          {                                               \
            perror("read() error");                       \
            free(pointer);                                \
            return NULL;                                  \
          }                                               \
        })



/* macro used to check fast if a pointer is NULL and avoid hard coding */
#define DIE_IF_NULL(pointer)                           \
        ({                                             \
          if (pointer == NULL)                         \
          {                                            \
            printf("Error: NULL pointer given.\n");    \
            return 0;                                  \
          }                                            \
        })
