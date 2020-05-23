# Operating Systems Assignments

In this repo there are the 4 assignments that I did for the class K22 Operating Systems of the Department of Informatics and Telecommunications, Athens.

- Assignment1: Voting Simulation (basically implement data structures like a RBTree or a Bloom filter.
- Assignment2: Sorting via different processes (basically to familiarize with different system calls like fork(), exec() and process communication with pipes).
- Assignment 3: Bus station Simulation (basically implementation of a buses as processes and a bus_station manager that uses shared memory between processes to guide them).
- Assignment 4: Implementation of a cfs (container file system). Basically the assigment is to create a whole (simplified) file system inside a file. The file is organized so that is has an order.

Programmatistic Assignment in Operating Systems: Implement a Container File System (cfs) in C. Basically you can create a file and it works just like a whole file system. You can create directories, (empty) files, rename them, concatenate them, delete them, copy from one location to another, etc.. You can also import and export files/directories to the linux file system.

Some of the commands are:

1. cfs_create any_name.cfs (parameters can be given)
2. cfs_mkdir dir_name (create a directory, you can also give a path)
3. cfs_touch file_name (create a file, you can also give a path)
4. cfs_pwd (print working directory)
5. cfs_ls (parameters can be given)
6. cfs_cp path_of_source1 path_of_source2 ... path_of_dest (copy files/directories, parameters can be given)
7. cfs_mv path_of_source1 path_of_source2 ... path_of_dest (move/rename files/directories, parameters can be given)
8. cfs_ln source dest (create a hard link)
9. cfs_rm path_of_dest1 path_of_dest2 ... (delete files/directories, parameters can be given)
10. cfs_import path_of_linux_file_system path_in_cfs (import from linux to cfs)
11. cfs_export path_in_cfs path_of_linux_file_system (export to linux file system)
12. cfs_workwith any_existing_name.cfs (choose with which cfs file you want to work with)
