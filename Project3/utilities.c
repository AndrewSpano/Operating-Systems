#include "utilities.h"




inline void *MallocOrDie(size_t MemSize)
{
    void* AllocMem = malloc(MemSize);
    if(!AllocMem && MemSize)
    {
        printf("Could not allocate memory of size %lu\n", MemSize);
        perror("Malloc error");
        exit(-1);
    }
    return AllocMem;
}



inline void free_and_exit(shared_segment_t* shared_memory, int shmid, char** file_name)
{
  free_shared_segment_memory(shared_memory);
  free_shared_segment(shared_memory, shmid);
  free(*file_name);
  exit(EXIT_FAILURE);
}



/* function that takes a string (2nd parameter) with multiple sub-strings in it, and copies the n-th string (n is the 3rd parameter)
   in anoter string (1st parameter). Returns 1 if it manages to make the copy, else returns -1 if it fails for some reason. */
int get_nth_string(char* str, const char buf[], int n)
{
  int i = 0, j = 1;
  for (j = 1; j <= n - 1; j++) {

    while (i < MAX_BUFFER_SIZE && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\0') {
      i++;
    }

    if (i >= MAX_BUFFER_SIZE || (buf[i] != ' ' && buf[i] != '\t')) {
      return -1;
    }

    while (i < MAX_BUFFER_SIZE && (buf[i] == ' ' || buf[i] == '\t')) {
      i++;
    }

  }

  if (i >= MAX_BUFFER_SIZE || (buf[i] == '\n' || buf[i] == '\0')) {
    return -1;
  }

  int k = 0;
  while (i < MAX_BUFFER_SIZE && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\0') {
    str[k] = buf[i];
    i++;
    k++;
  }
  str[k] = '\0';

  return 1;
}






unsigned long waiting_time(struct tm start, struct tm end)
{
  const unsigned long seconds_per_year = 60 * 60 * 24 * 365;
  const unsigned long seconds_per_month = 60 * 60 * 24 * 30;
  const unsigned long seconds_per_day = 60 * 60 * 24;
  const unsigned long seconds_per_hour = 60 * 60;
  const unsigned long seconds_per_minute = 60;


  unsigned long end_time_seconds = end.tm_year * seconds_per_year + end.tm_mon * seconds_per_month + end.tm_mday * seconds_per_day
                                 + end.tm_hour * seconds_per_hour + end.tm_min * seconds_per_minute + end.tm_sec;

  unsigned long start_time_seconds = start.tm_year * seconds_per_year + start.tm_mon * seconds_per_month + start.tm_mday * seconds_per_day
                                   + start.tm_hour * seconds_per_hour + start.tm_min * seconds_per_minute + start.tm_sec;


  return end_time_seconds - start_time_seconds;
}





void print_time_from_seconds(unsigned long seconds, char* activity)
{
  const unsigned long seconds_per_hour = 60 * 60;
  const unsigned long seconds_per_minute = 60;

  int hours = seconds / seconds_per_hour;
  int mins = (seconds % seconds_per_hour) / seconds_per_minute;
  int secs = (seconds - hours * seconds_per_hour - mins * seconds_per_minute);

  printf("Average wait time to %s is: %d hours, %d mins, %d seconds.\n", activity, hours, mins, secs);
}
