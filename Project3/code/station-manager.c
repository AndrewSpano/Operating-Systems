#include "station-manager_funcs.h"
#include "shared_memory.h"


int main(int argc, char* argv[])
{
  // printf("\n");
  if (argc != STAT_MAN_ARGS)
  {
    printf("Wrong command line parameters given in station-manager.c. The input should be something like:\n\n");
    printf("./station-manager -s shmid\n\n");
    exit(EXIT_FAILURE);
  }

  key_t shmid_key;

  if (!get_station_manager_input(argv, &shmid_key))
  {
    printf("Wrong values for command line parameters in station-manager.c\n\n");
    exit(EXIT_FAILURE);
  }


  // printf("\nExecuting station-manager!\n");


  int shmid = 0;
  shared_segment_t* shared_memory = NULL;
  if (!get_shared_segment(shmid_key, &shmid, &shared_memory))
  {
    perror("Error in station-manager.c::get_shared_segment()");
    exit(-1);
  }







  while (shared_memory->ledger.buses_departed < shared_memory->ledger.total_buses)
  {

    /* block station-manager while no process requests access to the station.
       Wait for a bus to wake up the station manager */
    sem_wait(&(shared_memory->station_manager_mutex));


    /* if we get here, it means some bus wants to either move (enter or exit),
       or inform that it has finished its manuever. So use the flags defined
       in the shared segment to distinguish which case it is */



    /* if the below condition is true, then a bus has left the station, so we
       have to update the exit traffic in order for the next bus to leave */
    if (shared_memory->ledger.has_left)
    {
      /* the next bus to leave will not have left */
      shared_memory->ledger.has_left = FALSE;
      /* decrement the traffic because the previous bus left */
      shared_memory->ledger.exit_traffic--;
      /* increment the counter of the departed buses */
      shared_memory->ledger.buses_departed++;
    }



    /* if the below condition is true, at least one bus is waiting to exit, and
       there is no exit traffic, so the bus can leave */
    if (shared_memory->ledger.requesting_access_to_exit && shared_memory->ledger.exit_traffic < 1)
    {

      sem_wait(&(shared_memory->traffic_mutex));
      /* start of "traffic critical section" */

      /* the bus will leave the station, so decrement the counter */
      shared_memory->ledger.requesting_access_to_exit--;

      /* end of "traffic critical section" */
      sem_post(&(shared_memory->traffic_mutex));


      /* update the exit_traffic variable so that no other bus can leave */
      shared_memory->ledger.exit_traffic++;

      /* unblock the bus so it can leave */
      sem_post(&(shared_memory->exit_queue));
    }




    /* if the below condition is true, there is a bus which wishes to enter
       the station, and is communicating with the station manager */
    if (shared_memory->ledger.requesting_access_to_enter && shared_memory->ledger.is_ready_to_enter)
    {
      /* get the destination of the bus that wants to enter */
      Destination dest = shared_memory->ledger.entering_bus_type;

      /* try to find a spot for the bus to park */
      int islet = findSpot(shared_memory, dest);


      /* if an islet has an availabe spot for the bus to park */
      if (islet != -1)
      {
        /* variable in shared memory used to "indicate" to the bus where
           to park */
        shared_memory->ledger.islet_for_entering_bus = islet;


        sem_wait(&(shared_memory->traffic_mutex));
        /* start of "traffic critical section" */

        /* the bus will park, so decrement the counter */
        shared_memory->ledger.requesting_access_to_enter--;
        /* the bus will no longer be waiting to enter, so decrement the counter
           of waiting buses */
        shared_memory->ledger.waiting_to_enter--;

        /* end of "traffic critical section" */
        sem_post(&(shared_memory->traffic_mutex));


        /* the bus will no longer be waiting to enter because it will park */
        shared_memory->ledger.is_ready_to_enter = FALSE;

        /* unblock the bus and let it proceed to park */
        sem_post(&(shared_memory->park_mutex));
      }
    }

    /* if the below condition is true, then the bus has parked. Therefore we can
       move onto the next bus */
    else if (shared_memory->ledger.has_parked)
    {
      /* new bus will not have parked */
      shared_memory->ledger.has_parked = FALSE;
      /* unblock the new bus */
      sem_post(&(shared_memory->entry_queue));
    }


  }







  if (de_attach_shared_segment(shared_memory) == -1)
  {
    perror("Error in station-manager.c::de_attach_shared_segment()");
  }


  // printf("\nTerminating station-manager!\n");

  return EXIT_SUCCESS;
}
