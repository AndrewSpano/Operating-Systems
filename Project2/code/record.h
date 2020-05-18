#ifndef __RECORD__
#define __RECORD__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILE_NAME_LENGTH 30
#define MAX_COACHES 4
#define MAX_INT_DIGITS 10
#define MAX_LONG_DIGITS 19
#define BATCH 100
#define READ 0
#define WRITE 1


typedef unsigned int uint;

typedef struct Record {
  long id;
  char name[20];
  char surname[20];
  char street_name[20];
  int house_id;
  char city[20];
  char postcode[6];
  float salary;
} Record;



int comparator(const Record *p, const Record *q, int attribute)
{

  switch (attribute) {

    case 1: {
      return p->id - q->id;
    }

    case 2: {
      return strcmp(p->name, q->name);
    }

    case 3: {
      return strcmp(p->surname, q->surname);
    }

    case 4: {
      return strcmp(p->street_name, q->street_name);
    }

    case 5: {
      return p->house_id - q->house_id;
    }

    case 6: {
      return strcmp(p->city, q->city);
    }

    case 7: {
      return strcmp(p->postcode, q->postcode);
    }

    case 8: {
      /* works well because all salaries are actually integers . If they
         differed for like 0.5 then we should use an if statement and return -1
         if p->salary < q->salary, 0 if they are equal, and 1 otherwise. */
      return p->salary - q->salary;
    }

    default: {
      printf("Should have never come here. Error in record.h::comparator.\n");
      exit(0);
    }
  }

}


void swap(Record* rec1, Record* rec2)
{
  Record temp;
  memcpy(&temp, rec1, sizeof(Record));
  memcpy(rec1, rec2, sizeof(Record));
  memcpy(rec2, &temp, sizeof(Record));
}


void print_record(Record record)
{
  printf("%ld %s %s  %s %d %s %s %-9.2f\n", record.id, record.surname, record.name, record.street_name, record.house_id, record.city, record.postcode, record.salary);
}


#endif
