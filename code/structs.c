
typedef unsigned int uint;

typedef struct cfs_header
{
  uint block_size;
  uint number_of_files;
  uint number_of_directories;
  uint number_of_links;

  uint file_name_size;
  char* file_name;

  uint max_file_size;
  uint max_directort_file_number;



  char* path;
} cfs_header;



typedef struct
{
  uint datablocks[DATABLOCK_NUM];
} Datastream

typedef struct
{

} MDS;


typedef struct block
{
  
} block;
