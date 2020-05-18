#include "comptroller_funcs.h"
#include "shared_memory.h"


int main(int argc, char* argv[])
{

  // printf("\n");
  if (argc != COMP_ARGS)
  {
    printf("Wrong command line parameters given in comptroller.c. The input should be something like:\n\n");
    printf("./comptroller -d time -t stattimes -s shmid\n\n");
    exit(EXIT_FAILURE);
  }

  double time = 0.0;
  double stattimes = 0.0;
  key_t shmid_key;

  if (!get_comptroller_input(argv, &time, &stattimes, &shmid_key))
  {
    printf("Wrong values for command line parameters in comptroller.c\n\n");
    exit(EXIT_FAILURE);
  }




  int shmid = 0;
  shared_segment_t* shared_memory = NULL;
  if (!get_shared_segment(shmid_key, &shmid, &shared_memory))
  {
    perror("Error in comptroller.c::get_shared_segment()");
    exit(-1);
  }





  int time_as_int = (int) time;
  int stattimes_as_int = (int) stattimes;



  /* while the simulation is not over */
  while (shared_memory->ledger.buses_departed < shared_memory->ledger.total_buses)
  {
    /* print the state and statistics= */

    printf("\n\n\n\n");

    sleep(time_as_int);
    print_state(shared_memory);

    printf("\n\n\n\n");

    sleep(stattimes_as_int);
    print_statistics(shared_memory);

  }




  if (de_attach_shared_segment(shared_memory) == -1)
  {
    perror("Error in comptroller.c::de_attach_shared_segment()");
  }



  printf("\n");
  return EXIT_SUCCESS;
}
