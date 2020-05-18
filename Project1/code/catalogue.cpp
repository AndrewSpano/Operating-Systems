#include <iostream>
#include "catalogue.h"

using namespace std;


Catalogue::Catalogue(void)
{
  head = NULL;
  size = 0;
  total_people = 0;
  // cout << "A new Catalogue has been created!\n";
}



Catalogue::~Catalogue(void)
{
  // cout << "A Catalogue is being destructed!\n";
  delete head;
  head = NULL;
  tail = NULL;
  size = 0;
}


/* function that inserts the id of a voter that has voted to the Catalogue. We need the postal code of
   the voter in order to place him in the correct list. */
void Catalogue::insert(char* id, const int postal_code)
{
  if (total_people == 0) {

    head = new catalogue_node(postal_code, NULL, NULL);
    head->list->insert(id);
    tail = head;

  } else {

    catalogue_nodePtr temp = head;
    while (temp != NULL) {
      if (temp->list->postal_code != postal_code) {
        temp = temp->next;
      } else {
        break;
      }
    }

    if (temp != NULL) {
      temp->list->insert(id);
    } else {
      tail->next = new catalogue_node(postal_code, tail, NULL);
      tail->next->list->insert(id);
      tail = tail->next;
    }

  }

  size++;
}


/* function that removes a voter from the list with a specific postcode. Returns true if the delete was successful. Else returns false. */
bool Catalogue::remove(char* id, const int postcode)
{
  if (size == 0) {
    cout << "\n\nError in Catalogue::remove(). Catalogue is already empty.\n\n";
    return false;
  }

  catalogue_nodePtr temp = head;
  while (temp != NULL) {
    if (temp->list->postal_code != postcode) {
      temp = temp->next;
    } else {
      break;
    }
  }

  if (temp == NULL) {
    cout << "\n\nError in Catalogue::remove(). Postal code List was not found.\n\n";
    return false;
  }

  if (temp->list->remove(id) != true) {
    cout << "\n\nError in Catalogue::remove(). Voter with id " << id << " was not found.\n\n";
    return false;
  }

  size--;
  total_people--;
  return true;
}



long Catalogue::get_size(void)
{
  return size;
}



long Catalogue::get_voters_in_postcode(int postal_code)
{
  catalogue_nodePtr temp = head;
  while (temp != NULL) {
    if (temp->list->postal_code == postal_code) {
      return temp->list->size;
    }
    temp = temp->next;
  }
  return 0;
}



void Catalogue::print_percentages(void)
{
  catalogue_nodePtr temp = head;
  while (temp != NULL) {
    if (temp->list->total_people_in_postal_code != 0) {
      double percentage = temp->list->size / ((double) temp->list->total_people_in_postal_code);
      cout << "# IN " << temp->list->postal_code << " PERCENTAGE-OF-VOTERS-IS " << percentage << endl;
    }
    temp = temp->next;
  }
}


/* function that increases by 1 the counter of the people in a specific postcode. If the postcode is new,
   and has no people in it, then a new list is created which corresponds to the new postcode. */
void Catalogue::add_person_to_postal_code(const int postcode)
{
  if (total_people == 0) {

    head = new catalogue_node(postcode, NULL, NULL);
    head->list->add_person_to_postal_code();
    tail = head;

  } else {

    catalogue_nodePtr temp = head;
    while (temp != NULL) {
      if (temp->list->postal_code != postcode) {
        temp = temp->next;
      } else {
        break;
      }
    }

    if (temp != NULL) {
      temp->list->add_person_to_postal_code();
    } else {
      tail->next = new catalogue_node(postcode, tail, NULL);
      tail->next->list->add_person_to_postal_code();
      tail = tail->next;
    }

  }

  total_people++;
}


/* decreases the counter of people in a specific postcode. */
bool Catalogue::remove_person_from_postal_code(const int postcode)
{
  if (total_people == 0) {
    cout << "\n\nError in Catalogue::remove_person_from_postal_code(). Total people is zero.\n\n";
    return false;
  }

  catalogue_nodePtr temp = head;
  while (temp != NULL) {
    if (temp->list->postal_code != postcode) {
      temp = temp->next;
    } else {
      break;
    }
  }

  if (temp == NULL) {
    cout << "\n\nError in Catalogue::remove_person_from_postal_code(). Postal code List was not found.\n\n";
    return false;
  }

  if (temp->list->remove_person_from_postal_code() == false) {
    cout << "Error in Catalogue::remove_person_from_postal_code(). Total people in List is zero.\n";
    return false;
  }

  total_people--;
  return true;
}
