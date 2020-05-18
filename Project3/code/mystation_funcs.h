#ifndef __MYSTATION__
#define __MYSTATION__

#define MYSTATION_ARGS 3
#define CONFIGFILE_ARGS 8

#include "shared_memory.h"


int get_mystation_input(char* argv[], char** file_name);
int get_configfile_input(char* file_name, uint8_t* islets, uint8_t* spots_per_islet, uint16_t* capacity,
                                          double* max_park_period, double* max_man_time, uint32_t* total_buses,
                                          double* state_time, double* statistics_time);

pid_t create_station_manager(key_t key);
pid_t create_compotroller(double state_time, double statistics_time, key_t key);
int create_buses(pid_t bus_pids[], uint32_t total_buses, uint16_t max_capacity, double max_park_period,
                                                         double max_man_time, key_t key);

int wait_child_processes(pid_t station_manager_pid, pid_t comptroller_pid, pid_t bus_pids[], uint32_t total_buses);


#endif
