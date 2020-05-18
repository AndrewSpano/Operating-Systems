#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comptroller_funcs.h"
#include "bus_funcs.h"

char* my_destinations[] = {"ASK", "PEL", "VOR", NULL};

int get_comptroller_input(char* argv[], double* time, double* stattimes, key_t* shmid_key)
{
  uint8_t flag_d = 0;
  uint8_t flag_t = 0;
  uint8_t flag_s = 0;

  uint8_t error_input = 0;

  int i = 1;
  for (; i < COMP_ARGS; i += 2)
  {

    if (!strcmp(argv[i], "-d"))
    {

      if (!flag_d) {
        flag_d = 1;
      } else {
        printf("Error, parameter -d is given more than 1 time in comptroller.c. Exiting..\n");
        error_input = 1;
        break;
      }

      char* endptr = NULL;
      double d = strtod(argv[i + 1], &endptr);
      if (d <= 0) {
        printf("Wrong value for -d parameter in comptroller.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      }

      *time = d;

    }
    else if (!strcmp(argv[i], "-t"))
    {

      if (!flag_t) {
        flag_t = 1;
      } else {
        printf("Error, parameter -t is given more than 1 time in comptroller.c. Exiting..\n");
        error_input = 1;
        break;
      }

      char* endptr = NULL;
      double t = strtod(argv[i + 1], &endptr);
      if (t <= 0) {
        printf("Wrong value for -t parameter in comptroller.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      }

      *stattimes = t;

    }
    else if (!strcmp(argv[i], "-s"))
    {

      if (!flag_s) {
        flag_s = 1;
      } else {
        printf("Error, parameter -s is given more than 1 time in comptroller.c. Exiting..\n");
        error_input = 1;
        break;
      }

      key_t s = atoi(argv[i + 1]);
      if (s <= 0) {
        printf("Wrong value for -s parameter in comptroller.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      }

      *shmid_key = s;

    }
    else {

      printf("No matching flags for this input: \"%s\" in comptroller.c. Available flags are -d, -t, -s.\n", argv[i]);
      printf("Error in comptroller_funcs.c::get_comptroller_input()\n");
      error_input = 1;
      break;

    }

  }

  return !error_input;
}








void print_state(shared_segment_t* shared_memory)
{
  printf("The state of the station is:\n\n\n");
  uint16_t total_parked_buses = 0;
  int bay = 0;
  for (; bay < BAYS; bay++)
  {

    sem_wait(&(shared_memory->bays_mutex[bay]));

    if (shared_memory->ledger.bays[bay].spots_taken == 0)
    {
      sem_post(&(shared_memory->bays_mutex[bay]));
      continue;
    }

    printf("Buses parked in bay %s are:\n\n", my_destinations[bay]);

    int spot = 0;
    for (; spot < SPOTS; spot++)
    {
      if (shared_memory->ledger.bays[bay].spots[spot].has_bus)
      {
        print_bus_info(&(shared_memory->ledger.bays[bay].spots[spot].current_bus), bay, spot);
        total_parked_buses++;
        printf("\n");
      }
    }

    printf("In bay %s, %d spots are taken out of the %d total spots.\n", my_destinations[bay], shared_memory->ledger.bays[bay].spots_taken, shared_memory->ledger.bays[bay].total_spots);
    printf("Therefore, there are %d available spots in bay %s for other buses to park.", shared_memory->ledger.bays[bay].total_spots - shared_memory->ledger.bays[bay].spots_taken, my_destinations[bay]);

    printf("\n\n");

    printf("So far, %d passengers have been dropped in bay %s.\n", shared_memory->ledger.bays[bay].total_passengers_dropped, my_destinations[bay]);
    printf("So far, %d passengers have been picked up from bay %s.\n", shared_memory->ledger.bays[bay].total_passengers_picked, my_destinations[bay]);

    printf("\n\n");

    sem_post(&(shared_memory->bays_mutex[bay]));

  }


  printf("In total, there are %d parked buses occupying the %d spots that the station has.\n", total_parked_buses, BAYS * SPOTS);
  printf("Therefore, the are %d availabe spots in the station for other buses to park.\n\n", BAYS * SPOTS - total_parked_buses);



  sem_wait(&(shared_memory->traffic_mutex));

  printf("Right now, %d buses outside the station are waiting to enter.\n", shared_memory->ledger.waiting_to_enter);
  printf("Also, %d buses have finished boarding passengers, and are requesting access to exit the station.\n\n\n", shared_memory->ledger.requesting_access_to_exit);

  sem_post(&(shared_memory->traffic_mutex));

}







void print_statistics(shared_segment_t* shared_memory)
{
  /* critical section of the statistics */
  sem_wait(&(shared_memory->statistics_mutex));



  printf("The statistics of the station so far, are:\n\n");

  if (shared_memory->ledger.counter_waiting_to_park_times != 0)
  {
    unsigned long average_waiting_time_to_park = shared_memory->ledger.sum_of_waiting_to_park_times / shared_memory->ledger.counter_waiting_to_park_times;
    print_time_from_seconds(average_waiting_time_to_park, "park");
  }
  else
  {
    printf("No buses have parked in the station so far in order to calculate an average waiting time for parking.\n");
  }


  if (shared_memory->ledger.counter_waiting_to_leave_times != 0)
  {
    unsigned long average_waiting_time_to_leave = shared_memory->ledger.sum_of_waiting_to_leave_times / shared_memory->ledger.counter_waiting_to_leave_times;
    print_time_from_seconds(average_waiting_time_to_leave, "depart");
  }
  else
  {
    printf("No buses have departed the station so far in order to calculate an average waiting time for the departure.\n");
  }

  printf("\n\n");

  sem_post(&(shared_memory->statistics_mutex));
  /* end of critical section of the statistics */
}
