#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include "record.h"


// int main(int argc, char* argv[])
// {
//   /* store the arguments in variables so that the code becomes more readable */
//   char* inputfile = argv[1];
//   char* number_of_records = argv[2];
//   char* left = argv[3];
//   char* right = argv[4];
//   char* sort_function = argv[5];
//   char* attribute = argv[6];
//   char* pipe_fd = argv[7];
//   char* children_pipes = argv[9];
//
//   /* process id of the coach, used to send a signal to it */
//   pid_t coach_pid = atoi(argv[8]);
//
//   /* decide which sorting algorithm will be used */
//   int flag_quicksort = 1;
//   if (strcmp(sort_function, "q") != 0) {
//     flag_quicksort = 0;
//   }
//
//   /* create new process */
//   pid_t child_pid = fork();
//
//   if (child_pid < 0) {
//
//     printf("Something went wrong with fork() in sorter.c: pipe_fd = %d\n", atoi(pipe_fd));
//     exit(-1);
//
//   } else if (child_pid != 0) {
//
//     /* wait for the child process to end */
//     int returnStatus;
//     wait(&returnStatus);
//
//     if (returnStatus == -1) {
//       printf("The child process terminated with an error in sorter.c!\n");
//       exit(-1);
//     }
//
//   } else {
//
//     /* execute child process -> sort algorithm */
//     if (flag_quicksort == 1) {
//       char *args[] = {"./quicksort", inputfile, attribute, left, right, pipe_fd, children_pipes, NULL};
//       execvp(args[0], args);
//     } else {
//       char *args[] = {"./heapsort", inputfile, attribute, left, right, pipe_fd, children_pipes, NULL};
//       execvp(args[0], args);
//     }
//
//   }
//
//
//   /* send signal SIGUSR2 to the root */
//   kill(coach_pid, SIGUSR2);
//
//   return 0;
// }



int main(int argc, char* argv[])
{
  /* store the arguments in variables so that the code becomes more readable */
  char* inputfile = argv[1];
  char* number_of_records = argv[2];
  char* left = argv[3];
  char* right = argv[4];
  char* sort_function = argv[5];
  char* attribute = argv[6];
  char* pipe_fd = argv[7];
  char* children_pipes = argv[9];

  /* process id of the coach, used to send a signal to it */
  char* coach_pid = argv[8];

  /* decide which sorting algorithm will be used */
  int flag_quicksort = 1;
  if (strcmp(sort_function, "q") != 0) {
    flag_quicksort = 0;
  }

  /* execute child process -> sort algorithm */
  if (flag_quicksort == 1) {
    char *args[] = {"./quicksort", inputfile, attribute, left, right, pipe_fd, children_pipes, coach_pid, NULL};
    execvp(args[0], args);
  } else {
    char *args[] = {"./heapsort", inputfile, attribute, left, right, pipe_fd, children_pipes, coach_pid, NULL};
    execvp(args[0], args);
  }


  return 0;
}
