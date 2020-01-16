#ifndef __STRUCTS__
#define __STRUCTS__

#define MAX_HOLES 1000
#define PERMS 0777 // set access permissions

#define LINK 0
#define DIRECTORY 1
#define FILE 2

#define MAX_CFS_FILENAME_SIZE 30



#include "time.h"

typedef unsigned int uint;
typedef unsigned char byte;
typedef unsigned long long ull;

typedef struct hole
{
  size_t start;
  size_t end;
} hole;


typedef struct hole_map
{
  uint current_hole_number;
  hole holes_table[MAX_HOLES];
} hole_map;


typedef struct
{
  uint id;
  size_t size;
  uint type;
  uint number_of_hard_links;
  size_t parent_offset;

  time_t creation_time;
  time_t access_time;
  time_t modification_time;

  uint blocks_using;
  size_t first_block;
} MDS;



typedef struct pair
{
  size_t offset;
  char name[];
} pair;


typedef struct block
{
  size_t next_block;
  byte data[];
} block;



typedef struct superblock
{
  // uint files;
  // uint links;
  // uint directories;

  uint fd;
  char cfs_filename[MAX_CFS_FILENAME_SIZE];

  size_t root_directory;
  size_t current_size;
  size_t block_size;
  size_t filename_size;
  size_t max_file_size;
  uint max_dir_file_number;
} superblock;




#endif
