#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "functions.h"
#include "functions2.h"
#include "functions_util.h"
#include "util.h"
#include "list.h"



void free_mem(superblock** my_superblock, hole_map** holes, Stack_List** list)
{
  FREE_IF_NOT_NULL(*my_superblock);
  *my_superblock = NULL;

  FREE_IF_NOT_NULL(*holes);
  *holes = NULL;

  Stack_List_Destroy(list);
  *list = NULL;
}



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

  /* useful information about the current cfs file that we are working with */
  size_t block_size = 0;
  size_t fns = 0;
  size_t max_entity_size = 0;
  uint max_number_of_files = 0;


  do {

    /* print the filename, if we are currently working with a file */
    if (cfs_filename[0] != 0)
    {
      // [0;32m	Green
      // [1;32m	Bold Green

      /* set print colour to Bold Green */
      printf("\033[1;32m");

      printf("%s", cfs_filename);

      /* set print colour back to normal */
      printf("\033[0m");
      printf(":");
    }

    /* prompt to enter command, just like in linux */
    Stack_List_Print_Directories(list, PRINT_HIERARCHY);
    printf("$ ");
    /* read an option from the user */
    fgets(buffer, MAX_BUFFER_SIZE, stdin);
    /* determine which option we have using the get_option() function */
    option = get_option(buffer);


    switch (option)
    {
      case 1:
      {
        char new_cfs_filename[MAX_CFS_FILENAME_SIZE] = {0};
        if (!get_nth_string(new_cfs_filename, buffer, 2))
        {
          printf("Error input, the name of the cfs file to work with has to be given.\n");
          break;
        }

        superblock* new_superblock = NULL;
        hole_map* new_holes = NULL;
        Stack_List* new_list = NULL;

        int new_fd = cfs_workwith(new_cfs_filename, &new_superblock, &new_holes, &new_list);
        if (new_fd == -1)
        {
          printf("Error with the given cfs filename.\n");
          break;
        }

        free_mem(&my_superblock, &holes, &list);

        my_superblock = new_superblock;
        holes = new_holes;
        list = new_list;

        block_size = my_superblock->block_size;
        fns = my_superblock->filename_size;
        max_entity_size = my_superblock->max_file_size;
        max_number_of_files = my_superblock->max_dir_file_number;
        strcpy(cfs_filename, new_cfs_filename);

        /* close previous open file */
        if (fd != -1)
        {
          int ret = close(fd);
          if (ret == -1)
          {
            perror("Unexpected close() error when closing the previous cfs file. Exiting..");
            FREE_AND_CLOSE(my_superblock, holes, list, new_fd);

            return EXIT_FAILURE;
          }
        }

        fd = new_fd;

        break;
      }

      case 2:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        /* array of characters used to read the user input */
        char read_input[MAX_BUFFER_SIZE] = {0};
        if (!get_nth_string(read_input, buffer, 2))
        {
          printf("Error input, at least 1 new directory has to be given.\n");
          break;
        }

        /* for every directory name given */
        int index = 2;
        int directory_exists = get_nth_string(read_input, buffer, index);
        while (directory_exists)
        {
          /* get the name of the new directory */
          char new_directory_name[MAX_BUFFER_SIZE] = {0};
          extract_last_entity_from_path(read_input, new_directory_name);

          /* check that the limitations of the cfs are met */
          if (strlen(new_directory_name) > fns - 1)
          {
            printf("Error input: the name \"%s\" exceeds the max number of characters that a file name can have: fns = %ld\n", new_directory_name, fns);
            index++;
            directory_exists = get_nth_string(read_input, buffer, index);
            continue;
          }

          /* get the offset of the directory that will host the new directory */
          off_t destination_directory_offset = get_offset_from_path(fd, my_superblock, list, read_input);
          /* check for errors */
          if (destination_directory_offset == (off_t) 0)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if (destination_directory_offset == (off_t) -1)
          {
            index++;
            directory_exists = get_nth_string(read_input, buffer, index);
            continue;
          }

          /* get the destination directory */
          MDS* destination_directory = get_MDS(fd, destination_directory_offset);
          /* check for errors */
          if (destination_directory == NULL)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }

          /* check that the limitations of the cfs are met */
          if (!is_root_offset(my_superblock, destination_directory_offset) && number_of_sub_entities_in_directory(destination_directory, fns) == max_number_of_files)
          {
            if (read_input[0] == 0)
            {
              printf("Can't create the directory \"%s\" because the current directory has already reached max number of sub-entites.\n", new_directory_name);
            }
            else
            {
              printf("Can't create the directory \"%s\" because the directory \"%s\" has already reached max number of sub-entites.\n", new_directory_name, read_input);
            }
            free(destination_directory);
            index++;
            directory_exists = get_nth_string(read_input, buffer, index);
            continue;
          }

          /* make sure than no entity with the same name exists */
          off_t exists = directory_get_offset(fd, destination_directory, block_size, fns, new_directory_name);
          if (exists == (off_t) 0)
          {
            printf("Error in directory_get_offset() when called from main() from case 2.\n");
            free(destination_directory);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if (exists != (off_t) -1)
          {
            if (read_input[0] == 0)
            {
              printf("Can't create the directory \"%s\" because the current directory already contains an entity with the same name.\n", new_directory_name);
            }
            else
            {
              printf("Can't create the directory \"%s\" because the directory \"%s\" contains an entity with the same name.\n", new_directory_name, read_input);
            }
            free(destination_directory);
            index++;
            directory_exists = get_nth_string(read_input, buffer, index);
            continue;
          }


          /* create the new directory */
          int retval = cfs_mkdir(fd, my_superblock, holes, destination_directory, destination_directory_offset, new_directory_name);
          /* free up the allocated space */
          free(destination_directory);
          /* check for errors */
          if (!retval)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }

          /* read the next entry */
          index++;
          directory_exists = get_nth_string(read_input, buffer, index);
        }

        break;
      }

      case 3:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        /* array of characters used to read the user input */
        char read_input[MAX_BUFFER_SIZE] = {0};
        if (!get_nth_string(read_input, buffer, 2))
        {
          printf("Error input: missing file operand.\n");
          break;
        }

        /* get the parameters */
        int flag_a = 0;
        int flag_m = 0;
        int valid_parameters = get_cfs_touch_parameters(buffer, &flag_a, &flag_m);
        if (!valid_parameters)
        {
          break;
        }
        else if (flag_a && flag_m)
        {
          printf("Error: parameters -a and -m cannot be given at the same time.");
          break;
        }


        /* skip the parameter (options) strings */
        int index = 2;
        while (is_parameter(read_input))
        {
          index++;
          get_nth_string(read_input, buffer, index);
        }

        /* for every directory name given */
        int file_exists = get_nth_string(read_input, buffer, index);
        /* make sure that at least one file has been given */
        if (!file_exists)
        {
          printf("Error input: missing file operand.\n");
          break;
        }
        while (file_exists)
        {
          /* get the name of the file */
          char new_file_name[MAX_BUFFER_SIZE] = {0};
          extract_last_entity_from_path(read_input, new_file_name);

          /* check that the limitations of the cfs are met */
          if (strlen(new_file_name) > fns - 1)
          {
            printf("Error input: the name \"%s\" exceeds the max number of characters that a file name can have: fns = %ld\n", new_file_name, fns);
            index++;
            file_exists = get_nth_string(read_input, buffer, index);
            continue;
          }

          /* get the offset of the directory that will host the new directory */
          off_t destination_directory_offset = get_offset_from_path(fd, my_superblock, list, read_input);
          /* check for errors */
          if (destination_directory_offset == (off_t) 0)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if (destination_directory_offset == (off_t) -1)
          {
            index++;
            file_exists = get_nth_string(read_input, buffer, index);
            continue;
          }

          /* get the destination directory */
          MDS* destination_directory = get_MDS(fd, destination_directory_offset);
          /* check for errors */
          if (destination_directory == NULL)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }

          /* check that the limitations of the cfs are met */
          if (!flag_a && !flag_m && !is_root_offset(my_superblock, destination_directory_offset) && number_of_sub_entities_in_directory(destination_directory, fns) == max_number_of_files)
          {
            if (read_input[0] == 0)
            {
              printf("Can't create the file \"%s\" because the current directory has already reached max number of sub-entites.\n", new_file_name);
            }
            else
            {
              printf("Can't create the file \"%s\" because the directory \"%s\" has already reached max number of sub-entites.\n", new_file_name, read_input);
            }
            free(destination_directory);
            index++;
            file_exists = get_nth_string(read_input, buffer, index);
            continue;
          }

          /* make sure than an entity with the same name exists or not, depending on the parameters */
          off_t file_offset = directory_get_offset(fd, destination_directory, block_size, fns, new_file_name);
          /* check for errors */
          if (file_offset == (off_t) 0)
          {
            printf("Error in directory_get_offset() when called from main() from case 3.\n");
            free(destination_directory);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if ((!flag_a && !flag_m) && file_offset != (off_t) -1)
          {
            if (read_input[0] == 0)
            {
              printf("Can't create the file \"%s\" because the current directory already contains an entity with the same name.\n", new_file_name);
            }
            else
            {
              printf("Can't create the file \"%s\" because the directory \"%s\" contains an entity with the same name.\n", new_file_name, read_input);
            }
            free(destination_directory);
            index++;
            file_exists = get_nth_string(read_input, buffer, index);
            continue;
          }


          /* create the file */
          int retval = cfs_touch(fd, my_superblock, holes, destination_directory, destination_directory_offset, new_file_name, file_offset, flag_a, flag_m);
          /* free up the allocated space */
          free(destination_directory);
          /* check for errors */
          if (!retval)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }


          /* read the next entry */
          index++;
          file_exists = get_nth_string(read_input, buffer, index);
        }


        break;
      }

      case 4:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        cfs_pwd(fd, my_superblock, list);

        break;
      }

      case 5:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        char spam[MAX_BUFFER_SIZE] = {0};
        if (get_nth_string(spam, buffer, 3))
        {
          printf("Error: too many arguments for cd.\n");
          break;
        }

        char path[MAX_BUFFER_SIZE] = {0};
        /* get the path */
        get_nth_string(path, buffer, 2);


        /* handle too big paths that overflow the path array */
        if (strlen(path) == MAX_BUFFER_SIZE && path[MAX_BUFFER_SIZE - 1] != 0)
        {
          printf("Path given is too big. Give a path that has totally less than %d characters.\n", MAX_BUFFER_SIZE);
          break;
        }

        /* create a copy of the list (of the current path) */
        Stack_List* copy_list = copy_List(list);
        if (copy_list == NULL)
        {
          printf("Error: copy_List() returned NULL.\n");
          break;
        }

        int retval = cfs_cd(fd, my_superblock, copy_list, path);
        /* check if the operation failed */
        if (retval == 0)
        {
          Stack_List_Destroy(&copy_list);
          break;
        }

        /* "destroy" the previous path */
        Stack_List_Destroy(&list);

        /* get the new path */
        list = copy_list;

        break;
      }

      case 6:
      {
        BREAK_IF_NO_FILE_OPEN(fd);
        int flag_a = 0;
        int flag_r = 0;
        int flag_l = 0;
        int flag_u = 0;
        int flag_d = 0;
        int flag_h = 0;
        int valid_parameters = get_cfs_ls_parameters(buffer, &flag_a, &flag_r, &flag_l, &flag_u, &flag_d, &flag_h);
        if (!valid_parameters)
        {
          printf("Error: not valid parameters.\n");
          break;
        }
        else if (flag_d && flag_h)
        {
          printf("Error: parameters -d and -h cannot be given at the same time.\n");
          break;
        }
        else if (flag_h && flag_a)
        {
          printf("Error: parameters -a and -h cannot be given at the same time.\n");
          break;
        }
        else if (flag_r && flag_h)
        {
          printf("Error: parameters -r and -h cannot be given at the same time.\n");
          break;
        }

        char* cur_name = malloc(MAX_CFS_FILENAME_SIZE * sizeof(char));
        off_t cur_offset;
        Stack_List_Peek(list, &cur_name, &cur_offset);
        cfs_ls(fd, cur_offset, flag_a, flag_r, flag_l, flag_u, flag_d, flag_h);
        free(cur_name);

        break;
      }

      case 7:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        /* get the parameters */
        int flag_R = 0;
        int flag_i = 0;
        int flag_r = 0;
        int valid_parameters = get_cfs_cp_parameters(buffer, &flag_R, &flag_i, &flag_r);
        /* check for errors */
        if (!valid_parameters)
        {
          break;
        }
        else if (flag_R && flag_r)
        {
          printf("Error: parameters -r and -R cannot be given at the same time.");
          break;
        }

        /* array of characters used to read the user input */
        char read_input[MAX_BUFFER_SIZE] = {0};


        /* skip the parameter (options) strings */
        int index = 2;
        int exists = get_nth_string(read_input, buffer, index);
        while (exists && is_parameter(read_input))
        {
          index++;
          exists = get_nth_string(read_input, buffer, index);
        }

        if (!exists)
        {
          printf("Error input: missing file operads.\n");
          break;
        }


        int start_of_sources = index;
        /* iterate to find the destination folder */
        while (exists)
        {
          index++;
          exists = get_nth_string(read_input, buffer, index);
        }

        /* check for errors */
        if (index == start_of_sources + 1)
        {
          printf("Error input: you must give at least 2 files; 1 source and 1 destination.\n");
          break;
        }

        /* counter used later */
        int total_sources = index - start_of_sources - 1;

        /* information about the destination file */
        char destination_directory_path[MAX_BUFFER_SIZE] = {0};
        get_nth_string(destination_directory_path, buffer, index - 1);


        /* get the offset of the destination directory */
        off_t destination_directory_offset = get_offset_from_path(fd, my_superblock, list, read_input);
        if (destination_directory_offset == (off_t) 0)
        {
          break;
        }


        /* get the destination file */
        MDS* destination_directory = get_MDS(fd, destination_directory_offset);
        /* check for errors */
        if (destination_directory == NULL)
        {
          break;
        }
        else if (destination_directory->type != DIRECTORY)
        {
          printf("Error input: destination must be a directory.\n");
          free(destination_directory);
          break;
        }
        else if (!is_root_offset(my_superblock, destination_directory_offset) && number_of_sub_entities_in_directory(destination_directory, fns) == max_number_of_files)
        {
          printf("Error: the directory \"%s\" has reached it's max number of files, therefore nothing can be copied in it.\n", read_input);
          free(destination_directory);
          break;
        }


        /* iterate through all the sources */
        for (index = start_of_sources; index < start_of_sources + total_sources; index++)
        {
          /* get the path of the entity */
          get_nth_string(read_input, buffer, index);

          /* check for errors */
          if (!is_root_offset(my_superblock, destination_directory_offset) && number_of_sub_entities_in_directory(destination_directory, fns) == max_number_of_files)
          {
            printf("Error: entity \"%s\" and those after that can't be copied because the destination directory has reached its max limit in entities.\n", read_input);
            break;
          }

          /* get the offset of the entity to be copied */
          off_t entity_offset = get_offset_from_path(fd, my_superblock, list, read_input);
          /* check for errors */
          if (entity_offset == (off_t) 0)
          {
            free(destination_directory);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if (entity_offset == (off_t) -1)
          {
            printf("Error input: the entity \"%s\" does not exist.\n", read_input);
            /* reset the array used to read the user input */
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }
          else if (is_root_offset(my_superblock, entity_offset))
          {
            printf("Error input: /root directory is the only directory that can't be copied. Therefore the entity \"%s\" can't be copied.\n", read_input);
            /* reset the array used to read the user input */
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }


          /* ask user if he wants to copy the specific file */
          if (flag_i)
          {
            if (!get_approval(read_input, destination_directory_path, "copy"))
            {
              /* reset the array used to read the user input */
              memset(read_input, 0, MAX_BUFFER_SIZE);
              continue;
            }
          }



          /* get the name of the entity in order to check if an entity with the
             same name in the destination directory exists */
          char temp_read_input[MAX_BUFFER_SIZE] = {0};
          strcpy(temp_read_input, read_input);
          char last_entity_name[MAX_BUFFER_SIZE] = {0};
          extract_last_entity_from_path(temp_read_input, last_entity_name);


          /* get the entitys actual name if "." or ".." was given as the last
             pathname */
          if (strcmp(last_entity_name, ".") || strcmp(last_entity_name, ".."))
          {
            /* get the directory's actual name in the array last_entity_name */
            int retval = get_legit_name_from_path(fd, my_superblock, list, read_input, last_entity_name);
            if (!retval)
            {
              printf("Error: operation get_legit_name_from_path() failed when called from main() in case 7. It should never fail. Exiting..\n");
              free(destination_directory);
              FREE_AND_CLOSE(my_superblock, holes, list, fd);
              return EXIT_FAILURE;
            }
          }


          /* check for existance of entity with same name */
          off_t check_if_exists = directory_get_offset(fd, destination_directory, block_size, fns, last_entity_name);
          if (check_if_exists == (off_t) 0)
          {
            free(destination_directory);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if (check_if_exists != (off_t) -1)
          {
            if (read_input[0] != 0)
            {
              printf("Error input: entity named \"%s\" already exists in directory \"%s\".\n", last_entity_name, read_input);
            }
            else
            {
              printf("Error input: entity named \"%s\" already exists in current directory.\n", last_entity_name);
            }
            /* reset the array used to read the user input */
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }


          /* get the entity */
          MDS* entity = get_MDS(fd, entity_offset);
          /* check for errors */
          if (entity == 0)
          {
            /* reset the array used to read the user input */
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }


          /* call cfs_cp to copy the source file to the destination directory */
          int retval = cfs_cp(fd, my_superblock, holes, entity, last_entity_name, destination_directory, destination_directory_offset, flag_R, flag_i, flag_r, read_input, destination_directory_path);
          if (!retval)
          {
            printf("Unexpected error in cfs_cp. Exiting..\n");
            free(entity);
            free(destination_directory);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);

            return EXIT_FAILURE;
          }


          /* free the entity allocated */
          free(entity);
          /* reset the array used to read the user input */
          memset(read_input, 0, MAX_BUFFER_SIZE);
        }

        /* free up the allocated memory */
        free(destination_directory);

        break;
      }

      case 8:
      {
        /* array used to read strings from the user input */
        char read_input[MAX_BUFFER_SIZE] = {0};

        int index = 2;
        int exists = get_nth_string(read_input, buffer, index);
        /* count and skip the source files */
        while (exists && strcmp(read_input, "-o"))
        {
          index++;
          exists = get_nth_string(read_input, buffer, index);
        }

        /* check for errors */
        if (index == 2)
        {
          printf("Error input: you must give at least one source file.\n");
          break;
        }
        if (!exists)
        {
          printf("Error input: you must give the flag \"-o\" to mark the destination file.\n");
          break;
        }

        /* how many source files are being added up */
        int total_sources = index - 2;

        /* array used to store the name of the output file */
        char destination_file_path[MAX_BUFFER_SIZE] = {0};
        index++;

        /* make sure that a destination is given */
        if (!get_nth_string(destination_file_path, buffer, index))
        {
          printf("Error input: you must give a destination file.\n");
          break;
        }

        /* make sure that no more than 1 destinations are given */
        if (get_nth_string(read_input, buffer, index + 1))
        {
          printf("Error input: only 1 destination file can be given.\n");
          break;
        }


        /* extract the name of the destination path, and check if a file with
           the same name already exists */
        char destination_file_name[MAX_BUFFER_SIZE] = {0};
        extract_last_entity_from_path(destination_file_path, destination_file_name);
        /* check that cfs limitations are met */
        if (strlen(destination_file_name) > fns - 1)
        {
          printf("Error input: the output file name \"%s\" exceeds the max number of characters that a filename can have.\n", destination_file_name);
          break;
        }

        Stack_List* destination_path_list = copy_List(list);
        if (destination_path_list == NULL)
        {
          break;
        }

        /* check if the output file is to be placed in another directory. If so,
           then build the list to point at that directory */
        if (destination_file_path[0] != 0)
        {
          int retval = cfs_cd(fd, my_superblock, destination_path_list, destination_file_path);
          /* check if the operation failed */
          if (retval == 0)
          {
            Stack_List_Destroy(&destination_path_list);
            break;
          }
        }


        /* get the offset of the current directory, in order to get the directory */
        off_t destination_directory_offset = Stack_List_Peek_offset(destination_path_list);
        if (destination_directory_offset == (off_t) 0)
        {
          Stack_List_Destroy(&destination_path_list);
          printf("Error in Stack_List_Peek_offset() when called from main:: cfs_cat.\n");
          break;
        }


        /* get the directory in which the output file will be placed */
        MDS* destination_file_directory = get_MDS(fd, destination_directory_offset);
        if (destination_file_directory == NULL)
        {
          Stack_List_Destroy(&destination_path_list);
          break;
        }
        else if (number_of_sub_entities_in_directory(destination_file_directory, fns) == max_number_of_files)
        {
          if (destination_file_path[0] == 0)
          {
            printf("Error input: the current directory has reached it's max number of files, therefore the file \"%s\" can't be created\n", destination_file_name);
          }
          else
          {
            printf("Error input: the directory \"%s\" has reached it's max number of files, therefore the file \"%s\" can't be created\n", destination_file_path, destination_file_name);
          }
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          break;
        }


        /* check if the file already exists in the directory we want to place it */
        off_t destination_file_offset = directory_get_offset(fd, destination_file_directory, block_size, fns, destination_file_name);
        if (destination_file_offset == 0)
        {
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          break;
        }
        else if (destination_file_offset != (off_t) -1)
        {
          if (destination_file_path[0] == 0)
          {
            printf("Error input: a file with the same name as the output file already exists in the current directory.\n");
          }
          else
          {
            printf("Error input: a file with the same name as the output file already exists in the %s directory.\n", destination_file_path);
          }
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          break;
        }

        /* create the output file, in order to concatenate to it the source files */
        int retval = cfs_touch(fd, my_superblock, holes, destination_file_directory, destination_directory_offset, destination_file_name, -1, 0, 0);
        if (!retval)
        {
          printf("Unexpected error occured in cfs_cat(). Exiting..\n");
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }


        /* now actually get the offset of the destination file inside its directory */
        destination_file_offset = directory_get_offset(fd, destination_file_directory, block_size, fns, destination_file_name);
        if (destination_file_offset == 0)
        {
          free(destination_file_directory);
          Stack_List_Destroy(&destination_path_list);
          break;
        }

        /* we do not need the MDS of the directory anymore */
        free(destination_file_directory);

        /* get the file to change its attributes */
        MDS* destination_file = get_MDS(fd, destination_file_offset);
        if (destination_file_directory == NULL)
        {
          Stack_List_Destroy(&destination_path_list);
          break;
        }


        int i = 0;
        size_t destination_file_size = 0;
        /* iterate to concatenate all the source files */
        for (; i < total_sources; i++)
        {
          /* get the path of the file */
          get_nth_string(read_input, buffer, i + 2);

          /* get the offset of the source file */
          off_t source_file_offset = get_offset_from_path(fd, my_superblock, list, read_input);
          if (source_file_offset == (off_t) 0)
          {
            continue;
          }

          /* get the source file */
          MDS* source_file = get_MDS(fd, source_file_offset);
          /* check for errors */
          if (source_file == 0)
          {
            continue;
          }
          else if (source_file->type != FILE)
          {
            char last_entity_name[MAX_BUFFER_SIZE] = {0};
            extract_last_entity_from_path(read_input, last_entity_name);
            printf("Error input: entity named %s is not a file.\n", last_entity_name);
            free(source_file);

            continue;
          }
          else if (destination_file_size + source_file->size > max_entity_size)
          {
            char last_entity_name[MAX_BUFFER_SIZE] = {0};
            extract_last_entity_from_path(read_input, last_entity_name);
            printf("Error input: entity named %s can't be concatenated to file %s because if would exceed max capacity of file.\n", last_entity_name, destination_file_name);
            free(source_file);

            continue;
          }

          /* if source file is not empty, concatenate */
          if (source_file->size > 0)
          {
            retval = cfs_cat(fd, my_superblock, holes, destination_file, destination_file_offset, source_file);
            if (!retval)
            {
              free(source_file);
              free(destination_file);
              Stack_List_Destroy(&destination_path_list);
              FREE_AND_CLOSE(my_superblock, holes, list, fd);
              return EXIT_FAILURE;
            }

            retval = set_MDS(destination_file, fd, destination_file_offset);
            if (!retval)
            {
              free(source_file);
              free(destination_file);
              Stack_List_Destroy(&destination_path_list);
              FREE_AND_CLOSE(my_superblock, holes, list, fd);
              return EXIT_FAILURE;
            }
          }

          /* increment size */
          destination_file_size += source_file->size;
          /* free source file */
          free(source_file);

          memset(read_input, 0, MAX_BUFFER_SIZE);
        }


        /* update the superblock */
        my_superblock->current_size += sizeof(MDS) + block_size * destination_file->blocks_using;
        my_superblock->total_entities++;
        retval = set_superblock(my_superblock, fd);
        if (!retval)
        {
          free(destination_file);
          Stack_List_Destroy(&destination_path_list);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }

        /* update the hole map */
        retval = set_hole_map(holes, fd);
        if (!retval)
        {
          free(destination_file);
          Stack_List_Destroy(&destination_path_list);
          FREE_AND_CLOSE(my_superblock, holes, list, fd);

          return EXIT_FAILURE;
        }


        /* free up the used space */
        free(destination_file);
        Stack_List_Destroy(&destination_path_list);

        break;
      }

      case 9:
      {
        /* array used to read strings from the user input */
        char read_input[MAX_BUFFER_SIZE] = {0};

        /* arrays to store the paths of the files given as parameters */
        char source_file_path[MAX_BUFFER_SIZE] = {0};
        char output_file_path[MAX_BUFFER_SIZE] = {0};


        /* check for correct input */
        int exists = get_nth_string(source_file_path, buffer, 2);
        if (!exists)
        {
          printf("Error input: you must give at least one source file and one output file.\n");
          break;
        }

        /* check for correct input */
        exists = get_nth_string(output_file_path, buffer, 3);
        if (!exists)
        {
          printf("Error input: you must give at least one output file.\n");
          break;
        }

        /* check for correct input */
        exists = get_nth_string(read_input, buffer, 4);
        if (exists)
        {
          printf("Error input: too many arguments. Input should be something like: cfs_ln source_file output_file.\n");
          break;
        }


        int retval = cfs_ln(fd, my_superblock, holes, list, source_file_path, output_file_path);
        if (retval == 0)
        {
          FREE_AND_CLOSE(my_superblock, holes, list, fd);
          return EXIT_FAILURE;
        }

        break;
      }

      case 10:
      {
         print_hole_table(holes);
        holes->holes_table[0].start = 100;
        holes->holes_table[0].end = 200;
        holes->current_hole_number = 1;

        holes->holes_table[1].start = 300;
        holes->holes_table[1].end = 400;
        holes->current_hole_number++;

        holes->holes_table[2].start = 700;
        holes->holes_table[2].end = 1000;
        holes->current_hole_number++;

        holes->holes_table[3].start = 1300;
        holes->holes_table[3].end = 1400;
        holes->current_hole_number++;

        holes->holes_table[4].start = 3500;
        holes->holes_table[4].end = 0;
        holes->current_hole_number++;

        print_hole_table(holes);

        insert_hole(holes, 0, 50, fd);
        print_hole_table(holes);

        insert_hole(holes, 200, 300, fd);
        print_hole_table(holes);

        insert_hole(holes, 600, 700, fd);
        print_hole_table(holes);


        insert_hole(holes, 2400, 3500, fd);
        print_hole_table(holes);

        insert_hole(holes, 400, 450, fd);
        print_hole_table(holes);

        insert_hole(holes, 2100, 2300, fd);
        print_hole_table(holes);


        break;
      }

      case 11:
      {

        break;
      }

      case 12:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        /* array used to read the input of the user */
        char read_input[MAX_BUFFER_SIZE] = {0};

        /* check for correct input */
        int exists = get_nth_string(read_input, buffer, 2);
        if (!exists)
        {
          printf("Error input: you must give at least one source file and one destination directory.\n");
          break;
        }

        /* check for correct input */
        exists = get_nth_string(read_input, buffer, 3);
        if (!exists)
        {
          printf("Error input: you must give one destination directory.\n");
          break;
        }


        /* count the number of sources and get the destination directory */
        int index = 2;
        exists = get_nth_string(read_input, buffer, index);
        while (exists)
        {
          index++;
          exists = get_nth_string(read_input, buffer, index);
        }

        /* counter used later */
        uint total_sources = index - 3;

        /* get the path of the destination directory */
        char destination_directory_path[MAX_BUFFER_SIZE] = {0};
        get_nth_string(destination_directory_path, buffer, index - 1);

        /* get the offset of the destination directory */
        off_t destination_directory_offset = get_offset_from_path(fd, my_superblock, list, destination_directory_path);
        /* check for errors */
        if (destination_directory_offset == (off_t) 0)
        {
          break;
        }
        else if (destination_directory_offset == (off_t) -1)
        {
          printf("Error input: the destination directory \"%s\" does not exist.\n", destination_directory_path);
          break;
        }

        /* get the destination directory */
        MDS* destination_directory = get_MDS(fd, destination_directory_offset);
        /* check for errors */
        if (destination_directory == NULL)
        {
          break;
        }
        else if (destination_directory->type != DIRECTORY)
        {
          printf("Error input: the destination must be a directory.\n");
          free(destination_directory);
          break;
        }
        else if (!is_root_offset(my_superblock, destination_directory_offset) && number_of_sub_entities_in_directory(destination_directory, fns) == max_number_of_files)
        {
          printf("Error input: the directory \"%s\" has reached its max capacity, and therefore it can't import new files.\n", destination_directory_path);
          free(destination_directory);
          break;
        }


        /* iterate through all the sources to import them all */
        for (index = 2; index < total_sources + 2; index++)
        {
          /* read the source to be imported */
          get_nth_string(read_input, buffer, index);

          /* make sure that the new entity can be imported */
          if (!is_root_offset(my_superblock, destination_directory_offset) && number_of_sub_entities_in_directory(destination_directory, fns) == max_number_of_files)
          {
            printf("Error input: the directory \"%s\" has reached its max capacity, therefore the entity \"%s\" and those after that can't be imported.\n", destination_directory_path, read_input);
            break;
          }

          /* get the name of the entity in order to check if an entity with the
             same name in the destination directory exists */
          char temp_read_input[MAX_BUFFER_SIZE] = {0};
          strcpy(temp_read_input, read_input);
          char last_entity_name[MAX_BUFFER_SIZE] = {0};
          extract_last_entity_from_path(temp_read_input, last_entity_name);

          /* make sure that the limitations of the cfs are met */
          if (strlen(last_entity_name) > fns - 1)
          {
            printf("Error input: the name \"%s\" exceeds the max number of characters that a file name can have: fns = %ld\n", last_entity_name, fns);
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }

          /* make sure that no entity with the same name exists in the
             destination directory */
          off_t check_if_exists = directory_get_offset(fd, destination_directory, block_size, fns, last_entity_name);
          /* check for errors */
          if (check_if_exists == (off_t) 0)
          {
            free(destination_directory);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if (check_if_exists != (off_t) -1)
          {
            if (read_input[0] != 0)
            {
              printf("Error input: entity named \"%s\" already exists in directory \"%s\".\n", last_entity_name, read_input);
            }
            else
            {
              printf("Error input: entity named \"%s\" already exists in current directory.\n", last_entity_name);
            }
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }

          /* import the files */
          int retval = cfs_import(fd, my_superblock, holes, destination_directory, destination_directory_offset, read_input);
          if (!retval)
          {
            printf("Something unexpected happened in cfs_import() when called from main. Exiting..\n");
            free(destination_directory);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }

          /* reset the read input */
          memset(read_input, 0, MAX_BUFFER_SIZE);
        }


        /* free up the allocated space */
        free(destination_directory);

        break;
      }

      case 13:
      {
        BREAK_IF_NO_FILE_OPEN(fd);

        /* array used to read the input of the user */
        char read_input[MAX_BUFFER_SIZE] = {0};

        /* check for correct input */
        int exists = get_nth_string(read_input, buffer, 2);
        if (!exists)
        {
          printf("Error input: you must give at least one source file and one destination directory.\n");
          break;
        }

        /* check for correct input */
        exists = get_nth_string(read_input, buffer, 3);
        if (!exists)
        {
          printf("Error input: you must give one destination directory.\n");
          break;
        }

        /* count the number of sources and get the destination directory */
        int index = 2;
        exists = get_nth_string(read_input, buffer, index);
        while (exists)
        {
          index++;
          exists = get_nth_string(read_input, buffer, index);
        }

        /* counter used later */
        uint total_sources = index - 3;

        /* get the path of the destination directory */
        char destination_directory_path[MAX_BUFFER_SIZE] = {0};
        get_nth_string(destination_directory_path, buffer, index - 1);


        /* variable that will store directory statistics */
        struct stat file_statistics = {0};
        /* get the file statistics */
        int retval = stat(destination_directory_path, &file_statistics);
        /* check for errors */
        if (retval == -1)
        {
          if (errno == ENOENT)
          {
            printf("Error input: the given linux path \"%s\" does not exist.\n", destination_directory_path);
            break;
          }
          else
          {
            perror("stat()");
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
        }

        /* make sure that the destination is a directory */
        if ((file_statistics.st_mode & S_IFMT) != S_IFDIR)
        {
          printf("Error input: the given linux entity \"%s\" is not a directory.\n", destination_directory_path);
          break;
        }



        /* iterate through all the sources to import them all */
        for (index = 2; index < total_sources + 2; index++)
        {
          /* read the source to be imported */
          get_nth_string(read_input, buffer, index);

          /* get the offset of the entity to be copied */
          off_t entity_offset = get_offset_from_path(fd, my_superblock, list, read_input);
          /* check for errors */
          if (entity_offset == (off_t) 0)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }
          else if (entity_offset == (off_t) -1)
          {
            printf("Error input: the entity \"%s\" does not exist.\n", read_input);
            /* reset the array used to read the user input */
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }


          /* get the name of the entity in order to construct the linux path */
          char temp_read_input[MAX_BUFFER_SIZE] = {0};
          strcpy(temp_read_input, read_input);
          char last_entity_name[MAX_BUFFER_SIZE] = {0};
          extract_last_entity_from_path(temp_read_input, last_entity_name);

          /* get the entitys actual name if "." or ".." was given as the last
             pathname */
          if (strcmp(last_entity_name, ".") || strcmp(last_entity_name, ".."))
          {
            /* get the directory's actual name in the array last_entity_name */
            int retval = get_legit_name_from_path(fd, my_superblock, list, read_input, last_entity_name);
            if (!retval)
            {
              printf("Error: operation get_legit_name_from_path() failed when called from main() in case 7. It should never fail. Exiting..\n");
              FREE_AND_CLOSE(my_superblock, holes, list, fd);
              return EXIT_FAILURE;
            }
          }

          /* the linux name that the entity will have */
          char linux_path_name[MAX_BUFFER_SIZE] = {0};
          strcpy(linux_path_name, destination_directory_path);
          strcat(linux_path_name, "/");
          strcat(linux_path_name, last_entity_name);

          /* variable used just to determine whether a file with the same name
             already exists or not in the linux file system */
          struct stat temp_statistics = {0};
          /* get the file statistics */
          int retval = stat(linux_path_name, &temp_statistics);
          /* check for errors */
          if (retval != -1)
          {
            printf("Error input: an entity with name \"%s\" already exists in the linux file system, therefore the entity \"%s\" can't be exported\n", linux_path_name, read_input);
            /* reset the array used to read the user input */
            memset(read_input, 0, MAX_BUFFER_SIZE);
            continue;
          }
          else if (retval == -1 && errno != ENOENT)
          {
            perror("stat()");
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }

          /* get the entity that will be exported */
          MDS* entity = get_MDS(fd, entity_offset);
          if (entity == NULL)
          {
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }


          /* export the entity */
          retval = cfs_export(fd, my_superblock, entity, linux_path_name);
          /* check for errors */
          if (!retval)
          {
            printf("Something unexpected happened in cfs_export() when called from main. Exiting..\n");
            free(entity);
            FREE_AND_CLOSE(my_superblock, holes, list, fd);
            return EXIT_FAILURE;
          }


          /* free up the allocated space */
          free(entity);
          /* reset the read input */
          memset(read_input, 0, MAX_BUFFER_SIZE);
        }


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
          // printf("Error while reading the parameters of cfs_create.\n");
          break;
        }

        retval = cfs_create(new_cfs_filename, bs, fns, cfs, mdfn);
        if (retval == -1)
        {
          printf("Error in cfs_create().\n");
          break;
        }

        break;
      }

      case 15:
      {
        printf("\nTerminating the program..\n");

        break;
      }

      default:
      {
        char wrong_input[MAX_BUFFER_SIZE] = {0};
        if (get_nth_string(wrong_input, buffer, 1))
        {
          printf("Unknown command: \"%s\".\n", wrong_input);
        }

        break;
      }
    }

  } while (option != 15);

  if (max_entity_size || block_size);

  free_mem(&my_superblock, &holes, &list);

  if (fd > 0)
  {
    close(fd);
  }
  return EXIT_SUCCESS;
}
