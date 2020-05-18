#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "record.h"
#include "handler.h"


/* global variable used to count the number of SIGUSR2 signals that have arrived */
/* declare it volatile because we want the system to be able to change it */
volatile sig_atomic_t signals_arrived = 0;


void get_bounds(long total_records, long** left, long** right, int coach_number, int sorters);
void free_bounds(long** left, long** right);

void create_pipes(int*** pipes, int coach, int sorters);
void free_pipes(int*** pipes, int sorters);

void allocate_records_arrays(Record*** records_from_sorters, int sorters, long* left, long* right);
void free_records_arrays(Record*** records_from_sorters, int sorters);


void read_records_from_pipes(Record** records_from_sorters, int** sorters_pipes, long total_records, long* left, long* right, int sorters);
void merge_and_print(long total_records, Record** records_from_sorters, int sorters, long* left, long* right, char* attr);

int power(int base, int exp);



int main(int argc, char* argv[])
{

  if (argc != 9) {
    printf("Wrong amount of parameters passed in coach process.\n");
    exit(-1);
  }

  /* store the arguments in variables to make the code more readable */
  int coach_number = atoi(argv[1]);
  char* records_number = argv[2];
  char* inputfile = argv[3];
  char* sort = argv[4];
  char* attribute = argv[5];
  int father_pipe = atoi(argv[6]);
  char* children_pipes = argv[7];
  int signal_pipe = atoi(argv[8]);

  /* number of sorters that the coach will make */
  int sorters = power(2, coach_number);
  long total_records = atoi(records_number);


  /* struct defined in <sys/times.h> used to measure execution time */
  struct tms tb1, tb2;
  double t1, t2, ticspersec;

  /* start counting execution time */
  ticspersec = (double) sysconf(_SC_CLK_TCK);
  t1 = (double) times(&tb1);


  /* struct defined on <signals.h> */
  struct sigaction signal_action;
  /* select the function that will be executed when a signal is caught */
  signal_action.sa_handler = signal_handler;
  signal_action.sa_flags = SA_RESTART;

  /* block every signal when running the signal_handler function */
  sigfillset(&signal_action.sa_mask);

  if (sigaction(SIGUSR2, &signal_action, NULL) == -1) {
    printf("Error in coach.c:: sigaction()\n");
    exit(-1);
  }


  /* pipes used to transfer records from sorters to coaches */
  int** sorters_pipes = NULL;
  create_pipes(&sorters_pipes, 1, sorters);

  /* matrices that indicate the bounds (which records) every sorter will sort */
  long* left = NULL;
  long* right = NULL;
  get_bounds(total_records, &left, &right, coach_number, sorters);


  /* create child processes -> sorters */
  pid_t child_pid;
  int i = 0;
  for (i = 0; i < sorters; i++) {

    child_pid = fork();
    if (child_pid < 0) {

      printf("\nSomething went wrong when creating the sorter %d in coach %d in coach.c\n\n", i + 1, coach_number);
      exit(-10);

    } else if (child_pid == 0) {

      /* get the left bound for the i-th sorter as a string */
      char* left_as_string = malloc(sizeof(char) * (MAX_LONG_DIGITS + 1));
      sprintf(left_as_string, "%ld", left[i]);

      /* get the right bound for the i-th sorter as a string */
      char* right_as_string = malloc(sizeof(char) * (MAX_LONG_DIGITS + 1));
      sprintf(right_as_string, "%ld", right[i]);

      /* get its records pipe (which will be used to send records) as a string */
      char* sorters_pipes_as_string = malloc(sizeof(char) * (MAX_INT_DIGITS + 1));
      sprintf(sorters_pipes_as_string, "%d", sorters_pipes[i][1]);

      /* get the pid of the coach so that the sorter can send a SIGUSR2 to it */
      char* coach_pid = malloc(sizeof(char) * (MAX_INT_DIGITS + 1));
      sprintf(coach_pid, "%d", getppid());


      /* invoke child process -> sorter */
      char *args[] = {"./sorter", inputfile, records_number, left_as_string, right_as_string, sort, attribute, sorters_pipes_as_string, coach_pid, children_pipes, NULL};
      execvp(args[0], args);

    }

  }

  /* array of sorted records that the sorters will send through a pipe */
  Record** records_from_sorters;
  allocate_records_arrays(&records_from_sorters, sorters, left, right);

  /* read the sorted records from the pipes and place them in the arrays we just created */
  read_records_from_pipes(records_from_sorters, sorters_pipes, total_records, left, right, sorters);
  /* merge them in a single output file */
  merge_and_print(total_records, records_from_sorters, sorters, left, right, attribute);


  /* free the allocated memory */
  free_bounds(&left, &right);
  free_records_arrays(&records_from_sorters, sorters);
  free_pipes(&sorters_pipes, sorters);


  int returnStatus;
  while (wait(&returnStatus) > 0);

  /* get the number of signals that arrived in the coach process as an int */
  int signals_received = signals_arrived;
  /* write that amount in it's corresponding pipe so that the parent process
     (coordinator) can read it */
  write(signal_pipe, &signals_received, sizeof(int));


  /* calculate the execution time for the coach. Note that it may be kinda big,
     because it wastes time waiting to read the records from the pipes */
  t2 = (double) times(&tb2);
  double cpu_time = ((tb2.tms_utime + tb2.tms_stime) -(tb1.tms_utime + tb1.tms_stime)) / ticspersec;

  /* write the execution time in the corresponding pipe */
  write(father_pipe, &cpu_time, sizeof(double));

  return 0;
}




/* get the bounds in which every sorter will do the sorting */
void get_bounds(long total_records, long** left, long** right, int coach_number, int sorters)
{
  *left = malloc(sizeof(long) * sorters);
  *right = malloc(sizeof(long) * sorters);

  switch (coach_number) {

    case 0: {

      (*left)[0] = 0;
      (*right)[0] = total_records - 1;

      break;
    }

    case 1: {

      (*left)[0] = 0;
      (*right)[0] = total_records / 2 - 1;

      (*left)[1] = total_records / 2;
      (*right)[1] = total_records - 1;

      break;
    }

    case 2: {

      (*left)[0] = 0;
      (*right)[0] = total_records / 8 - 1;

      (*left)[1] = total_records / 8;
      (*right)[1] = 2 * total_records / 8 - 1;

      (*left)[2] = 2 * total_records / 8;
      (*right)[2] = 4 * total_records / 8 - 1;

      (*left)[3] = 4 * total_records / 8;
      (*right)[3] = total_records - 1;

      break;
    }

    case 3: {

      (*left)[0] = 0;
      (*right)[0] = total_records / 16 - 1;

      (*left)[1] = total_records / 16;
      (*right)[1] = 2 * total_records / 16 - 1;

      (*left)[2] = 2 * total_records / 16;
      (*right)[2] = 3 * total_records / 16 - 1;

      (*left)[3] = 3 * total_records / 16;
      (*right)[3] = 4 * total_records / 16 - 1;

      (*left)[4] = 4 * total_records / 16;
      (*right)[4] = 6 * total_records / 16 - 1;

      (*left)[5] = 6 * total_records / 16;
      (*right)[5] = 8 * total_records / 16 - 1;

      (*left)[6] = 8 * total_records / 16;
      (*right)[6] = 12 * total_records / 16 - 1;

      (*left)[7] = 12 * total_records / 16;
      (*right)[7] = total_records - 1;

      break;
    }

    default: {
      printf("skata giati eftase edw gamw: coach.c::get_bounds()\n");
    }

  }
}




/* free the bounds used by sorters */
void free_bounds(long** left, long** right)
{
  free(*left);
  free(*right);
}




/* create sorters amount of pipes */
void create_pipes(int*** pipes, int coach, int sorters)
{
  int i = 0, j = 0;
  *pipes = malloc(sizeof(int *) * sorters);
  for (i = 0; i < sorters; i++) {

    (*pipes)[i] = malloc(sizeof(int) * 2);

    if (pipe((*pipes)[i]) < 0) {
      for (j = 0; j <= i; j++) {
        free((*pipes)[j]);
      }
      free(*pipes);
      printf("Error in creating a pipe in coach %d for sorter %d.\n", coach, i);
      exit(1);
    }

  }
}




/* free the pipes */
void free_pipes(int*** pipes, int sorters)
{
  int i = 0;
  for (i = 0; i < sorters; i++) {
    free((*pipes)[i]);
  }
  free(*pipes);
}




/* allocate the arrays that will be used to get the sorted records from the sorters pipes */
void allocate_records_arrays(Record*** records_from_sorters, int sorters, long *left, long *right)
{
  *records_from_sorters = malloc(sizeof(Record *) * sorters);
  int i = 0;
  for (i = 0; i < sorters; i++) {
    (*records_from_sorters)[i] = malloc(sizeof(Record) * (right[i] - left[i] + 1));
  }
}




/* free the memory that was allocated for the records arrays */
void free_records_arrays(Record*** records_from_sorters, int sorters)
{
  int i = 0;
  for (i = 0; i < sorters; i++) {
    free((*records_from_sorters)[i]);
  }
  free(*records_from_sorters);
}




/* reads records from the sorters pipes and places them in the records arrays */
void read_records_from_pipes(Record** records_from_sorters, int** sorters_pipes, long total_records, long* left, long* right, int sorters)
{
  /* keep track of how many records a specific sorter has sent to an array */
  int* count = malloc(sizeof(int) * sorters);
  memset(count, 0, sizeof(int) * sorters);

  /* the max number of records that a sorter can transfer */
  int* max_count = malloc(sizeof(int) * sorters);
  int i = 0, j = 0;
  for (i = 0; i < sorters; i++) {
    max_count[i] = right[i] - left[i] + 1;
  }

  /* get the max number of records that can be transfered by a sorter */
  int max = max_count[0];
  for (i = 1; i < sorters; i++) {
    if (max_count[i] > max) {
      max = max_count[i];
    }
  }



  /* get the records in the records_from_sorters array */
  for (i = 0; i < max; i += BATCH) {

    // printf("in read: i = %d\n", i);

    /* read records from every sorter */
    for (j = 0; j < sorters; j++) {

      if (count[j] + BATCH < max_count[j]) {
        read(sorters_pipes[j][READ], &(records_from_sorters[j][i]), BATCH * sizeof(Record));
        count[j] += BATCH;
      } else if (count[j] < max_count[j]) {
        read(sorters_pipes[j][READ], &(records_from_sorters[j][i]), (max_count[j] - count[j]) * sizeof(Record));
        count[j] += max_count[j] - count[j];
      }

    }

  }


  free(max_count);
  free(count);
}




/* merge all the records from the arrays in the output file */
void merge_and_print(long total_records, Record** records_from_sorters, int sorters, long* left, long* right, char* attr)
{
  /* keep track of how many records a specific sorter has transfered */
  int* count = malloc(sizeof(int) * sorters);
  memset(count, 0, sizeof(int) * sorters);

  /* the max number of records that a sorter can transfer */
  int* max_count = malloc(sizeof(int) * sorters);
  int i = 0;
  for (i = 0; i < sorters; i++) {
    max_count[i] = right[i] - left[i] + 1;
  }

  /* open the outfile where the records will be printed in */
  char* outfile_name = malloc(sizeof(sizeof(char) * MAX_FILE_NAME_LENGTH));
  strcpy(outfile_name, "myinputfile.");
  strcat(outfile_name, attr);

  FILE* outfile = fopen(outfile_name, "w");
  if (outfile == NULL) {
    printf("There was a problem opening the outfile: %s\n", outfile_name);
    exit(-5);
  }

  int attribute = atoi(attr);

  int sorter_with_min_record = 0;
  int j = 0;

  for (i = 0; i < total_records; i++) {

    /* get the index of the first sorter who has available records to merge */
    sorter_with_min_record = 0;
    while (sorter_with_min_record < sorters && count[sorter_with_min_record] == max_count[sorter_with_min_record]) {
      sorter_with_min_record++;
    }

    if (sorter_with_min_record >= sorters) {
      printf("Should have never come here in coach.c:: merge()\n");
      exit(-1);
    }

    /* get index with sorter who has the next min record available */
    for (j = sorter_with_min_record; j < sorters; j++) {
      if (count[j] < max_count[j] && comparator(&(records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]]),
                                                &(records_from_sorters[j][count[j]]), attribute) > 0) {

        sorter_with_min_record = j;

      }
    }

    /* copy in the file the min record */
    fprintf(outfile, "%ld %s %s  %s %d %s %s %-9.2f\n", records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].id,
                                                        records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].surname,
                                                        records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].name,
                                                        records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].street_name,
                                                        records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].house_id,
                                                        records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].city,
                                                        records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].postcode,
                                                        records_from_sorters[sorter_with_min_record][count[sorter_with_min_record]].salary);
    count[sorter_with_min_record]++;

  }

  fclose(outfile);
  free(outfile_name);
  free(max_count);
  free(count);
}




/* function that returns: base raised to the exp */
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
