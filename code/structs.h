#ifndef __STRUCTS__
#define __STRUCTS__

#define MAX_HOLES 1000
#define PERMS 0777 // set access permissions

#include "time.h"

typedef unsigned int uint;
typedef unsigned char byte;
typedef unsigned long long ull;

typedef struct hole
{
  ull start;
  ull end;
} hole;


typedef struct hole_table
{
  hole holes[MAX_HOLES];
} hole_table;


typedef struct
{
  uint id;
  uint size;
  uint type;
  ull parent_id;
  time_t creation_time;
  time_t access_time;
  time_t modification_time;

  uint blocks_using;
  ull first_block;

  char name[];
} MDS;



typedef struct block
{
  ull next_block;
  byte data[];
} block;



typedef struct superblock
{
  // uint files;
  // uint links;
  // uint directories;

  uint fd;

  ull root_directory;
  uint current_size;

  uint current_hole_number;

  uint block_size;
  uint filename_size;
  uint max_file_size;
  uint max_dir_file_number;

  char cfs_filename[];

} superblock;




#endif
