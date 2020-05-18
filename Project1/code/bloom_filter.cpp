#include <iostream>
#include "bloom_filter.h"
#include "red_black_tree.h"

#define SEED 1700146                                                            // The seed for MurmurHash3_x86_32(). It is my Registration Number (sdi).

#define FNV_32_PRIME 16777619                                                   // A prime that seems to work very efficiantly in the FNV hashing
#define FNV_32_OFFSET 2166136261U                                               // An offset that seems to work very efficiantly in the FNV hashing

/* Both parameters were in the site that I have mentioned in the README.txt file.
   Some explanation about them:
       FNV_PRIME_32 is a prime number wchich is equal to 2^24 + 2^8 + 0x93 = 16777619
       FNV_OFFSET_32 == 2166136261 is also an offeset that seems to work perfectly.
       Both these numbers were found trough testing, there is no magic formula. */

using namespace std;



/* Constructor for the Bloom Filter that used the records stored in the Red Black Tree */
BF::BF(unsigned long s, uint num_of_updates, const RBTPtr rbT): size(s), numofupdates(num_of_updates)
{
  bits = new int[size / 32 + 1];
  for (int i = 0; i < size / 32 + 1; i++) {
    bits[i] = 0;
  }
  current_numofupdates = 0;
  rbT->recreate_bloom_filter(this);
  // cout << "A new Bloom Filter has been created via the Red Black Tree!\n";
}



/* Destructor for the Bloom Filter */
BF::~BF(void)
{
  // cout << "A Bloom Filter with size " << size << " is being destroyed!\n";
  delete[] bits;
  size = 0;
}



/* function that returns true if the Bloom Filter needs to be recreated */
bool BF::needs_recreation(void)
{
  return numofupdates == current_numofupdates;
}



/* Increase the current number of updated everytime an insertion/deletion happens */
void BF::increase_updates(void)
{
  current_numofupdates++;
}



/* function that sets the n-th bit to 1 */
void BF::bitSet(const uint n)
{
  /* suppose bits[n / 32] == 0b100111...0110 (32 bits) has a random bit representation.
     We want to set the (n % 32)-th bit of bits[n / 32] to 1.
     We take 1 == 0b00000...0001 where the 31 most significant bits are 0, and the last one is 1.
     Then => 1 << (n % 32) == 0b000...00100...0000 which means that all bits are 0 except for the (n % 32)-th which is 1
     If we "or" bits[n / 32] and 1 << (n % 32) we get bits[n / 32] where its (n % 32)-th bit is 1. Maybe it was already 1. We don't care. */

  bits[n / 32] |= 1 << (n % 32);     // Set the bit at the n-th position in bits[i] using a mask
}



/* function that returns the value of the n-th bit */
bool BF::getBit(const uint n)
{
  /* Using the same logic as above, we isolate the (n % 32)-th bit and this time use an "and" between bits[n / 32] and 1 << (n % 32)
     to see if they give a non zero value. A non zero value will be given iff both the (n % 32)-th bit of bits[n / 32] and
     1 << (n % 32) is 1. Therefore the result depends on bits[n / 32], which is just exactly what we want. */

  if ((bits[n / 32] & (1 << (n % 32))) != 0) {          // we use a mask to isolate the n-th bit and then return the result
    return true;
  }
  else {
    return false;
  }
}




/* Returns true if all 3 bits are set to 1, else return false. True means that the the key is possibly in the registry,
   false means it is definetely NOT in the registry. */
bool BF::search(const char* key)
{
  int len = strlen(key);
  return getBit(MurmurHash3_x86_32(key, len, SEED) % size) && getBit(FNV32(key, len) % size) && getBit(hash_func_3(key, len, SEED) % size);
}




/* function that sets to 1 the index in bits that the hash functions return */
void BF::insert(const char* key)
{
  int len = strlen(key);

  uint digset_1 = MurmurHash3_x86_32(key, len, SEED) % size;
  uint digset_2 = FNV32(key, len) % size;
  uint digset_3 = hash_func_3(key, len, SEED) % size;

  bitSet(digset_1);
  bitSet(digset_2);
  bitSet(digset_3);
}



/* A function that takes as a prameter an unsigned integer and "shuffles" it. I am not sure why these operations are optimal, but they
   seem to minimize the collision rate quite much in the tests that have been made. This function is used in MurmurHash3_x86_32(). */
uint BF::fmix32(uint h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}



/*

   A function that "swaps" two parts of the bit representation of an unsigned interget x. The (32 - r) most significant bits of x
   get shifted to the right, and the r least significant get shifted to the left. Then we return the result. This function is used
   in MurmurHash3_x86_32().

   In example, suppose that
   x == 0b1011010011011 0011010111000101011 (you will see why I put a space in the 13th bit in a moment)
   r == 13

   Then (x << r) == 0b0011010111000101011 0000000000000000000
   and  (x >> (32 - r)) == 0b0000000000000000000 1011010011011

   Therefore (x << r) | (x >> (32 - r)) == 0b0011010111000101011 1011010011011

   The 2 the part of bits that composed x, have swicted places. That is the functianality of rotl32().

*/
uint BF::rotl32(uint x, unsigned char r)
{
  return (x << r) | (x >> (32 - r));
}




/* This function has been explained in the README.txt file */
uint BF::MurmurHash3_x86_32(const char* key, int len, uint seed)
{
  const unsigned char* data = (const unsigned char*) key;
  const int nblocks = len / 4;
  int i = 0;

  uint h1 = seed;

  uint c1 = 0xcc9e2d51;
  uint c2 = 0x1b873593;

  const uint* blocks = (const uint*) (data + nblocks * 4);

  for (i = -nblocks; i; i++) {
    uint k1 = blocks[i];

    k1 = k1 * c1;
    k1 = rotl32(k1, 15);
    k1 = k1 * c2;

    h1 = h1 ^ k1;
    h1 = rotl32(h1, 13);
    h1 = h1 * 5+0xe6546b64;
  }

  const unsigned char* tail = (const unsigned char*) (data + nblocks * 4);

  uint k1 = 0;

  switch (len & 3) {

    case 3:
      k1 = k1 ^ tail[2] << 16;

    case 2:
      k1 = k1 ^ tail[1] << 8;

    case 1:
      k1 = k1 ^ tail[0];
      k1 = k1 * c1;
      k1 = rotl32(k1 , 15);
      k1 = k1 * c2;
      h1 = h1 ^ k1;
  }

  h1 = h1 ^ len;

  h1 = fmix32(h1);

  return h1;
}



/* This function has been explained in the README.txt file */
uint BF::FNV32(const char *key, const int len)
{
    uint digset = FNV_32_OFFSET;                                                // begin the digset from a fixed offset that was found to work well
    for(uint i = 0; i < len; i++) {
        digset = digset ^ (key[i]);                                             // xor next character (byte) into the bottom of the hash
        digset = digset * FNV_32_PRIME;                                         // multiply by a prime number that was found to work well
    }
    return digset;
}




/* This function has been explained in the README.txt file */
uint BF::hash_func_3(const char* str, int len, uint seed)
{
  return MurmurHash3_x86_32(str, len, seed) + 3 * FNV32(str, len);
}




/* A function to print the array just to see the uniformity of the hash functions. */
void BF::print_array(void)
{
  for (int i = 0; i < size / 32 + 1; i++) {
    cout << bits[i] << endl;
  }
}
