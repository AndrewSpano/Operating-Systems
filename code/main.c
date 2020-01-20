#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "functions.h"
#include "functions_util.h"
#include "functions2.h"
#include "util.h"
#include "list.h"



int main(int argc, char* argv[])
{
  /* variable used to read input from the user */
  char buffer[MAX_BUFFER_SIZE] = {0};

  /* option given that will be derived from the buffer */
  int option = 0;

  /* file descriptor that will be used to store the currently open cfs file */
  int fd = -1;

  /* name of the current cfs file being used */
  char cfs_filename[MAX_CFS_FILENAME_SIZE] = {0};

  /* pointers used to fast access the data of the cfs file */
  superblock* my_superblock = NULL;
  hole_map* holes = NULL;
  Stack_List* list = NULL;


  do {

    printf("> ");
    fgets(buffer, MAX_BUFFER_SIZE, stdin);
    option = get_option(buffer);

    switch (option)
    {
      case 1:
      {
        if (!get_nth_string(cfs_filename, buffer, 2))
        {
          printf("Error input, the name of the cfs file to work with has to be given.\n");
          break;
        }

        superblock* new_superblock = NULL;
        hole_map* new_holes = NULL;
        Stack_List* new_list = NULL;

        int new_fd = cfs_workwith(cfs_filename, &new_superblock, &new_holes, &new_list);
        if (new_fd == -1)
        {
          printf("Error with the given cfs filename.\n");
          break;
        }

        FREE_IF_NOT_NULL(my_superblock);
        FREE_IF_NOT_NULL(holes);
        Stack_List_Destroy(&list);

        my_superblock = new_superblock;
        holes = new_holes;
        list = new_list;

        if (fd != -1)
        {
          int ret = close(fd);
          if (ret == -1)
          {
            perror("Unexpected close() error");
            FREE_IF_NOT_NULL(my_superblock);
            FREE_IF_NOT_NULL(holes);
            Stack_List_Destroy(&list);
            return EXIT_FAILURE;
          }
        }

        fd = new_fd;

        break;
      }

      case 2:
      {

        break;
      }

      case 3:
      {

        break;
      }

      case 4:
      {

        break;
      }

      case 5:
      {

        break;
      }

      case 6:
      {

        break;
      }

      case 7:
      {

        break;
      }

      case 8:
      {

        break;
      }

      case 9:
      {

        break;
      }

      case 10:
      {

        break;
      }

      case 11:
      {

        break;
      }

      case 12:
      {

        break;
      }

      case 13:
      {

        break;
      }

      case 14:
      {
        size_t bs = 0;
        size_t fns = 0;
        size_t cfs = 0;
        uint mdfn = 0;

        char new_cfs_filename[MAX_CFS_FILENAME_SIZE] = {0};

        int retval = get_cfs_create_parameters(buffer, &bs, &fns, &cfs, &mdfn, new_cfs_filename);
        if (!retval)
        {
          printf("Error while reading the parameters of cfs_create\n");
          break;
        }

        retval = cfs_create(new_cfs_filename, bs, fns, cfs, mdfn);
        if (retval == -1)
        {
          printf("Error in cfs_create()\n");
          break;
        }

        break;
      }

      case 15:
      {

        break;
      }

      default:
      {
        char wrong_input[MAX_BUFFER_SIZE] = {0};
        if (get_nth_string(wrong_input, buffer, 1))
        {
          printf("Unknown command: '%s'\n", wrong_input);
        }

        break;
      }
    }

  } while (option != 15);


  printf("\n\n");

  /* MAY CAUSE SEGMENTATION ERROR */
  FREE_IF_NOT_NULL(my_superblock);
  FREE_IF_NOT_NULL(holes);
  Stack_List_Destroy(&list);

  if (fd > 0)
  {
    close(fd);
  }
  return EXIT_SUCCESS;
}
