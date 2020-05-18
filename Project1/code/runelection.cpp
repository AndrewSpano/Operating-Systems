#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "main_funcs.h"

#define DEFAULT_NUMOFUPDATES 5

using namespace std;



int main(int argc, char* argv[])
{
  if (argc != 3 && argc != 5 && argc != 7) {                                             // -o outile and -n numofupdates are optionary
    cout << "Error, wrong amount of command line arguments was given.\n";
    exit(1);
  }

  char* inputfile = NULL;
  char* outfile = NULL;
  unsigned int numofupdates = DEFAULT_NUMOFUPDATES;

  if (get_paremeters(&inputfile, &outfile, &numofupdates, argc, argv) == false) {        // returns false is not -i inputfile was found
    cout << "Could not find an input file. Please try the following format in any order: ./runelection -i inputfile -o outfile -n numofupdates\n";
    exit(36);
  }


  BFPtr bf = NULL;                                                                       // The size of the Bloom Filter depends on the number of records
  RBTPtr rbT = new RBT();
  CataloguePtr catalogue = new Catalogue();
  unsigned int num_of_records = 0;

  get_voters(&bf, rbT, catalogue, inputfile, &num_of_records, numofupdates);             // opens registry and creates the structs


  unsigned short option = 0;                                                             // option is used to decide which case corresponds to the input the user gave
  char buf[256];                                                                         // buf is used to read the input from the user

  FILE* output = NULL;                                                                   // output file
  bool outfile_flag = false;
  if (outfile != NULL) {                                                                 // if it is NULL, it means that not -o outfile parameter was given
    output = fopen(outfile, "w");
    outfile_flag = true;
    if (output == NULL) {                                                                // if it is NULL, for some reason we failed to open the outfile
      cout << "There was a problem opening the outfile.\n";
      exit(3);
    }
  }


  do {

    cout << "> ";
    fgets(buf, sizeof buf, stdin);                                                       // gets input from user from stdin
    option = get_option(buf);                                                            // translates input into a case for switch ()

    char* key = NULL;                                                                    // key is used to denote the second string in the buffer, which is usually a voter's id

    switch (option) {

      case 1:

        {
          key = new char[256];
          if (get_nth_string(key, buf, 2) == false) {                                    // if get_nth_string() returns false, then no second string was found, therefore proceed
            delete[] key;                                                                // free the memory allocated to read the second string from buf
            break;
          }

          if (bf->search(key) == true) {                                                 // if it was found or not, print corresponding messages
            cout << "# KEY " << key << " POSSIBLY-IN REGISTRY";
          } else {
            cout << "# KEY " << key << " Not-in-LBF";
          }
          cout << endl;
          delete[] key;                                                                  // free the memory allocated to read the second string from buf
          key = NULL;
        }

        break;

      case 2:

        {
          key = new char[256];
          if (get_nth_string(key, buf, 2) == false) {                                    // if get_nth_string() returns false, then no second string was found, therefore proceed
            delete[] key;                                                                // free the memory allocated to read the second string from buf
            break;
          }

          if (rbT->search(key) == true) {                                                // if it was found or not, print corresponding messages
            cout << "# KEY " << key << " FOUND-IN-RBT";
          } else {
            cout << "# KEY " << key << " NOT-IN-RBT";
          }
          cout << endl;
          delete[] key;                                                                  // free the memory allocated to read the second string from buf
          key = NULL;
        }

        break;


      case 3:

        {
          key = new char[256];
          if (get_nth_string(key, buf, 2) == false) {                                    // if get_nth_string() returns false, then no second string was found, therefore proceed
            delete[] key;                                                                // free the memory allocated to read the second string from buf
            break;
          }

          if (rbT->search(key) == true) {                                                // print corresponding messages
            if (outfile_flag == true) {
              fprintf(output, "- REC-WITH %s EXISTS\n", key);
            } else {
              cout << "- REC-WITH " << key << " EXISTS\n";
            }
            delete[] key;                                                                // voter with same id (key) already existed, so delete key
            break;
          } else {

            /* allocate memory for next info about the new voter to be inserted */
            char* firstname = new char[256];
            char* lastname = new char[256];
            char* year = new char[256];
            char* sex = new char[256];
            char* postcode = new char[256];

            /* if get_record() returns false, it means that the parameters given in the input were not enough, therefore delete them and proceed */
            if (get_record(key, firstname, lastname, year, sex, postcode, buf) == false) {
              delete[] key;
              key = NULL;
              delete[] firstname;
              delete[] lastname;
              delete[] sex;
              delete[] year;
              delete[] postcode;
              break;
            }

            /* create a new voter from the parameters given in the input, and add him to the structs */
            voter* v = new voter(key, firstname, lastname, atoi(year), (*sex == 'M'), atoi(postcode));
            bf->insert(key);
            bf->increase_updates();
            rbT->insert(v);
            catalogue->add_person_to_postal_code(atoi(postcode));

            /* recreate Bloom Filter if needed */
            if (bf->needs_recreation() == true) {
              delete bf;
              unsigned int bf_size = get_BF_size(rbT->size);
              bf = new BF(bf_size, numofupdates, rbT);
            }

            cout << "# REC-WITH " << key << " INSERTED-IN-BF-RBT";

            delete[] sex;                                                                 // free the memory that is not being used anymore
            delete[] postcode;
            delete[] year;
          }

          cout << endl;
          key = NULL;
        }

        break;

      case 4:

        {
          key = new char[256];
          if (get_nth_string(key, buf, 2) == false) {                                    // if get_nth_string() returns false, then no second string was found, therefore proceed
            delete[] key;                                                                // free the memory allocated to read the second string from buf
            break;
          }

          if (bf->search(key) == false) {                                                // print corresponding messages
            cout << "# REC-WITH " << key << " NOT-in-structs";
          } else {
            if (rbT->search(key) == true) {
              voter* temp = rbT->get_voter(key);                                         // RBT::get_voter() returns a pointer to the voter with id = key. Use that pointer to print voter's info.
              cout << "# REC IS: " << temp->id << " " << temp->name << " " << temp->surname << " " << temp->age << " " << (temp->sex ? 'M' : 'F') << " " << temp->postal_code;
              temp = NULL;
            } else {
              cout << "# REC-WITH " << key << " NOT-in-structs";
            }
          }
          cout << endl;
          delete[] key;                                                                  // free the memory allocated to read the second string from buf
          key = NULL;
        }

        break;


      case 5:

        {
          key = new char[256];
          if (get_nth_string(key, buf, 2) == false) {                                    // if get_nth_string() returns false, then no second string was found, therefore proceed
            delete[] key;                                                                // free the memory allocated to read the second string from buf
            break;
          }

          if (rbT->search(key) == true) {                                                // search the tree to see if a voter with id == key exists
            voter* temp = rbT->get_voter(key);
            if (rbT->has_voted(key) == true) {
              catalogue->remove(temp->id, temp->postal_code);                            // if he exists and has voted, then we need to also remove him from the catalogue
            }
            temp = NULL;
            rbT->remove(key);                                                            // remove him from the Red Black Tree
            bf->increase_updates();

            /* recreate bloom filter if needed */
            if (bf->needs_recreation() == true) {
              delete bf;
              unsigned int bf_size = get_BF_size(rbT->size);
              bf = new BF(bf_size, numofupdates, rbT);
            }

            cout << "# DELETED " << key << " FROM-structs";
            if (outfile_flag == true) {
              cout << endl;
            }

          } else {                                                                       // voter was not found in the Red Black Tree, therefore he does not exit

            if (outfile_flag == true) {
              fprintf(output, "- KEY %s NOT-in-structs\n", key);
            } else {
              cout << "- KEY " << key << " NOT-in-structs";
            }

          }

          if (outfile_flag == false) {
            cout << endl;
          }

          delete[] key;                                                                  // free the memory allocated to read the second string from buf
          key = NULL;
        }

        break;


      case 6:

        {
          key = new char[256];
          if (get_nth_string(key, buf, 2) == false) {                                    // if get_nth_string() returns false, then no second string was found, therefore proceed
            delete[] key;                                                                // free the memory allocated to read the second string from buf
            break;
          }

          if (rbT->search(key) == false) {                                               // search key in Red Black Tree and print corresponding messages
            if (outfile_flag == true) {
              fprintf(output, "- REC-WITH %s NOT-in-structs\n", key);
            } else {
              cout << "- REC-WITH " << key << " NOT-in-structs\n";
            }
          } else {
            if (rbT->has_voted(key) == true) {                                           // if voter has already voted, then print message and proceed
              cout << "# REC-WITH " << key << " ALREADY-VOTED\n";
            } else {
              voter* temp_voter = rbT->vote(key);                                        // RBT::vote(key) makes the voter with id == key field hasvoted = true, and returns the whole voter
              catalogue->insert(temp_voter->id, temp_voter->postal_code);                // we have a new voter who was voted, so insert him in the catalogue
              temp_voter = NULL;
              cout << "# REC-WITH " << key << " SET-VOTED\n";

            }
          }

          delete[] key;                                                                  // free the memory allocated to read the second string from buf
          key = NULL;
        }

        break;

      case 7:

        {
          char* fileofkeys = new char[256];                                              // second string in buf is the name of the file with the id's to set them voted

          if (get_nth_string(fileofkeys, buf, 2) == true) {                              // if get_nth_string() is true, there exists a second string that is the file's name

            FILE* vote_file;
            vote_file = fopen(fileofkeys, "r");

            if (vote_file == NULL) {                                                     // if vote_file is NULL for some reason we failed to open the fileofkeys, so we proceed
              delete[] fileofkeys;                                                       // free the memory that was allocated to read the name of the file
              break;
            }

            key = new char[256];                                                         // used to get the IDs in the file

            while (feof(vote_file) == 0) {                                               // while we have not reached the end of file

              fscanf(vote_file, "%s", key);                                              // get the next id (key)

              /* this part is just like case 6, no need to explain it again */
              if (rbT->search(key) == false) {
                if (outfile_flag == true) {
                  fprintf(output, "- REC-WITH %s NOT-in-structs\n", key);
                } else {
                  cout << "- REC-WITH " << key << " NOT-in-structs\n";
                }
              } else {
                if (rbT->has_voted(key) == true) {
                  cout << "# REC-WITH " << key << " ALREADY-VOTED\n";
                } else {
                  voter* temp_voter = rbT->vote(key);
                  catalogue->insert(temp_voter->id, temp_voter->postal_code);
                  temp_voter = NULL;
                  cout << "# REC-WITH " << key << " SET-VOTED\n";
                }
              }

            }

            delete[] key;                                                                // free the memory allocated to read the second string from buf
            key = NULL;
            fclose(vote_file);
          }
          delete[] fileofkeys;                                                           // free the memoty that was used to read the name of the file
        }

        break;

      case 8:

        {
          cout << "# NUMBER " << catalogue->get_size() << endl;                          // catalogue->get_size() returns the number of people who have voted so far (who are inside the catalogue)
        }

        break;

      case 9:

        {
          char* postal_code = new char[256];                                             // postal_code is used to get the seconds string from the buffer which should be a postal code
          if (get_nth_string(postal_code, buf, 2) == true) {                             // if there is a second string in the buffer, proceed
            cout << "# IN " << atoi(postal_code) << " VOTERS-ARE " << catalogue->get_voters_in_postcode(atoi(postal_code));
          }
          cout << endl;

          delete[] postal_code;                                                          // delete the allocated memory to read the postal code
        }

        break;

      case 10:

        {
          catalogue->print_percentages();                                                // pretty much straightforward
        }

        break;

      case 11:

        {
          cout << "- exit program\n";                                                    // also pretty much straightforward
        }

        break;

      default:

        break;
    }

  } while (option >= 0 && option <= 10);                                                 // option is 0 if get a non valid input from the buffer. Keep iterating until opion == 11.

  if (outfile != NULL) {                                                                 // if an outfile exists, print the current state of the registry (all voters)
    rbT->print(output);
    fclose(output);
  }

  /* Free the memory that we allocated. Each destructor invokes the destructor of it's fields until all data is free'd. */
  delete bf;
  delete rbT;
  delete catalogue;

  delete[] inputfile;
  delete[] outfile;

  return 0;
}
