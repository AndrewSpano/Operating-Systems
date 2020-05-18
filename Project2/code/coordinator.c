#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "record.h"



int get_input(char** inputfile, int** attributes_to_sort, char** sort_function, int* number_of_attributes, int argc, char* argv[]);
void free_input(char** inputfile, int** attributes_to_sort, char** sort_function);
long count_records(char* inputfile);
int create_pipes(int*** pipes, int n);
void free_pipes(int*** pipes, int n);

void get_coaches_times(double** coaches_times, int** coaches_time_execution_pipes, int coaches);
void free_coaches_times(double** coaches_times, int coaches);

void get_sorters_times(double*** sorters_times, int** pipe, int coaches);
void free_sorters_times(double*** sorters_times, int coaches);

void get_signals(int** signals, int** pipes, int coaches);
void free_signals(int** signals, int coaches);

double get_min(double* arr, int n);
double get_max(double* arr, int n);
double get_average(double* arr, int n);

int power(int base, int exp);




int main(int argc, char* argv[])
{
  if (argc != 3 && argc != 5 && argc != 7 && argc != 9 && argc != 11) {
    printf("\nWrong amount of command line parameters was given.\n");
    printf("Correct layout of input:\n");
    printf("./mysort -f inputfile [-h|-q columnid] [-h|-q columnid] [-h|-q columnid] [-h|-q columnid]\n\n");
    return -1;
  }


  /* struct defined in <sys/time.h> used to measure turnaround time*/
  struct timeval start, end;
  gettimeofday(&start, NULL);

  /* matrices that will store the information coaches will be using to make
     the correct sorts */
  char* inputfile = NULL;
  int* attributes_to_sort = NULL;
  char* sort_function = NULL;

  /* a variable that indicates how many attribute we have to sort.
     Technically, it's also the amount of coaches that we will have */
  // int number_of_attributes = (argc - 4) / 2 + 1;
  int number_of_attributes = 0;

  if (get_input(&inputfile, &attributes_to_sort, &sort_function, &number_of_attributes, argc, argv) == 0) {
    printf("\nWrong layout of command line parameters was given.\n");
    printf("Correct layout of input:\n");
    printf("./mysort -f inputfile [-h|-q columnid] [-h|-q columnid] [-h|-q columnid] [-h|-q columnid]\n");
    printf("Also note that columnid is an integer between 1 and 8, that is 1 <= columnid <= 8.\n\n");
    return -2;
  }



  /* pretty much self explanatory */
  const long number_of_records = count_records(inputfile);
  if (number_of_records == -1) {
    printf("There was a problem opening the inputfile in coordinator.c!\n");
    free_input(&inputfile, &attributes_to_sort, &sort_function);
    return -3;
  }


  /* create an array of pipes that will be used to transfer to the coordinator the execution time of the coaches */
  int** coaches_time_execution_pipes = NULL;
  if (create_pipes(&coaches_time_execution_pipes, number_of_attributes) == 0) {
    printf("Something went wrong when creating the pipes for the coaches SIGUSR2 signals in coordinator.c!\n");
    free_input(&inputfile, &attributes_to_sort, &sort_function);
    return -4;
  }


  /* create an array of pipes that will be used to transfer to the coordinator the execution time of the sorters for every coach */
  int** sorters_time_execution_pipes = NULL;
  if (create_pipes(&sorters_time_execution_pipes, number_of_attributes) == 0) {
    printf("Something went wrong when creating the pipes for the sorters execution time in coordinator.c!\n");
    free_pipes(&coaches_time_execution_pipes, number_of_attributes);
    free_input(&inputfile, &attributes_to_sort, &sort_function);
    return -5;
  }


  /* create an array of pipes that will be used to transfer to the coordinator the number of SIGUSR2 signals that each coach received */
  int** signals_arrived_pipes = NULL;
  if (create_pipes(&signals_arrived_pipes, number_of_attributes) == 0) {
    printf("Something went wrong when creating the pipes for the received signals in coordinator.c!\n");
    free_pipes(&sorters_time_execution_pipes, number_of_attributes);
    free_pipes(&coaches_time_execution_pipes, number_of_attributes);
    free_input(&inputfile, &attributes_to_sort, &sort_function);
  }


  /* create new processes -> coaches */
  pid_t child_pid;
  int coach = 0;

  for (coach = 0; coach < number_of_attributes; coach++) {

    child_pid = fork();

    if (child_pid < 0) {

      printf("\nSomething went wrong when creating coach%d. Error in coordinator.c\n\n", coach);
      free_pipes(&signals_arrived_pipes, number_of_attributes);
      free_pipes(&sorters_time_execution_pipes, number_of_attributes);
      free_pipes(&coaches_time_execution_pipes, number_of_attributes);
      free_input(&inputfile, &attributes_to_sort, &sort_function);
      return -10;

    } else  if (child_pid == 0) {

      /* the below information will all be converted to strings so that they
         can be stored in the args array, which is the array that will be used
         to invoke execvp() */

      char* sort = malloc(sizeof(char) * (sizeof(int) + 1));
      sprintf(sort, "%c", sort_function[coach]);

      char* attribute = malloc(sizeof(char) * 2);
      sprintf(attribute, "%d", attributes_to_sort[coach]);

      char* records_number = malloc(sizeof(char) * (MAX_LONG_DIGITS + 1));
      sprintf(records_number, "%ld", number_of_records);

      char* coach_string = malloc(sizeof(char) * 2);
      sprintf(coach_string, "%d", coach);

      char* coaches_pipe_as_string = malloc(sizeof(char) * (MAX_INT_DIGITS + 1));
      sprintf(coaches_pipe_as_string, "%d", coaches_time_execution_pipes[coach][WRITE]);

      char* sorters_pipe_as_string = malloc(sizeof(char) * (MAX_INT_DIGITS + 1));
      sprintf(sorters_pipe_as_string, "%d", sorters_time_execution_pipes[coach][WRITE]);

      char* signals_received_as_string = malloc(sizeof(char) * (MAX_INT_DIGITS + 1));
      sprintf(signals_received_as_string, "%d", signals_arrived_pipes[coach][WRITE]);


      /* invoke child process -> coach */
      char *args[] = {"./coach", coach_string, records_number, inputfile, sort, attribute, coaches_pipe_as_string, sorters_pipe_as_string, signals_received_as_string, NULL};
      execvp(args[0], args);

    }

  }

  /* wait for the child processes (coaches) to end */
  int returnStatus;
  while (wait(&returnStatus) > 0);

  /* if a problem occurs, terminate the program */
  if (returnStatus == -1) {
    printf("A child process terminated with an error in coordinator.c!\n");
    free_pipes(&signals_arrived_pipes, number_of_attributes);
    free_pipes(&sorters_time_execution_pipes, number_of_attributes);
    free_pipes(&coaches_time_execution_pipes, number_of_attributes);
    free_input(&inputfile, &attributes_to_sort, &sort_function);
    return -100;
  }


  /* variables used to print statistics */
  double min = 0.0, max = 0.0, average = 0.0;

  /* array used to store the execution time of coaches */
  double* coaches_times = NULL;
  get_coaches_times(&coaches_times, coaches_time_execution_pipes, number_of_attributes);

  /* pretty much self explanatory */
  min = get_min(coaches_times, number_of_attributes);
  max = get_max(coaches_times, number_of_attributes);
  average = get_average(coaches_times, number_of_attributes);
  printf("\n\nMin execution time for Coach: %f\nMax execution time for Coach %f\nAverage execution time for coach %f\n\n", min, max, average);


  /* array of arrays used to store the execution of every sorter for every coach */
  double** sorters_times = NULL;
  get_sorters_times(&sorters_times, sorters_time_execution_pipes, number_of_attributes);

  int sorters = 0;
  min = 0.0;
  max = 0.0;
  average = 0.0;

  /* pretty much self explanatory */
  for (coach = 0; coach < number_of_attributes; coach++) {
    sorters = power(2, coach);
    min = get_min(sorters_times[coach], sorters);
    max = get_max(sorters_times[coach], sorters);
    average = get_average(sorters_times[coach], sorters);
    printf("For coach %d:\tMin sorter execution time was %f sec\tMax sorter execution time was %f sec\tAverage sorter execution time was %f sec.\n", coach, min, max, average);
  }

  printf("\n");

  /* array used to store the number of signals received by every coach */
  int* signals_arrived = NULL;
  get_signals(&signals_arrived, signals_arrived_pipes, number_of_attributes);

  /* pretty much self explanatory */
  for (coach = 0; coach < number_of_attributes; coach++) {
    printf("%d SIGUSR2 signals arrived in coach %d\n", signals_arrived[coach], coach);
  }

  printf("\n");


  /* free all the allocated memory */
  free_signals(&signals_arrived, number_of_attributes);
  free_coaches_times(&coaches_times, number_of_attributes);
  free_sorters_times(&sorters_times, number_of_attributes);
  free_pipes(&signals_arrived_pipes, number_of_attributes);
  free_pipes(&sorters_time_execution_pipes, number_of_attributes);
  free_pipes(&coaches_time_execution_pipes, number_of_attributes);
  free_input(&inputfile, &attributes_to_sort, &sort_function);


  /* calculate the turnaround time */
  gettimeofday(&end, NULL);

  double tournaround_time = ((double) ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec))) / 1000000.0;
  printf("Turnaround Time = %lf\n\n\n", tournaround_time);

  return 0;
}




/* gets the input from the argc, argv[] arguments, and places them in the variables that were passed
   as parameters. If a problem occurs, then it returns 0 */
int get_input(char** inputfile, int** attributes_to_sort, char** sort_function, int* number_of_attributes, int argc, char* argv[])
{
  /* pretty much self explanatory */
  *number_of_attributes = 0;

  /* temp array to temporarily store the different attributes that we find */
  int* temp_attributes = malloc(sizeof(int) * MAX_COACHES);
  memset(temp_attributes, 0, MAX_COACHES);

  /* temp array to temporarily store the different sorts that we find */
  char* temp_sorts = malloc(sizeof(char) * (MAX_COACHES + 1));
  memset(temp_sorts, '\0', MAX_COACHES + 1);

  /* a counter to keep track of how many attribute information has been
     stored */
  int counter = 0;

  /* a "boolean" to see if the flag "-f" has been found */
  int flag_inputfile_found = 0;
  /* a "boolean" to determine whether we have duplicates attributes */
  int flag_attribute_already_found = 0;


  int i = 0, j = 0;
  for (i = 1; i < argc; i += 2) {

    /* if we find the "-f" file flag, store the name of the file in the
       inputfile string */
    if (strcmp(argv[i], "-f") == 0 && (flag_inputfile_found == 0)) {

      *inputfile = malloc(sizeof(char) * (strlen(argv[i + 1]) + 1));
      strcpy(*inputfile, argv[i + 1]);
      flag_inputfile_found = 1;

    /* else if we find an attribute-sort flag, store the information */
  } else if ((strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "-h") == 0) && (counter < MAX_COACHES)) {

      int attribute = atoi(argv[i + 1]);

      /* check if there has been made an error in the input */
      if (attribute < 1 || attribute > 8) {
        /* if there is an error, free the memory that was allocated and exit */
        free(temp_attributes);
        free(temp_sorts);

        if (flag_inputfile_found == 1) {
          free(*inputfile);
        }
        /* exit */
        return 0;
      }

      /* check if we have already found the attribute */
      flag_attribute_already_found = 0;

      for (j = 0; j < counter; j++) {
        if (temp_attributes[j] == attribute) {
          flag_attribute_already_found = 1;
          break;
        }
      }

      /* If we haven't found it, add in the attributes array.
         Else, do nothing */
      if (flag_attribute_already_found == 0) {
        temp_attributes[counter] = attribute;
        temp_sorts[counter] = argv[i][1];
        counter++;
      }

    } else {

      /* if we get inside this else statement, it means that we have an invalid
         flag in the command line parameters. So we have a wrong input,
         therefore free the memory that was allocated and exit */
      free(temp_attributes);
      free(temp_sorts);

      if (flag_inputfile_found == 1) {
        free(*inputfile);
      }
      /* exit */
      return 0;

    }

  }


  /* check if in the given parameters the inputfile was given */
  if (flag_inputfile_found == 0) {

    /* if it was not given, then the input is wrong. So free the allocated
       memory and exit */
    free(temp_attributes);
    free(temp_sorts);
    /* exit */
    return 0;

  }

  /* is argc == 3, then no attributes have been given to sort the records,
     so we will sort the records with quick sort, by the first attribute */
  if (argc == 3) {

    *attributes_to_sort = malloc(sizeof(int));
    **attributes_to_sort = 1;

    *sort_function = malloc(sizeof(char) * 2);
    strcpy(*sort_function, "q");

    *number_of_attributes = 1;

  /* else, prepare the matrices to store information that coaches and sorters
     will use to do the sorting */
  } else {

    if (counter == 0) {
      printf("Error in coordinator.c::get_input(). Counter cannot be 0.\n");
      free(temp_attributes);
      free(temp_sorts);
      return 0;
    }

    /* allocate memory for the arrays */
    *attributes_to_sort = malloc(sizeof(int) * counter);
    *sort_function = malloc(sizeof(char) * (counter + 1));

    /* set the values */
    for (j = 0; j < counter; j++) {
      (*attributes_to_sort)[j] = temp_attributes[j];
      (*sort_function)[j] = temp_sorts[j];
    }

    *number_of_attributes = counter;

  }


  free(temp_attributes);
  free(temp_sorts);

  return 1;
}




/* frees the memory allocated to store the input */
void free_input(char** inputfile, int** attributes_to_sort, char** sort_function)
{
  free(*inputfile);
  *inputfile = NULL;

  free(*attributes_to_sort);
  *attributes_to_sort = NULL;

  free(*sort_function);
  *sort_function = NULL;
}




/* returns the number of records in a file. If an error occurs, it returns -1 */
long count_records(char* inputfile)
{
  /* open binary file and get the number of records it contains */
  FILE* records_file = fopen (inputfile, "rb");
  if (records_file == NULL) {
     printf("\n\nCannot open binary file. Error in coordinator.c\n\n");
     return -1;
  }

  /* calculate the number of records in the file */
  fseek(records_file , 0 , SEEK_END);
  long size = ftell(records_file);
  fclose(records_file);

  return (int) size / sizeof(Record);
}




/* function that allocates memory for n pipes */
int create_pipes(int*** pipes, int n)
{
  int i = 0;
  /* create the pipes array */
  *pipes = malloc(sizeof(int *) * n);
  for (i = 0; i < n; i++) {

    /* allocate the i-th pipe */
    (*pipes)[i] = malloc(sizeof(int) * 2);

    if (pipe((*pipes)[i]) < 0) {

      printf("Error in creating pipes in coordinator.c\n");
      /* de-allocate the allocated memory and exit the process */
      int j = 0;
      for (j = 0; j <= i; j++) {
        free((*pipes)[i]);
      }
      free(*pipes);

      return 0;
    }

  }

  return 1;
}




/* frees the memory of the allocated pipes */
void free_pipes(int*** pipes, int n)
{
  int i = 0;
  for (i = 0; i < n; i++) {
    free((*pipes)[i]);
  }
  free(*pipes);
  *pipes = NULL;
}




/* gets the execution times of coaches, and stores them in a double array that it creates */
void get_coaches_times(double** coaches_times, int** pipes, int coaches)
{
  *coaches_times = malloc(sizeof(double) * coaches);
  int coach  = 0;
  for (coach = 0; coach < coaches; coach++) {
    read(pipes[coach][READ], &((*coaches_times)[coach]), sizeof(double));
  }
}




/* frees the memory of the allocated array for coach execution times */
void free_coaches_times(double** coaches_times, int coaches)
{
  free(*coaches_times);
  *coaches_times = NULL;
}




/* allocates the memory for the sorters coaches_time_execution_pipes times, and reads them through a pipe */
void get_sorters_times(double*** sorters_times, int** pipes, int coaches)
{
  *sorters_times = malloc(sizeof(double *) * coaches);
  int coach = 0;
  int sorters = 0;

  for (coach = 0; coach < coaches; coach++) {

    sorters = power(2, coach);
    (*sorters_times)[coach] = malloc(sizeof(double) * sorters);
    read(pipes[coach][READ], &((*sorters_times)[coach][0]), sorters * sizeof(double));

  }
}




/* frees the memory for the sorters execution times */
void free_sorters_times(double*** sorters_times, int coaches)
{
  int coach = 0;
  for (coach = 0; coach < coaches; coach++) {
    free((*sorters_times)[coach]);
  }
  free(*sorters_times);
  *sorters_times = NULL;
}




/* allocates memory to get the number of signals caught, which is provided by
   a pipe communicating through coordinator - coach */
void get_signals(int** signals, int** pipes, int coaches)
{
  *signals = malloc(sizeof(int) * coaches);
  int coach = 0;

  for (coach = 0; coach < coaches; coach++) {
    read(pipes[coach][READ], &((*signals)[coach]), sizeof(int));
  }
}




/* frees the memory that was allocated above */
void free_signals(int** signals, int coaches)
{
  free(*signals);
  *signals = NULL;
}





/* returns the min element of a doubles array */
double get_min(double* arr, int n)
{
  int i = 0;
  double min = arr[0];
  for (i = 1; i < n; i++) {
    if (arr[i] < min) {
      min = arr[i];
    }
  }

  return min;
}




/* returns the max element of a doubles array */
double get_max(double* arr, int n)
{
  int i = 0;
  double max = arr[0];
  for (i = 1; i < n; i++) {
    if (arr[i] > max) {
      max = arr[i];
    }
  }

  return max;
}




/* returns the average of a doubles array */
double get_average(double* arr, int n)
{
  int i = 0;
  double sum = 0.0;
  for (i = 0; i < n; i++) {
    sum += arr[i];
  }
  return ((double) sum) / n;
}




/* calculates base raised to the exp */
int power(int base, int exp)
{
  if (exp == 0) {
    return 1;
  } else if (exp == 1) {
    return base;
  } else if (exp % 2 == 0) {
    return power(base, exp / 2) * power(base, exp / 2);
  } else {
    return base * power(base, exp / 2) * power(base, exp / 2);
  }
}
