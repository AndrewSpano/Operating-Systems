#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "functions.h"
#include "functions2.h"
#include "util.h"
#include "list.h"



int main(int argc, char* argv[])
{

  Stack_List* list = NULL;

  int retval = cfs_create("test.cfs", 512, 30, 5000, 10);
  int fd = cfs_workwith("test.cfs", &list);
  cfs_read("test.cfs", fd);

  printf("\n\n");
  Stack_List_Destroy(&list);
  close(fd);
  return EXIT_SUCCESS;
}
