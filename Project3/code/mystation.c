#include "mystation_funcs.h"
#include "shared_memory.h"
#include "utilities.h"


int main(int argc, char* argv[])
{

  // printf("\n");
  if (argc != MYSTATION_ARGS)
  {
    printf("Wrong command line parameters given in mystation.c. The input should be something like:\n\n");
    printf("./mystation -l configfile\n\n");
    exit(EXIT_FAILURE);
  }

  char* file_name = NULL;
  if (!get_mystation_input(argv, &file_name))
  {
    printf("Wrong values for command line parameters in mystation.c\n\n");
    exit(EXIT_FAILURE);
  }


  uint8_t islets = 0;
  uint8_t spots_per_islet = 0;
  uint16_t max_capacity = 0;
  double max_park_period = 0.0;
  double max_man_time = 0.0;

  double state_time = 0.0;
  double statistics_time = 0.0;

  /* number of buses that the mystation process will produce */
  uint32_t total_buses = 0;

  if (!get_configfile_input(file_name, &islets, &spots_per_islet, &max_capacity, &max_park_period, &max_man_time, &total_buses, &state_time, &statistics_time))
  {
    printf("An error occured with the configuration file: %s.\n\n", file_name);
    free(file_name);
    exit(EXIT_FAILURE);
  }



  /* variable used to catch errors */
  int error = 0;

  /* in the logfile we will write useful information */
  char logfile_name[MAX_FILENAME_LENGTH] = "Log.txt";


  /* initialize the key for the shared memory */
  key_t key = SHARED_MEMORY_KEY;
  /* create shared memory segment and get the id */
  int shmid = shmget(key, sizeof(shared_segment_t), SEGMENT_PERM | IPC_CREAT);
  if (shmid == -1)
  {
    perror("Error in mystation.c::shmget()");
    free(file_name);
    exit(-1);
  }

  /* no need to worry about critical section, because right now no other
     processes are running */
  shared_segment_t* shared_memory = (shared_segment_t *) shmat(shmid, (void *) 0, 0);
  if (shared_memory == NULL)
  {
    perror("Error in mystation.c::shmat()");
    shmctl(shmid, IPC_RMID, NULL);
    free(file_name);
    exit(-2);
  }





  /* initialize all the shared memory variables */
  initialize_shared_segment(shared_memory, total_buses, logfile_name);


  // free_and_exit(shared_memory, shmid, &file_name);


  /* Execute station-manager */
  pid_t station_manager_pid = create_station_manager(key);

  if (station_manager_pid < 0)
  {
    printf("An error occured while creating the station manager process. Exiting ..\n");
    free_and_exit(shared_memory, shmid, &file_name);
  }




  /* Execute comptroller */
  pid_t comptroller_pid = create_compotroller(state_time, statistics_time, key);

  if (comptroller_pid < 0)
  {
    printf("An error occured while creating the comptroller process. Exiting ..\n");
    kill(station_manager_pid, SIGKILL);
    free_and_exit(shared_memory, shmid, &file_name);
  }



  /* Execute buses */
  pid_t bus_pids[total_buses];
  error = create_buses(bus_pids, total_buses, max_capacity, max_park_period, max_man_time, key);

  if (error)
  {
    printf("An error occured while creating the bus processes. Exiting ..\n");
    kill(station_manager_pid, SIGKILL);
    kill(comptroller_pid, SIGKILL);
    free_and_exit(shared_memory, shmid, &file_name);
  }



  /* wait for all processes to be over */
  error = wait_child_processes(station_manager_pid, comptroller_pid, bus_pids, total_buses);

  if (error)
  {
    printf("A problem occured while waiting for the child processes to terminate. Exiting ..\n");
    free_and_exit(shared_memory, shmid, &file_name);
  }





  if (free_shared_segment(shared_memory, shmid) == -1)
  {
    printf("\nProblem with shared memory when terminating mystation.c\n\n");
    free(file_name);
    exit(EXIT_FAILURE);
  }


  free(file_name);
  // printf("\n");
  return EXIT_SUCCESS;
}
