#ifndef __BLOOM_FILTER__
#define __BLOOM_FILTER__

#include "string_funcs.h"

typedef unsigned int uint;
typedef struct RBT *RBTPtr;

typedef struct BF *BFPtr;

struct BF {

  int* bits;
  unsigned long size;
  const uint numofupdates;
  uint current_numofupdates;

  BF(unsigned long s, uint num_of_updates, const RBTPtr rbT);
  ~BF(void);

  bool needs_recreation(void);
  void increase_updates(void);

  void bitSet(const uint n);
  bool getBit(const uint n);

  bool search(const char* key);
  void insert(const char* key);

  uint fmix32(uint h1);
  uint rotl32(uint x, unsigned char r);
  uint MurmurHash3_x86_32(const char* key, int len, uint seed);

  uint FNV32(const char *s, int len);

  uint hash_func_3(const char* str, int len, uint seed);

  void print_array(void);

};

#endif
