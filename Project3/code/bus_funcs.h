#ifndef __BUS__
#define __BUS__

#define BUS_ARGS 15

#define ARRIVE 0
#define PARK 1
#define LEAVE 2

#define DROP 0
#define PICKUP 1

#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <semaphore.h>

typedef struct shared_segment shared_segment_t;

#include "shared_memory.h"
#include "utilities.h"

typedef enum Destination
{
  ASK = 0,
  PEL = 1,
  VOR = 2
} Destination;



typedef struct Bus
{
  Destination type;
  struct tm arrival_time;
  struct tm park_time;
  struct tm departure_time;
  uint16_t on_board_passengers;
  uint16_t capacity;
  double parkperiod;
  double mantime;
  char id[MAX_ID_LENGTH];
} Bus;



typedef struct Bus_Spot
{
  uint8_t has_bus;
  Bus current_bus;
} Bus_Spot;



typedef struct Bus_Bay
{
  Destination type;
  Bus_Spot spots[SPOTS];
  uint8_t total_spots;
  uint8_t spots_taken;
  uint32_t total_passengers_dropped;
  uint32_t total_passengers_picked;
} Bus_Bay;





int get_bus_input(char* argv[], Bus* bus, key_t* shmid_key);
void bus_init(Bus* bus);

void park_to_spot(Bus* bus, shared_segment_t* shared_memory, Destination dest, uint8_t spot);
void Park(Bus* bus, shared_segment_t* shared_memory, Destination dest, uint8_t* spot);

int Drop_Passenger(Bus* bus, shared_segment_t* shared_memory, Destination islet, uint8_t spot);
void Wait_Parked(Bus* bus);
int Pick_Passengers(Bus* bus, shared_segment_t* shared_memory, Destination islet, uint8_t spot);

void Leave(Bus* bus, shared_segment_t* shared_memory, Destination islet, uint8_t spot);

void write_status_to_logfile(shared_segment_t* shared_memory, Bus* bus, int bay, int status);
void write_activity_to_logfile(shared_segment_t* shared_memory, Bus* bus, int activity, int amount, int bay);
void update_statistics(shared_segment_t* shared_memory, Bus* bus);

void print_bus_info(Bus* bus, int bay, int spot);



#endif
