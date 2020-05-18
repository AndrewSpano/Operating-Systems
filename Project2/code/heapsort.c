#include <sys/times.h>
#include <signal.h>

#include "record.h"



/* Heapify function used to "transform" the records array into a heap array. */
void heapify(Record* records, long size, long index, int attribute, int (*compare)(const Record *, const Record *, int))
{
  long largest = index;
  long left_child = 2 * index + 1;
  long right_child = 2 * index + 2;


  if (left_child < size && compare(&records[left_child], &records[largest], attribute) > 0) {
    largest = left_child;
  }


  if (right_child < size && compare(&records[right_child], &records[largest], attribute) > 0) {
    largest = right_child;
  }


  if (largest != index) {

    swap(&records[index], &records[largest]);
    heapify(records, size, largest, attribute, compare);

  }
}



/* Heap Sort function. Big thanks to Introduction to Algorithms by Cormen. */
void heap_sort(Record* records, long size, int attribute, int (*compare)(const Record *, const Record *, int))
{
  long i = 0;
  for (i = size / 2 - 1; i >= 0; i--) {
    heapify(records, size, i, attribute, compare);
  }

  for (i = size - 1; i >= 0; i--) {

    swap(&records[0], &records[i]);
    heapify(records, i, 0, attribute, compare);

  }

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
     printf("\n\nCannot open binary file. Error in heapsort.c\n\n");
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

  /* heap sort */
  heap_sort(records, records_to_sort, attribute, comparator);


  // for (i = 0; i < records_to_sort; i++) {
  //
  //   print_record(records[i]);
  //
  // }


  long records_sent = 0;

  /* write records to pipes */
  for (i = 0; i < records_to_sort; i += BATCH) {

    // printf("heapsort: write(): i = %ld\n", i);
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

  // /* measure cpu_time */
  // time = ((double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime))) / ticspersec;

  write(pipe_for_time, &time, sizeof(double));

  /* send signal SIGUSR2 to the coach */
  kill(coach_pid, SIGUSR2);

  return 0;
}
