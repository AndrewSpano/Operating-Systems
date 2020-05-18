#ifndef __SHARED_MEMORY__
#define __SHARED_MEMORY__


#define BAYS 3
#define SPOTS 4
#define MAX_ID_LENGTH 20

#define SEGMENT_PERM 0666
#define SHARED_MEMORY_KEY 1700146

#define MAX_FILENAME_LENGTH 25
#define MAX_BUFFER_SIZE 256

#define MAX_SLEEP_TIME 10

#define TRUE 1
#define FALSE 0


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "bus_funcs.h"



typedef struct Reference_Ledger
{
  uint32_t waiting_to_enter;
  uint8_t requesting_access_to_enter;
  uint8_t is_ready_to_enter;
  uint8_t has_parked;

  uint32_t requesting_access_to_exit;
  uint8_t has_left;
  uint8_t exit_traffic;

  Destination entering_bus_type;
  Destination islet_for_entering_bus;

  Bus_Bay bays[BAYS];

  uint64_t sum_of_waiting_to_park_times;
  uint16_t counter_waiting_to_park_times;

  uint64_t sum_of_waiting_to_leave_times;
  uint16_t counter_waiting_to_leave_times;

  uint32_t buses_departed;
  uint32_t total_buses;

} Reference_Ledger;





typedef struct shared_segment
{
  Reference_Ledger ledger;

  /* pretty much self explanatory */
  sem_t station_manager_mutex;

  /* used for entrance in the station */
  sem_t entry_queue;
  sem_t park_mutex;

  /* used for exiting the station */
  sem_t exit_queue;

  /* used to preserve the "traffic critical section" of the shared segment */
  sem_t traffic_mutex;

  /* used to preserve the "bay critical section" of the shared segment */
  sem_t bays_mutex[BAYS];

  /* used to access logfile: many writers (buses), 1 reader (compotroller) */
  sem_t logfile_mutex;

  /* used to access and change statistics for the comptroller and buses */
  sem_t statistics_mutex;

  char logfile_name[MAX_FILENAME_LENGTH];
} shared_segment_t;






int get_shared_segment(key_t shmid_key, int* shmid, shared_segment_t** shared_memory);
int de_attach_shared_segment(shared_segment_t* shared_memory);

int initialize_shared_segment(shared_segment_t* shared_memory, uint32_t total_buses, char* logfile_name);
int free_shared_segment_memory(shared_segment_t* shared_memory);
int free_shared_segment(shared_segment_t* shared_memory, int shmid);



#endif
