#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mystation_funcs.h"
#include "utilities.h"


int get_mystation_input(char* argv[], char** file_name)
{
  uint8_t flag_l = 0;
  uint8_t error_input = 0;

  int i = 1;
  for (; i < MYSTATION_ARGS; i += 2)
  {

    if (!strcmp(argv[i], "-l"))
    {

      if (!flag_l) {
        flag_l = 1;
      } else {
        printf("Error, parameter -l is given more than 1 time in mystation.c. Exiting..\n");
        error_input = 1;
        break;
      }

      *file_name = malloc(sizeof(char) * (strlen(argv[i + 1]) + 1));
      strcpy(*file_name, argv[i + 1]);

    }
    else {

      printf("No matching flags for this input: \"%s\" in mystation.c. Available flags are -l.\n", argv[i]);
      printf("Error in mystation_funcs.c::get_mystation_input()\n");
      error_input = 1;
      break;

    }

  }


  if (error_input) {
    if (flag_l) {
      free(*file_name);
    }
  }

  return !error_input;
}





int get_configfile_input(char* file_name, uint8_t* islets, uint8_t* spots_per_islet,
                                          uint16_t* max_capacity, double* max_park_period,
                                          double* max_man_time, uint32_t* total_buses,
                                          double* state_time, double* statistics_time)

{

  char myflag[MAX_BUFFER_SIZE];
  char myattribute[MAX_BUFFER_SIZE];
  char* buffer = NULL;

  FILE* configfile = fopen(file_name, "r");
  if (!configfile)
  {
    free(buffer);
    perror("fopen() error in mystation_funcs.c::get_configfile_input()");
    return 0;
  }

  /* keep flags so that we make sure we don't read an attribute twice, in case
     the configuration file has an error */
  uint8_t flag_islets = 0;
  uint8_t flag_spots_per_islet = 0;
  uint8_t flag_max_capacity = 0;
  uint8_t flag_max_park_period = 0;
  uint8_t flag_max_man_time = 0;
  uint8_t flag_state_time = 0;
  uint8_t flag_statistics_time = 0;
  uint8_t flag_total_buses = 0;

  /* variable used to indicate errors that may have occured */
  uint8_t error = 0;

  int i = 0;
  for (; i < CONFIGFILE_ARGS; i++)
  {

    /* if for some reason we reached the end of the file when we shouldn't
       have, return with error indication */
    if (feof(configfile))
    {
      printf("Not enough information in the configuration file.\n");
      error = 1;
      break;
    }


    /* read a line from the configuration file of the form:
       -parameter attribute */
    size_t line_buf_size = 0;
    if (getline(&buffer, &line_buf_size, configfile) == -1)
    {
      perror("Error when reading from the configuration file.");
      error = 1;
      break;
    }


    /* extract the parameter from the line we just read */
    if (get_nth_string(myflag, buffer, 1) == -1)
    {
      printf("Error when extracting a parameter from the buffer line.\n");
      free(buffer);
      error = 1;
      break;
    }

    /* extract the attribute from the line we just read */
    if (get_nth_string(myattribute, buffer, 2) == -1)
    {
      printf("Error when extracting an attribute from the buffer line.\n");
      free(buffer);
      error = 1;
      break;
    }

    /* we don't need the buffer anymore, so free it */
    free(buffer);

    /* get the parameters from the configuration file, while also checking for
       any mistaked that the input may have. If a mistake is spotted, exit and
       return an indication of an error */

    if (!strcmp(myflag, "-islets"))
    {

      if (flag_islets)
      {
        printf("Error, islets has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_islets = 1;
      }

      int temp = atoi(myattribute);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: islets can't be negative.\n");
        error = 1;
        break;
      }

      *islets = temp;

    }
    else if (!strcmp(myflag, "-spots_per_islet"))
    {

      if (flag_spots_per_islet)
      {
        printf("Error, spots_per_islet has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_spots_per_islet = 1;
      }

      int temp = atoi(myattribute);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: spots_per_islet can't be negative.\n");
        error = 1;
        break;
      }

      *spots_per_islet = temp;

    }
    else if (!strcmp(myflag, "-max_capacity_of_bus"))
    {

      if (flag_max_capacity)
      {
        printf("Error, max_capacity has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_max_capacity = 1;
      }

      int temp = atoi(myattribute);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: max_capacity can't be negative.\n");
        error = 1;
        break;
      }

      *max_capacity = temp;

    }
    else if (!strcmp(myflag, "-max_park_period"))
    {

      if (flag_max_park_period)
      {
        printf("Error, max_park_period has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_max_park_period = 1;
      }

      char* endptr = NULL;
      double temp = strtod(myattribute, &endptr);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: max_park_period can't be negative.\n");
        error = 1;
        break;
      }

      *max_park_period = temp;

    }
    else if (!strcmp(myflag, "-max_man_time"))
    {

      if (flag_max_man_time)
      {
        printf("Error, max_man_time has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_max_man_time = 1;
      }

      char* endptr = NULL;
      double temp = strtod(myattribute, &endptr);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: max_man_time can't be negative.\n");
        error = 1;
        break;
      }

      *max_man_time = temp;

    }
    else if (!strcmp(myflag, "-state_time"))
    {

      if (flag_state_time)
      {
        printf("Error, state_time has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_state_time = 1;
      }

      char* endptr = NULL;
      double temp = strtod(myattribute, &endptr);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: state_time can't be negative.\n");
        error = 1;
        break;
      }

      *state_time = temp;

    }
    else if (!strcmp(myflag, "-statistics_time"))
    {

      if (flag_statistics_time)
      {
        printf("Error, statistics_time has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_statistics_time = 1;
      }

      char* endptr = NULL;
      double temp = strtod(myattribute, &endptr);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: statistics_time can't be negative.\n");
        error = 1;
        break;
      }

      *statistics_time = temp;

    }
    else if (!strcmp(myflag, "-number_of_buses"))
    {

      if (flag_total_buses)
      {
        printf("Error, total_buses has already been read from the configuration file.\n");
        error = 1;
        break;
      }
      else
      {
        flag_total_buses = 1;
      }

      int temp = atoi(myattribute);
      if (temp < 0)
      {
        printf("Error, wrong value read from configuration file: number_of_buses can't be negative.\n");
        error = 1;
        break;
      }

      *total_buses = temp;

    }
    else
    {

      printf("The flag %s read from the configuration file is wrong. Exiting..\n", myflag);
      error = 1;
      break;

    }

  }


  /* close the configuration file */
  fclose(configfile);

  return !error;
}






pid_t create_station_manager(key_t key)
{
  pid_t station_manager_pid = fork();

  if (station_manager_pid < 0)
  {

    perror("Fork for station manager failed");

  }
  else if (station_manager_pid == 0)
  {

    /* convert parameters to strings */
    char* key_as_string = MallocOrDie(sizeof(char) * (MAX_BUFFER_SIZE + 1));
    sprintf(key_as_string, "%d", key);


    /* create vector of parameters and execute */
    char* args[] = {"./station-manager", "-s", key_as_string, NULL};
    execvp(args[0], args);

    perror("execvp() of station-manager failed");

  }

  return station_manager_pid;
}





pid_t create_compotroller(double state_time, double statistics_time, key_t key)
{
  pid_t comptroller_pid = fork();

  if (comptroller_pid < 0)
  {

    perror("Fork for comptroller failed");

  }
  else if (comptroller_pid == 0)
  {

    /* convert parameters to strings */
    char* time_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
    sprintf(time_as_string, "%lf", state_time);

    char* stattime_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
    sprintf(stattime_as_string, "%lf", statistics_time);

    char* key_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
    sprintf(key_as_string, "%d", key);


    /* create vector of parameters and execute */
    char* args[] = {"./comptroller", "-d", time_as_string, "-t", stattime_as_string, "-s", key_as_string, NULL};
    execvp(args[0], args);

    perror("execvp() of comptroller failed");

  }

  return comptroller_pid;
}




int create_buses(pid_t bus_pids[], uint32_t total_buses, uint16_t max_capacity, double max_park_period,
                                                         double max_man_time, key_t key)
{
  srand(time(NULL));
  int i = 0;
  for (; i < total_buses; i++)
  {

    bus_pids[i] = fork();
    if (bus_pids[i] < 0)
    {

      perror("Fork for bus failed");
      printf("i = %d\n", i);
      int j = 0;
      for (j = 0; j < i; j++)
      {
        kill(bus_pids[j], SIGKILL);
      }
      return TRUE;

    }
    else if (bus_pids[i] == 0)
    {

      /* calculate random parameters for the bus */
      char* type[4] = {"ASK", "PEL", "VOR", NULL};
      uint8_t random_type = rand() % 3;
      uint16_t random_capacity = rand() % (max_capacity / 2) + (max_capacity / 2);
      uint16_t random_incpassengers = rand() % (random_capacity + 1);
      double random_parkperiod = (((double) rand()) / RAND_MAX) * max_park_period;
      double random_mantime = (((double) rand()) / RAND_MAX) * max_man_time;


      /* convert parameters to strings */
      char* capacity_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
      sprintf(capacity_as_string, "%d", random_capacity);

      char* incpassengers_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
      sprintf(incpassengers_as_string, "%d", random_incpassengers);

      char* parkperiod_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
      sprintf(parkperiod_as_string, "%lf", random_parkperiod);

      char* mantime_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
      sprintf(mantime_as_string, "%lf", random_mantime);

      char* id_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
      sprintf(id_as_string, "%d", i);

      char* key_as_string = MallocOrDie(sizeof(char) * MAX_BUFFER_SIZE);
      sprintf(key_as_string, "%d", key);


      /* create vector of parameters and execute */
      char* args[] = {"./bus", "-t", type[random_type], "-n", incpassengers_as_string,
                               "-c", capacity_as_string, "-p", parkperiod_as_string,
                               "-m", mantime_as_string, "-i", id_as_string, "-s", key_as_string, NULL};
      execvp(args[0], args);

      perror("execvp() of a bus failed");
      printf("i = %d\n", i);

    }

    /* random sleep time in between the creation of buses */
    uint8_t random_sleep_time = rand() % (MAX_SLEEP_TIME + 1);
    sleep(random_sleep_time);

  }

  return FALSE;
}



int wait_child_processes(pid_t station_manager_pid, pid_t comptroller_pid, pid_t bus_pids[], uint32_t total_buses)
{
  int returnStatus = 0;
  pid_t wait_pid;
  while ((wait_pid = wait(&returnStatus)) > 0)
  {
    if (!WIFEXITED(returnStatus) || (WIFEXITED(returnStatus) && WEXITSTATUS(returnStatus) != EXIT_SUCCESS))
    {
      /* an error has occured, so kill all processes in order to exit normally */
      printf("\nChild process bus with pid %d terminated with an error. Exiting..\n", wait_pid);

      int j = 0;
      /* kill the buses */
      for (; j < total_buses; j++)
      {
        /* kill the bus process */
        kill(bus_pids[j], SIGKILL);
        /* harvest the zombie process */
        waitpid(bus_pids[j], &returnStatus, 0);
      }

      /* kill the comptroller process */
      kill(comptroller_pid, SIGKILL);
      /* harvest the zombie */
      waitpid(comptroller_pid, &returnStatus, 0);

      /* kill the station manager process */
      kill(station_manager_pid, SIGKILL);
      /* harvest the zombie */
      waitpid(station_manager_pid, &returnStatus, 0);

      return TRUE;
    }
  }

  return FALSE;
}
