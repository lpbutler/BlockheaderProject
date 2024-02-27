// second allocation is too big to fit
#include <assert.h>
#include <stdlib.h>
#include "p3Heap.h"

int main() {
   assert(init_heap(4096) == 0);
   assert(balloc(2048) != NULL);
   assert(balloc(2047) == NULL);
   exit(0);
}
