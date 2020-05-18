#include "station-manager_funcs.h"




int get_station_manager_input(char* argv[], key_t* shmid_key)
{
  uint8_t flag_s = 0;

  uint8_t error_input = 0;

  int i = 1;
  for(; i < STAT_MAN_ARGS; i += 2)
  {

    if (!strcmp(argv[i], "-s"))
    {

      if (!flag_s) {
        flag_s = 1;
      } else {
        printf("Error, parameter -s is given more than 1 time in station-manager.c. Exiting..\n");
        error_input = 1;
        break;
      }

      key_t s = atoi(argv[i + 1]);
      if (s <= 0) {
        printf("Wrong value for -s parameter in station-manager.c, it should be positive. Exiting..\n");
        error_input = 1;
        break;
      }

      *shmid_key = s;

    }
    else {

      printf("No matching flags for this input: \"%s\" in station-manager.c. Available flags are -s.\n", argv[i]);
      printf("Error in station-manager_funcs.c::get_station_manager_input()\n");
      error_input = 1;
      break;

    }

  }

  return !error_input;
}



/* return an appropriate islet with an availabe parking spot
   if no such islet is availabe, return -1
   keep in mind that: ASK == 0, PEL == 1, VOR == 2 */
int findSpot(shared_segment_t* shared_memory, Destination dest)
{
  if (dest == ASK || dest == VOR)
  {
    /* check first their own islets */
    if (shared_memory->ledger.bays[dest].spots_taken < shared_memory->ledger.bays[dest].total_spots)
    {
      return dest;
    }
    /* else, check PEL's islet */
    else if (shared_memory->ledger.bays[PEL].spots_taken < shared_memory->ledger.bays[PEL].total_spots)
    {
      return PEL;
    }
    else
    {
      return -1;
    }
  }
  else
  {
    if (shared_memory->ledger.bays[PEL].spots_taken < shared_memory->ledger.bays[PEL].total_spots)
    {
      return PEL;
    }
    else
    {
      return -1;
    }
  }
}
