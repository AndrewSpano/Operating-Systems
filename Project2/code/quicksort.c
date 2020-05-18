#include <sys/times.h>
#include <signal.h>

#include "record.h"


/* Partition function. Thanks Introduction to Algorithms by Cormen for the nice explanation. */
int partition(Record* records, long low, long high, int attribute, int (*compare)(const Record *, const Record *, int))
{
  long left_index = low + 1;
  long right_index = high;

  while (1) {

    while(compare(&records[left_index], &records[low], attribute) < 0) {
      left_index++;
      if (left_index >= high) {
        break;
      }
    }

    while (compare(&records[right_index], &records[low], attribute) >= 0) {
      right_index--;
      if (right_index <= low) {
        break;
      }
    }

    if (left_index >= right_index) {
      break;
    }

    swap(&records[left_index], &records[right_index]);
  }

  swap(&records[low], &records[right_index]);

  return right_index;
}




/* Quick Sort function. No need to explain I believe. */
void quick_sort(Record* records, long low, long high, int attribute, int (*compare)(const Record *, const Record *, int))
{
  if (low >= high) {
    return;
  }

  long p = partition(records, low, high, attribute, compare);

  quick_sort(records, low, p - 1, attribute, compare);
  quick_sort(records, p + 1, high, attribute, compare);
}







int main(int argc, char* argv[])
{
  /* get the name of the file with the records, the attribute in which we want
     the records to be sorted, and the indices of the records that shall be
     sorted */
  char* inputfile = argv[1];
  int attribute = atoi(argv[2]);
  long left = atoi(argv[3]);
  long right = atoi(argv[4]);
  int pipe_fd = atoi(argv[5]);
  int pipe_for_time = atoi(argv[6]);
  pid_t coach_pid = atoi(argv[7]);


  struct tms tb1, tb2;
  double t1, t2, ticspersec;

  ticspersec = (double) sysconf(_SC_CLK_TCK);
  t1 = (double) times(&tb1);


  /* calculate how many records the sorting routine will sort */
  long records_to_sort = right - left + 1;

  /* open the records file */
  FILE* records_file = fopen (inputfile, "rb");
  if (records_file == NULL) {
     printf("\n\nCannot open binary file. Error in quicksort.c\n\n");
     exit(-2);
  }
  /* go to the start of the records we want to sort */
  fseek(records_file, left * sizeof(Record), 0);

  /* get the records we want to sort inside an array */
  long i = 0;
  Record* records = malloc(sizeof(Record) * records_to_sort);
  for (i = 0; i < records_to_sort; i++) {

    fread(&(records[i]), sizeof(Record), 1, records_file);

  }

  /* close the records file because we do not need it anymore */
  fclose(records_file);

  /* quick sort */
  quick_sort(records, 0, records_to_sort - 1, attribute, comparator);


  // for (i = 0; i < records_to_sort; i++) {
  //
  //   print_record(records[i]);
  //
  // }


  long records_sent = 0;

  /* write records to pipes */
  for (i = 0; i < records_to_sort; i += BATCH) {

    // printf("quicksort: write(): i = %d\n", i);
    if (records_sent + BATCH < records_to_sort) {
      write(pipe_fd, &records[i], BATCH * sizeof(Record));
    } else {
      write(pipe_fd, &records[i], (records_to_sort - records_sent) * sizeof(Record));
    }

  }


  free(records);



  /* calculate real time for sorter */
  t2 = (double) times(&tb2);

  double time = 0.0;

  /* measure real time */
  time = ((double) t2 - t1) / ticspersec;

  /* measure cpu_time */
  // time = ((double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime))) / ticspersec;

  write(pipe_for_time, &time, sizeof(double));

  /* send signal SIGUSR2 to the coach */
  kill(coach_pid, SIGUSR2);

  return 0;
}
