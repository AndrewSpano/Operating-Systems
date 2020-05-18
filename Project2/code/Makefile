all: mysort coach sorter quicksort heapsort

mysort: coordinator.o
		gcc coordinator.o -o mysort

coach: coach.o handler.o
		gcc coach.o handler.o -o coach

sorter: sorter.o
		gcc sorter.o -o sorter

quicksort: quicksort.o
		gcc quicksort.o -o quicksort

heapsort: heapsort.o
		gcc heapsort.o -o heapsort



heapsort.o: heapsort.c record.h

quicksort.o: quicksort.c record.h

sorter.o: sorter.c record.h

coach.o: coach.c record.h

coordinator.o: coordinator.c record.h

handler.o: handler.c handler.h


clean:
	rm -f mysort handler.o coordinator.o
	rm -f coach coach.o
	rm -f sorter sorter.o
	rm -f quicksort quicksort.o
	rm -f heapsort heapsort.o

rm_files:
	rm -f myinputfile.*
