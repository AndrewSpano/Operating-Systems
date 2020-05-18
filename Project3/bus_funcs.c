#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared_memory.h"
#include "bus_funcs.h"

char* destinations[] = {"ASK", "PEL", "VOR", NULL};


int get_bus_input(char* argv[], Bus* bus, key_t* shmid_key)
{
  uint8_t flag_t = 0;
  uint8_t flag_n = 0;
  uint8_t flag_c = 0;
  uint8_t flag_p = 0;
  uint8_t flag_m = 0;
  uint8_t flag_i = 0;
  uint8_t flag_s = 0;

  uint8_t error_input = 0;

  int i = 1;
  for (; i < BUS_ARGS; i += 2)
  {

    if (!strcmp(argv[i], "-t"))
    {

      if (!flag_t) {
        flag_t = 1;
      }
      else {
        printf("Error, parameter -t is given more than 1 time in bus.c. Exiting..\n");
        error_input = 1;
        break;
      }


      if (!strcmp(argv[i + 1], "ASK")) {
        bus->type = ASK;
      }
      else if (!strcmp(argv[i + 1], "PEL")) {
        bus->type = PEL;
      }
      else if (!strcmp(argv[i + 1], "VOR")) {
        bus->type = VOR;
      }
      else {
        printf("Wrong value given for bus type. Correct values are: -t ASK / PEL / VOR\n");
        error_input = 1;
        break;
      }

    }
    else if (!strcmp(argv[i], "-n"))
    {

      if (!flag_n) {
        flag_n = 1;
      } else {
        printf("Error, parameter -n is given more than 1 time in bus.c. Exiting..\n");
        error_input = 1;
        break;
      }

      int n = atoi(argv[i + 1]);
      if (n < 0) {
        printf("Wrong value given for -n parameter in bus.c, it should be >= 0. Exiting..\n");
        error_input = 1;
        break;
      } else if (flag_c && bus->capacity < n) {
        printf("Wrong value for -c and -n parameters. Number of current passengers (%d) can't exceed the capacity (%d) of the bus.\n", n, bus->capacity);
        error_input = 1;
        break;
      }

      bus->on_board_passengers = n;

    }
    else if (!strcmp(argv[i], "-c"))
    {

      if (!flag_c) {
        flag_c = 1;
      } else {
        printf("Error, parameter -c is given more than 1 time in bus.c. Exiting..\n");
        error_input = 1;
        break;
      }

      int c = atoi(argv[i + 1]);
      if (c <= 0) {
        printf("Wrong value given for -c parameter in bus.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      } else if (flag_n && bus->on_board_passengers > c) {
        printf("Wrong value for -c and -n parameters. Number of current passengers (%d) can't exceed the capacity (%d) of the bus.\n", bus->on_board_passengers, c);
        error_input = 1;
        break;
      }

      bus->capacity = c;

    }
    else if (!strcmp(argv[i], "-p"))
    {

      if (!flag_p) {
        flag_p = 1;
      } else {
        printf("Error, parameter -p is given more than 1 time in bus.c. Exiting..\n");
        error_input = 1;
        break;
      }

      char* endptr = NULL;
      double p = strtod(argv[i + 1], &endptr);
      if (p <= 0) {
        printf("Wrong value for -p parameter in bus.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      }

      bus->parkperiod = p;

    }
    else if (!strcmp(argv[i], "-m"))
    {

      if (!flag_m) {
        flag_m = 1;
      } else {
        printf("Error, parameter -m is given more than 1 time in bus.c. Exiting..\n");
        error_input = 1;
        break;
      }

      char* endptr = NULL;
      double m = strtod(argv[i + 1], &endptr);
      if (m <= 0) {
        printf("Wrong value for -m parameter in bus.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      }

      bus->mantime = m;

    }
    else if (!strcmp(argv[i], "-i"))
    {

      if (!flag_i) {
        flag_i = 1;
      } else {
        printf("Error, parameter -i is given more than 1 time in bus.c. Exiting..\n");
        error_input = 1;
        break;
      }

      strcpy(bus->id, argv[i + 1]);

    }
    else if (!strcmp(argv[i], "-s"))
    {

      if (!flag_s) {
        flag_s = 1;
      } else {
        printf("Error, parameter -s is given more than 1 time in bus.c. Exiting..\n");
        error_input = 1;
        break;
      }

      key_t s = atoi(argv[i + 1]);
      if (s <= 0) {
        printf("Wrong value for -s parameter in bus.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      }

      *shmid_key = s;

    }
    else {

      printf("No matching flags for this input: \"%s\" in bus.c. Available flags are -t, -n, -c, -p, -m, -s, -i.\n", argv[i]);
      printf("Error in bus_funcs.c::get_bus_input()\n");
      error_input = 1;
      break;

    }

  }


  return !error_input;
}





void bus_init(Bus* bus)
{
  time_t arrival_time = time(NULL);
  bus->arrival_time = *localtime(&arrival_time);
}




void park_to_spot(Bus* bus, shared_segment_t* shared_memory, Destination dest, uint8_t spot)
{

  /* copy the bus information to the ledger so that the comptroller can see it */
  memcpy(&(shared_memory->ledger.bays[dest].spots[spot].current_bus), bus, sizeof(Bus));
  /* mark the spot as not availabe because the bus parked in it */
  shared_memory->ledger.bays[dest].spots[spot].has_bus = TRUE;


  /* increment the number of parked buses in the specific bay */
  shared_memory->ledger.bays[dest].spots_taken++;

}



void Park(Bus* bus, shared_segment_t* shared_memory, Destination dest, uint8_t* spot)
{

  // remove this if later
  if (shared_memory->ledger.bays[dest].spots_taken == shared_memory->ledger.bays[dest].total_spots)
  {
    printf("SHOULD HAVE NEVER PRINTED THIS. ERROR IN bus_funcs::Park()\n");
    printf("dest = %d\n", dest);
    printf("spots_taken = %d, total_spots =  %d\n", shared_memory->ledger.bays[dest].spots_taken,
                                                    shared_memory->ledger.bays[dest].total_spots);
    printf("shared_memory->ledger.requesting_access_to_enter = %d && shared_memory->ledger.is_ready_to_enter = %d\n",
  shared_memory->ledger.requesting_access_to_enter, shared_memory->ledger.is_ready_to_enter);
    free(bus);
    exit(EXIT_FAILURE);
  }


  /* find the spot that the bus will park */
  int temp_spot = 0;
  for (; temp_spot < SPOTS; temp_spot++)
  {
    if (shared_memory->ledger.bays[dest].spots[temp_spot].has_bus == FALSE)
    {

      sem_wait(&(shared_memory->bays_mutex[dest]));
      /* critical section of the destination bay */


      /* spot has been found, so park the bus, save the spot and return */
      park_to_spot(bus, shared_memory, dest, temp_spot);
      *spot = temp_spot;

      /* sleep while the manuever happens */
      sleep((int) bus->mantime);

      /* get the park time and save it in the bus struct */
      time_t park_time = time(NULL);
      bus->park_time = *localtime(&park_time);



      /* end of critical section of the destination bay */
      sem_post(&(shared_memory->bays_mutex[dest]));

      return;
    }
  }
}





int Drop_Passenger(Bus* bus, shared_segment_t* shared_memory, Destination islet, uint8_t spot)
{
  /* define a random amount of passengers to drop */
  uint16_t random_drop = rand() % (bus->on_board_passengers + 1);

  /* decrease the amount of on board passengers in the bus* variable which has
     scope inside the bus process */
  bus->on_board_passengers -= random_drop;
  /* do the same for the shared segment variable which is not restricted by
     any scope, and therefore the compotroller can access it */
  shared_memory->ledger.bays[islet].spots[spot].current_bus.on_board_passengers -= random_drop;



  sem_wait(&(shared_memory->bays_mutex[islet]));
  /* critical section for the bay */
  shared_memory->ledger.bays[islet].total_passengers_dropped += random_drop;
  /* end of critical section for the bay */
  sem_post(&(shared_memory->bays_mutex[islet]));



  return random_drop;

}




void Wait_Parked(Bus* bus)
{
  uint8_t prob = (uint8_t) bus->parkperiod + 1;
  uint8_t random_sleep_time = rand() % prob;
  sleep(random_sleep_time);
}




int Pick_Passengers(Bus* bus, shared_segment_t* shared_memory, Destination islet, uint8_t spot)
{
  /* define a random amount of passengers to pick up */
  uint16_t random_pickup = rand() % (bus->capacity - bus->on_board_passengers + 1);

  /* increase the amount of on board passengers in the bus* variable which has
     scope inside the bus process */
  bus->on_board_passengers += random_pickup;
  /* do the same for the shared segment variable which is not restricted by
     any scope, and therefore the compotroller can access it */
  shared_memory->ledger.bays[islet].spots[spot].current_bus.on_board_passengers += random_pickup;


  sem_wait(&(shared_memory->bays_mutex[islet]));
  /* critical section for the bay */
  shared_memory->ledger.bays[islet].total_passengers_picked += random_pickup;
  /* end of critical section for the bay */
  sem_post(&(shared_memory->bays_mutex[islet]));



  return random_pickup;
}




void Leave(Bus* bus, shared_segment_t* shared_memory, Destination islet, uint8_t spot)
{

  sem_wait(&(shared_memory->bays_mutex[islet]));
  /* critical section of the current islet (bay) */



  /* sleep while the manuever happens */
  sleep((int) bus->mantime);

  /* delete the bus information */
  memset(&(shared_memory->ledger.bays[islet].spots[spot].current_bus), 0, sizeof(Bus));
  /* mark the spot that the bus had, as availabe */
  shared_memory->ledger.bays[islet].spots[spot].has_bus = FALSE;
  shared_memory->ledger.bays[islet].spots_taken--;



  /* end of critical section of the current islet (bay) */
  sem_post(&(shared_memory->bays_mutex[islet]));


  /* get the departure time and save it in the bus struct */
  time_t departure_time = time(NULL);
  bus->departure_time = *localtime(&departure_time);

}





/* pretty much self explanatory */
void write_status_to_logfile(shared_segment_t* shared_memory, Bus* bus, int bay, int status)
{
  FILE* logfile = NULL;

  logfile = fopen(shared_memory->logfile_name, "a");

  if (status == ARRIVE)
  {
    fprintf(logfile, "Bus with id %s arrived at: %d:%d:%d.\n", bus->id, (bus->arrival_time).tm_hour, (bus->arrival_time).tm_min, (bus->arrival_time).tm_sec);
  }
  else if (status == PARK)
  {
    fprintf(logfile, "Bus with id %s parked at bay %s at: %d:%d:%d.\n", bus->id, destinations[bay], (bus->park_time).tm_hour, (bus->park_time).tm_min, (bus->park_time).tm_sec);
  }
  else
  {
    fprintf(logfile, "Bus with id %s departed from bay %s at: %d:%d:%d.\n", bus->id, destinations[bay], (bus->departure_time).tm_hour, (bus->departure_time).tm_min, (bus->departure_time).tm_sec);
  }

  fclose(logfile);
}




/* pretty much self explanatory */
void write_activity_to_logfile(shared_segment_t* shared_memory, Bus* bus, int activity, int amount, int bay)
{
  FILE* logfile = NULL;

  logfile = fopen(shared_memory->logfile_name, "a");

  if (activity == DROP)
  {
    fprintf(logfile, "Bus with id %s dropped %d passengers at bay %s.\n", bus->id, amount, destinations[bay]);
  }
  else
  {
    fprintf(logfile, "Bus with id %s picked up %d passengers from bay %s.\n", bus->id, amount, destinations[bay]);
  }

  fclose(logfile);
}




void update_statistics(shared_segment_t* shared_memory, Bus* bus)
{
  unsigned long waiting_to_park_time = waiting_time(bus->arrival_time, bus->park_time);
  shared_memory->ledger.sum_of_waiting_to_park_times += waiting_to_park_time;
  shared_memory->ledger.counter_waiting_to_park_times++;

  unsigned long waiting_to_leave_time = waiting_time(bus->park_time, bus->departure_time);
  shared_memory->ledger.sum_of_waiting_to_leave_times += waiting_to_leave_time;
  shared_memory->ledger.counter_waiting_to_leave_times++;
}




/* pretty much self explanatory */
void print_bus_info(Bus* bus, int bay, int spot)
{

  if (bus->type == ASK)
  {
    printf("Bus type: ASK,");
  }
  else if (bus->type == PEL)
  {
    printf("Bus type: PEL,");
  }
  else
  {
    printf("Bus type: VOR,");
  }
  printf(" parked in bay %s, in spot %d\n", destinations[bay], spot + 1);

  printf("on_board_passengers: %d\n", bus->on_board_passengers);
  printf("capacity: %d\n", bus->capacity);
  printf("parkperiod: %lf\n", bus->parkperiod);
  printf("mantime: %lf\n", bus->mantime);
  printf("id: %s\n", bus->id);
}
