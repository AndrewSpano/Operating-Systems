#include "shared_memory.h"
#include "bus_funcs.h"



int main(int argc, char* argv[])
{

  // printf("\n");
  if (argc != BUS_ARGS)
  {
    printf("Wrong command line parameters given in bus.c. The input should be something like:\n\n");
    printf("./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -i bus_id -s shmid\n\n");
    exit(EXIT_FAILURE);
  }

  Bus* bus = NULL;
  bus = malloc(sizeof(Bus));
  key_t shmid_key;

  if (!get_bus_input(argv, bus, &shmid_key))
  {
    printf("Wrong values for command line parameters in bus.c\n\n");
    free(bus);
    exit(EXIT_FAILURE);
  }

  bus_init(bus);

  Destination islet;
  uint8_t spot;



  int shmid = 0;
  shared_segment_t* shared_memory = NULL;
  if (!get_shared_segment(shmid_key, &shmid, &shared_memory))
  {
    perror("Error in bus.c::get_shared_segment()");
    exit(-1);
  }







  /* write to logfile */
  sem_wait(&(shared_memory->logfile_mutex));

  write_status_to_logfile(shared_memory, bus, -1, ARRIVE);

  sem_post(&(shared_memory->logfile_mutex));



  sem_wait(&(shared_memory->traffic_mutex));
  /* start of "traffic critical section" */

  /* a new bus is waiting to enter, so increment the counter of waiting buses */
  shared_memory->ledger.waiting_to_enter++;

  /* end of the traffic critical section */
  sem_post(&(shared_memory->traffic_mutex));




  /* wait in queue, while some other bus is entering */
  sem_wait(&(shared_memory->entry_queue));



  /* bus wants to enter, save its type temporarily so we the station manager
     can decide where it will go */
  shared_memory->ledger.entering_bus_type = bus->type;



  sem_wait(&(shared_memory->traffic_mutex));
  /* start of "traffic critical section" */

  /* a new bus is trying to enter, so increment the counter for the enter traffic */
  shared_memory->ledger.requesting_access_to_enter++;

  /* end of the traffic critical section */
  sem_post(&(shared_memory->traffic_mutex));



  /* used to communicate with station manager to tell him that the bus is
     ready to enter to station. Now we just wait for him to find a spot */
  shared_memory->ledger.is_ready_to_enter = TRUE;



  /* notify the station manager that a bus wants to move inside the station */
  sem_post(&(shared_memory->station_manager_mutex));



  /* wait for the permission of station manager, for a park spot to be availabe */
  sem_wait(&(shared_memory->park_mutex));



  /* get the exact location that the bus has to park */
  islet = shared_memory->ledger.islet_for_entering_bus;



  /* park the bus to a specific islet, and store the spot that it gets assigned
     to the variable uint8_t spot */
  Park(bus, shared_memory, islet, &spot);



  /* flag used to tell to the station manager that the bus has parked, so that
     the next bus can proceed to park */
  shared_memory->ledger.has_parked = TRUE;


  /* write to logfile */
  sem_wait(&(shared_memory->logfile_mutex));

  write_status_to_logfile(shared_memory, bus, islet, PARK);

  sem_post(&(shared_memory->logfile_mutex));


  /* notify the station manager that the bus parked */
  sem_post(&(shared_memory->station_manager_mutex));



  /* drop passengers */
  int drop = Drop_Passenger(bus, shared_memory, islet, spot);
  /* update logfile */
  sem_wait(&(shared_memory->logfile_mutex));
  write_activity_to_logfile(shared_memory, bus, DROP, drop, islet);
  sem_post(&(shared_memory->logfile_mutex));


  /* wait while being parked */
  Wait_Parked(bus);

  /* pick up passengers */
  int pickup = Pick_Passengers(bus, shared_memory, islet, spot);
  /* update logfile */
  sem_wait(&(shared_memory->logfile_mutex));
  write_activity_to_logfile(shared_memory, bus, PICKUP, pickup, islet);
  sem_post(&(shared_memory->logfile_mutex));




  sem_wait(&(shared_memory->traffic_mutex));
  /* start of "traffic critical section" */

  /* a new bus is trying to exit, so increment the counter */
  shared_memory->ledger.requesting_access_to_exit++;

  /* end of the traffic critical section */
  sem_post(&(shared_memory->traffic_mutex));



  /* notify the station manager that a bus wants to leave the station */
  sem_post(&(shared_memory->station_manager_mutex));



  /* wait until station gives permission to exit */
  sem_wait(&(shared_memory->exit_queue));



  /* leave the station */
  Leave(bus, shared_memory, islet, spot);



  /* flag used to tell to the station manager that the bus has left the station,
     so that the next bus can proceed to leave */
  shared_memory->ledger.has_left = TRUE;



  /* notify the station manager that a bus left the station */
  sem_post(&(shared_memory->station_manager_mutex));



  /* write to logfile */
  sem_wait(&(shared_memory->logfile_mutex));

  write_status_to_logfile(shared_memory, bus, islet, LEAVE);

  sem_post(&(shared_memory->logfile_mutex));



  /* statistics critical section */
  sem_wait(&(shared_memory->statistics_mutex));

  update_statistics(shared_memory, bus);

  sem_post(&(shared_memory->statistics_mutex));
  /* end of statistics critical section */








  if (de_attach_shared_segment(shared_memory) == -1)
  {
    perror("Error in bus.c::de_attach_shared_segment()");
  }


  // printf("\n\n\nTerminating bus with id %s!\n\n", bus->id);


  free(bus);
  // printf("\n");
  return EXIT_SUCCESS;
}
