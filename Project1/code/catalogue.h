#ifndef __CATALOGUE__
#define __CATALOGUE__

#include "list.h"


typedef struct catalogue_node *catalogue_nodePtr;

struct catalogue_node {
  ListPtr list;
  catalogue_nodePtr prev;
  catalogue_nodePtr next;

  catalogue_node(const int postcode, catalogue_nodePtr p, catalogue_nodePtr n): prev(p), next(n)
  { list = new List(postcode); /* std::cout << "A new catalogue_node has been created!\n"; */ }

  ~catalogue_node(void)
  { delete list; delete next; /* std::cout << "A catalogue_node is being destructed!\n"; */ }
};


typedef struct Catalogue *CataloguePtr;

struct Catalogue {
  catalogue_nodePtr head;
  catalogue_nodePtr tail;
  unsigned int size;
  unsigned int total_people;

  Catalogue(void);
  ~Catalogue(void);

  void insert(char* id, const int postal_code);
  bool remove(char* id, const int postcode);

  long get_size(void);
  long get_voters_in_postcode(int postal_code);
  void print_percentages(void);

  void add_person_to_postal_code(const int postcode);
  bool remove_person_from_postal_code(const int postcode);
};


#endif
