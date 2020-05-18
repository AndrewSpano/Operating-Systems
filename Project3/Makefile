LIBS =
CFLAGS = -g -Wall -pthread -lrt
CC = gcc


all: mystation bus comptroller station-manager


mystation: mystation.o mystation_funcs.o shared_memory.o utilities.o
	$(CC) $(CFLAGS) mystation.o mystation_funcs.o shared_memory.o utilities.o -o mystation

bus: bus.o bus_funcs.o shared_memory.o utilities.o
	$(CC) $(CFLAGS) bus.o bus_funcs.o shared_memory.o utilities.o -o bus

comptroller: comptroller.o comptroller_funcs.o shared_memory.o bus_funcs.o utilities.o
	$(CC) $(CFLAGS) comptroller.o comptroller_funcs.o shared_memory.o bus_funcs.o utilities.o -o comptroller

station-manager: station-manager.o station-manager_funcs.o shared_memory.o utilities.o
	$(CC) $(CFLAGS) station-manager.o station-manager_funcs.o shared_memory.o utilities.o -o station-manager




clean:
	rm -f shared_memory.o
	rm -f utilities.o
	rm -f bus bus.o bus_funcs.o
	rm -f comptroller comptroller.o comptroller_funcs.o
	rm -f station-manager station-manager.o station-manager_funcs.o
	rm -f mystation mystation.o mystation_funcs.o

rm_log:
	rm -f Log.txt
