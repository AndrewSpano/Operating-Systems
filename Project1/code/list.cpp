#include <iostream>
#include "list.h"

using namespace std;


List::List(const int postcode): postal_code(postcode)
{
  total_people_in_postal_code = 0;
  size = 0;
  head = NULL;
  tail = NULL;
  // cout << "A new List has been created!\n";
}


List::~List(void)
{
  // cout << "A List is being destructed!\n";
  delete head;
  head = tail = NULL;
  size = 0;
}



void List::insert(char* id)
{

  if (size == 0) {
    head = new list_node(id, NULL, NULL);
    tail = head;
  } else {
    tail->next = new list_node(id, tail, NULL);
    tail = tail->next;
  }

  size++;
}



bool List::remove(char* id)
{
  if (size == 0) {
    cout << "\n\nError in List::remove(). List is already empty.\n\n";
    return false;
  }

  if (size == 1) {

    delete head;
    head = NULL;
    tail = NULL;

  } else {

    list_nodePtr temp = head;
    while (temp != NULL) {
      if (strcmp(temp->id, id) != 0) {
        temp = temp->next;
      } else {
        break;
      }
    }

    if (temp == NULL) {
      cout << "\n\nError in List::remove(). Voter with id " << id << " was not found/\n\n";
      return false;
    }

    if (temp->prev == NULL) {                              // if this condition is true, then the item we want to remove is the head
      head = head->next;
      head->prev->next = NULL;                             // the destructor of list_node destructs the next node, but we don't want that now
      delete head->prev;
      head->prev = NULL;
    } else if (temp->next == NULL) {                       // if this condition is true, then the item we want to remove is the tail
      tail = tail->prev;
      delete tail->next;
      tail->next = NULL;
    } else {                                               // else, normally remove the item from the list and delete it
      temp->prev->next = temp->next;
      temp->next->prev = temp->prev;

      temp->prev = NULL;
      temp->next = NULL;                                   // the destructor of list_node deletes list_node->next. But we don't want the next element to be deleted, so we set temp->next to NULL
      delete temp;
    }

  }

  total_people_in_postal_code--;
  size--;
  return true;
}



/* increases by 1 the counter total_people_in_postal_code */
void List::add_person_to_postal_code(void)
{
  total_people_in_postal_code++;
}



/* Decreases by 1 the counter total_people_in_postal_code */
bool List::remove_person_from_postal_code(void)
{
  if (total_people_in_postal_code == 0) {
    cout << "Error in List::remove_person_from_postal_code(). total_people_in_postal_code is already zero.\n";
    return false;
  }
  total_people_in_postal_code--;
  return true;
}
