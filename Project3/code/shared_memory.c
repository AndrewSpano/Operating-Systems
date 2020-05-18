#include "shared_memory.h"


int get_shared_segment(key_t shmid_key, int* shmid, shared_segment_t** shared_memory)
{

  /* get the id of the shared segment memory */
  *shmid = shmget(shmid_key, sizeof(shared_segment_t), SEGMENT_PERM | IPC_CREAT);
  if (*shmid == -1)
  {
    perror("Error in shared_memory.c::shmget()");
    return 0;
  }

  /* get the shared memory segment */
  *shared_memory = (shared_segment_t *) shmat(*shmid, (void *) 0, 0);
  if (*shared_memory == NULL)
  {
    perror("Error in shared_memory.c::shmat()");
    return 0;
  }

  return 1;
}




int de_attach_shared_segment(shared_segment_t* shared_memory)
{
  return shmdt(shared_memory);
}



int initialize_shared_segment(shared_segment_t* shared_memory, uint32_t total_buses, char* logfile_name)
{
  shared_memory->ledger.waiting_to_enter = 0;
  shared_memory->ledger.requesting_access_to_enter = 0;
  shared_memory->ledger.is_ready_to_enter = FALSE;
  shared_memory->ledger.has_parked = FALSE;
  shared_memory->ledger.requesting_access_to_exit = 0;
  shared_memory->ledger.has_left = FALSE;
  shared_memory->ledger.exit_traffic = 0;
  shared_memory->ledger.buses_departed = 0;
  shared_memory->ledger.total_buses = total_buses;

  shared_memory->ledger.sum_of_waiting_to_park_times = 0;
  shared_memory->ledger.counter_waiting_to_park_times = 0;
  shared_memory->ledger.sum_of_waiting_to_leave_times = 0;
  shared_memory->ledger.counter_waiting_to_leave_times = 0;


  int bay = 0;
  for (; bay < BAYS; bay++)
  {
    shared_memory->ledger.bays[bay].type = bay;
    shared_memory->ledger.bays[bay].total_spots = SPOTS;
    shared_memory->ledger.bays[bay].spots_taken = 0;
    shared_memory->ledger.bays[bay].total_passengers_dropped = 0;
    shared_memory->ledger.bays[bay].total_passengers_picked = 0;
    int spot = 0;
    for (; spot < SPOTS; spot++)
    {
      shared_memory->ledger.bays[bay].spots[spot].has_bus = FALSE;
      memset(&(shared_memory->ledger.bays[bay].spots[spot].current_bus), 0, sizeof(Bus));
    }
  }


  sem_init(&(shared_memory->station_manager_mutex), 1, 0);
  sem_init(&(shared_memory->entry_queue), 1, 1);
  sem_init(&(shared_memory->park_mutex), 1, 0);
  sem_init(&(shared_memory->exit_queue), 1, 0);
  sem_init(&(shared_memory->traffic_mutex), 1, 1);


  for (bay = 0; bay < BAYS; bay++)
  {
    sem_init(&(shared_memory->bays_mutex[bay]), 1, 1);
  }

  sem_init(&(shared_memory->logfile_mutex), 1, 1);
  sem_init(&(shared_memory->statistics_mutex), 1, 1);

  strcpy(shared_memory->logfile_name, logfile_name);

  /* create the logfile. if it already existed, erase its conternts */
  FILE* temp = fopen(shared_memory->logfile_name, "w");
  fclose(temp);

  return 1;
}




int free_shared_segment_memory(shared_segment_t* shared_memory)
{
  sem_destroy(&(shared_memory->station_manager_mutex));
  sem_destroy(&(shared_memory->entry_queue));
  sem_destroy(&(shared_memory->park_mutex));
  sem_destroy(&(shared_memory->exit_queue));
  sem_destroy(&(shared_memory->traffic_mutex));

  int bay = 0;
  for (; bay < BAYS; bay++)
  {
      sem_destroy(&(shared_memory->bays_mutex[bay]));
  }

  sem_destroy(&(shared_memory->logfile_mutex));
  sem_destroy(&(shared_memory->statistics_mutex));

  return 1;
}




int free_shared_segment(shared_segment_t* shared_memory, int shmid)
{
  int error = 0;

  error = shmdt(shared_memory);
  if (error == -1)
  {
    perror("Error in shared_memory.c::free_shared_segment::shmdt()");
  }

  error = shmctl(shmid, IPC_RMID, 0);
  if (error == -1)
  {
    perror("Error in shared_memory.c::free_shared_segment::shmctl()");
  }

  return error;
}
