# Operating Systems Assignments

In this repo there are the 4 assignments that I did for the class K22 Operating Systems of the Department of Informatics and Telecommunications, Athens.

## Short description

- [Assignment 1:](##-Assignment-1) Voting Simulation (basically implement data structures like a RBTree or a Bloom filter.
- [Assignment 2:](##-Assignment-2) Sorting via different processes (basically to familiarize with different system calls like fork(), exec() and process communication with pipes).
- [Assignment 3:](##-Assignment-3) Bus station Simulation (basically implementation of a buses as processes and a bus_station manager that uses shared memory between processes to guide them).
- [Assignment 4:](##-Assignment-4) Implementation of a cfs (container file system). Basically the assigment is to create a whole (simplified) file system inside a file. The file is organized so that is has an order.

<br />
<br />

## [Assignment 1](Project1/code)

As the short description suggests, this project is a simulation of a voting procedure where different data structures are used in order to maintain an order regarding the voters, and be able to perform queries in sub-linear time.

### The data structures used are:
- [Bloom Filter](https://en.wikipedia.org/wiki/Bloom_filter)
- [Red Black Tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree)
- [List of lists](https://en.wikipedia.org/wiki/List_of_data_structures)

### The functionalities (commands) of the simulation are:

1. **lbf key** (lookup bloom filter)\
Check the Bloom Filter data structure for the existence of the a record with the *key* passed in the command. There 2 possible answers: 1) No, 2) Maybe.

2. **lrb key** (lookup red-black tree)\
Check the Red Black Tree data structure for the existence of a record with the *key* passed in the command. If it exists, print it.

3. **ins record** (insert record to red-black-tree)\
Insert a *record* into the Red Black Tree, where all the fields of the record are given in one line. Update the Bloom Filter correspondigly.

4. **find key**\
Search the data structures for the record with *key*. If it exists, print it.

5. **delete key**\
Delete the record with *key* from the data structures. Note that deletion does not affect the Bloom Filter.

6. **vote key**\
Change the state of the voter record with *key*. If he has already voted, print a corresponding message.

7. **load fileofkeys**\
For every *key* in the file fileofkeys, try to vote for the corresponding record.

8. **voted**\
Print all the voters.

9. **voted postcode**\
Print all the voters from a specific *postcode*.

10. **votedperpc**\
For every *postcode*, provide the fraction of voters in it.

11. **exit**\
Exit the simulation and free all the allocated memory from the data structures.


### Compiling and Running

- To compile the project, go to the [project directory](Project1/code) and type
```bash
$ make
```

- To run the project, give an input of the form
```bash
$ ./runelection -i inputfile -o -outfile -n numofupdates
```
**runelection** is the executable\
**inputfile** is the file containing the records\
**outfile** is the name of the file where you want to store the output of the program\
**numofupdates** is the number of updates that must happen before recreating the bloom filter

An example of a run is
```bash
$ ./runelection -i ../Test_Files/10000records.csv -o output.txt -n 20
```

You can also use *valgrind* to check for memory leaks, e.g.
```bash
$ valgrind ./runelection -i ../Test_Files/1000records.csv -o output.txt -n 15
```

- To remove the executable and the objectives, use
```bash
$ make clean
```
<br />
<br />


## [Assignment 2](Project2/code)

In this project we implement 2 sorting algorithms (*quicksort* and *heapsort*), that work as separate processes from the rest of the program. The different processes communicate with the usage pipes, by passing the different flags and records needed for the sorting. The purpose of this assignment is to familiarize the developer with basic system calls (like fork(), exec(), etc..) and also intra-process communication.

### The Hierarchy of the processes
1) [coordinator](Project2/code/coordinator.c)
2) [coach](Project2/code/coach.c)
3) [sorter](Project2/code/sorter.c)
4) [quicksort](Project2/code/quicksort.c) and [heapsort](Project2/code/heapsort.c) (actual sorting algorithms)

### Explanation
The *coordinator*, as the name suggests, is the main process in this family of processes. His job is to create 1 to 4 (depending on the user input) *coaches* and measure their execution times.

Each *coach* is responsible for sorting the records given by the *coordinator*, at one specific attribute. The *coach* splits the records in different chunks and passes them to the *sorters*, which in turn perform the sorting and return, and then merges the results. The results for sorting by attrubute *n* are written in a file myinputfile.*n*, which is created at runtime. Every *coach* splits the record dataset in a different way, depending on its number (1-4).

A *sorter* is essentially an intermediate process, used to invoke the proper sorting algorithm (*quicksort* or *heapsort*) and send back the results to the *coach*.

The *quicksort* and *heaport* processes are pretty much self-explanatory. They just read the records from a pipe passed by their corresponding *sorter*, sort them with the specified algorithm by a specified attribute (column) and return.


### Compiling and Running

- To compile the project, go to the [project directory](Project2/code) and type
```bash
$ make
```

- To run the project, give an input of the form
```bash
$ ./mysort -f inputfile [-h|-q columnid] [-h|-q columnid] [-h|-q columnid] [-h|-q columnid]
```
**mysort** is the executable\
**inputfile** is the file containing the records\
**h** denotes the usage of heapsort algorithm to sort columnid\
**q** denotes the usage of quicksort algorithm to sort columnid\
**columnid** is the index of the column that we want to sort by (if we have n columns, then columnid can take values 1-n)

An example of a run is
```bash
$ ./mysort -f ../Data4Project2/Records100000.bin -q 2 -h 4 -h 7
```

You can also use *valgrind* to check for memory leaks, e.g.
```bash
$ valgrind ./mysort -f ../Data4Project2/Records10000.bin -h 1 -h 4 -h 7 -h 8
```

- To remove the executable and the objectives, use
```bash
$ make clean
```

- To remove the output files with the sorted Records, use
```bash
$ make rm_files
```

<br />
<br />

## [Assignment 3](Project3)

This project implements a Bus Station simulation. An overview of the Bus Station can be found in the 6th page of this [README.pdf](Project3/README.pdf). The purpose of this project is to familiarize the developer with race conditions, shared-memory, semaphores and intra-process communication. Basically we have *buses* that want to get into the station, park for a random amount of time and then depart. Each *Bus* is a different process. The catch is that the Bus Station has a "restricted" space, so only 1 *bus* at a time can be performing a manuever to park and only 1 *bus* can be performing a manuever to un-park. We need a process to coordinate the different *buses* (processes), that is, to tell them to wait before entering or departing the Station. This process is called *Station Manager*, and he is responsible for the traffic inside the Bus Station. Also, we have another process called *Compotroller*, which has the task of providing statistics about the different wait times of the *buses*, and also updating a log file which records the arrivals and departures of the *buses*.

### Configuration of the Simulation

The amount of buses that come to the station, the max capacity of passengers of a bus, the time intervals at which we want the statistics to be printed and other features can be manually edited in the [Configuration File](Project3/Configuration_File.txt). Note that the *flags* should not be changed. Only their values should be modified.

### Compiling and Running

- To compile the project, go to the [project directory](Project3) and type
```bash
$ make
```

- To run the project, give an input of the form
```bash
$ ./mystation -l Configuration_File.txt
```
**mystation** is the executable\
**Configuration_File** is the Configuration File containing the parameters of the simulation

You can also use *valgrind* to check for memory leaks, e.g.
```bash
$ valgrind ./mystation -l Configuration_File.txt
```

- To remove the executable and the objectives, use
```bash
$ make clean
```

- To remove the Log File created by the *Comptroller*, use
```bash
$ make rm_log
```
<br />
<br />

## [Assignment 4](Project4)

In this assignment we have implemented a Container File System (cfs) in C. The user can create files, directories, rename them, concatenate them, delete them, copy from one location to another, import entites from the Linux File System, export entities to the Linux File System, and many more functionalities.

### A little bit about the implementation

If we introduce a small level of abstraction, we could think of a file as a big 1D array of bytes. Modifying these bytes properly, anything can be created. And this is how we implement a File System inside a file (cfs), by allocating chunks of bytes per entity. Each entity comes with a Meta-Data structure, which contains useful fields for it. These structures help us maintain an order inside the File System. Each time an entity is deleted, a "hole" is created inside the array. We keep track of this "holes" in order to fill them later when a new entity comes.

An overview of the cfs can be found in the first page of this [README.pdf](Project4/README.pdf).

- **Superblock:** A Block that contains useful information (and meta-data) for the whole cfs file. Each cfs file gets one of these when it's created.
- **Hole Map:** A big array of bytes that helps us keep track of where the holes are. This also helps implement the "first fit" algorithm when introducing new entities.
- **Root MDS:** The Meta-Data Structure of the root directory.
- **Data Blocks of Root:** The Data blocks of the root directory.
- **MDS:** A Meta-Data Structure for any other entity inside the cfs. It contains the position of the first data Block.
- **Block:** A Block containing the data of the specific entity. It also contains the position of the next data block, if it exists.

### Functionalities (commands) of the cfs File System

1. **cfs_create [-bs block_size] [-fns filename_size] [-cfs max_file_size] [-mdfn max_directory_file_number] any_name.cfs**\
Create a .cfs File System with name "any_name.cfs"\
-bs: The size of the block (in bytes) that will contain the information of the entities\
-fns: The max size (in bytes) of the name of an entity\
-cfs: The max size (in bytes) of an entity\
-mdfn: The max number of entities per directory\
If no flags are passed, then the default values for the above fields are assumed

2. **cfs_mkdir dir_name**\
Create a directory with name "dir_name"

3. **cfs_touch [-a|-m] file_name**\
Create an empty file with name "file_name" (if no flags are given)\
-a: Update only the access time of "file_name"\
-m: Update only the edit time of "file_name"

4. **cfs_pwd**\
Print working directory

5. **cfs_cd path**\
Change Directory to "path"

6. **cfs_ls [-a] [-r] [-l] [-u] [-d|-h] [entity1 entity2 ...]**\
Print the contents of "entity1", "entity2", ...\
If no entities are given then print cwd info\
-a: Print all entities, including hidden directories "." and ".."\
-r: Print recursively all entities\
-l: Print the info of the entities (size, creation time, last access time, etc..)\
-u: Print the files without sorting, as they appear normally in the directory\
-d: Print only directories\
-h: Print only links

7. **cfs_cp [-R|-r] [-i] path_of_source1 path_of_source2 ... path_of_dest**\
Copy "path_of_source1", "path_of_source2", ... to the destination directory "path_of_dest"\
-R: Copy the contents of directories only at depth 1\
-i: Copy each entity after asking the user for permission\
-r: Copy recursively (all the way) the directories

8. **cfs_mv [-i] path_of_source1 path_of_source2 ... path_of_dest**\
Move or rename entities "path_of_source1", "path_of_source2", ... to "path_of_dest"\
Renaming happens only if 1 source is given, else we move all the sources to the directory "path_of_dest"
-i: Rename/Move entities after asking the user for permission

9. **cfs_cat path_to_source1, path_to_source2, ... -o path_to_dest**\
Concatenate all files "path_to_source1", path_to_source2, ... to a new file "path_to_dest"

10. **cfs_ln source dest**\
Create a hard link for "source", named "dest"

11. **cfs_rm [-i] [-r] path_of_dest1 path_of_dest2 ...**\
Delete entities from the File System\
-i: Delete entities after asking the user for permission
-r: Delete the directories recursively, including the directories

12. **cfs_import path_in_linux_file_system1, path_in_linux_file_system2, ... path_in_cfs**\
Import entities "path_in_linux_file_system1", "path_in_linux_file_system2", ... of the Linux File System to the cfs directory "path_in_cfs"

13. **cfs_export path_in_cfs1, path_in_cfs2, ... path_of_linux_file_system**\
Export the cfs entities "path_in_cfs1", "path_in_cfs2", ... to the Linux File System Directory "path_of_linux_file_system"

14. **cfs_workwith any_existing_name.cfs**\
Choose the cfs file on which to operate, named "any_existing_name.cfs"

15. **cfs_exit**\
Exit the program and free all the allocated memory


### Compiling and Running

- To compile the project, go to the [project directory](Project4) and type
```bash
$ make
```

- To run the project, give an input of the form
```bash
$ ./mycfs
```
**mycfs** is the executable

You can also use *valgrind* to check for memory leaks, e.g.
```bash
$ valgrind ./mycfs
```

- To remove the executable and the objectives, use
```bash
$ make clean
```