#ifndef __LIST__
#define __LIST__

#include "voter.h"


typedef struct list_node *list_nodePtr;

struct list_node {
  char* id;
  list_nodePtr prev;
  list_nodePtr next;

  list_node(char* at, list_nodePtr p, list_nodePtr n): id(at), prev(p), next(n)
  { /* std::cout << "A new list_node has been created!\n"; */ }

  ~list_node(void)
  { delete next; /* std::cout << "A list_node has been destructed!\n"; */ }
};


typedef struct List *ListPtr;

struct List {
  const int postal_code;
  unsigned int size;
  unsigned int total_people_in_postal_code;
  list_nodePtr head;
  list_nodePtr tail;

  List(const int postcode);
  ~List(void);

  void insert(char* id);
  bool remove(char* id);

  void add_person_to_postal_code(void);
  bool remove_person_from_postal_code(void);
};


#endif
