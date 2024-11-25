/*
The Worst Allocator

AUTHOR(s): Vilaverde

The Worst Allocator ("woralloc") Copyright (c) 2024 The WORALLOC Contributors.
All rights reserved.

The Worst Allocator ("woralloc") is free software: you can redistribute it 
and/or modify it under the terms of the 
GNU Lesser General Public License, Version 3, as published
by the Free Software Foundation.

The Worst Allocator ("woralloc") is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
License for more details.
*/    

#include "headers/woralloc_cfg.h"

#ifdef _WORALLOC_WASM
#define NULL 0

extern unsigned char __heap_base;
unsigned char *__heap_base_pt = &__heap_base;

unsigned char *heap_bottom = &__heap_base;
unsigned char *heap_last = NULL;
unsigned char *heap_unexp = &__heap_base;
#else
#include <stdio.h>
unsigned char fakeheap[10240];
unsigned char *heap_bottom = fakeheap;
unsigned char *heap_last = NULL;
unsigned char *heap_unexp = fakeheap;
#endif

typedef unsigned char byte;

enum {
  DEBUG_LASTOP_WORALLOCREARRANGE,
  DEBUG_LASTOP_WORALLOCALLOC,
  DEBUG_LASTOP_FREE,
  DEBUG_LASTOP_COALESCE,
  DEBUG_LASTOP_COALESCE_SINGLEPICK,
  DEBUG_LASTOP_WORALLOCSINGLEPICK,
  DEBUG_LASTOP_REALLOC
};

struct blockheader {  
  struct blockheader *next;

  byte isfree;
  byte lastop;
  int size;
};

struct memzone {
  struct blockheader *first;
  struct blockheader *last;
};

struct memzone find_contiguous(int n) {
  struct memzone toret;
  struct blockheader *bottom = (struct blockheader *) heap_bottom;

  struct blockheader *first_range = bottom;
  toret.first = first_range;
  toret.last = NULL;
  
  int total_sum = 0;
  
  for( ; ; ) {
    if(bottom->isfree == 1) {
      total_sum += bottom->size;
      if(total_sum > n) {
        toret.first = first_range;
        toret.last = bottom;

        return toret;
      } else {
        if(bottom->next != NULL) {
          bottom = bottom->next;
        } else {
          toret.first = NULL;
          toret.last = NULL;

          return toret;
        }
      }
    } else {
      if(bottom->next != NULL) {
        total_sum = 0;
        
        bottom = bottom->next;
        first_range = bottom;
      } else {
        toret.first = NULL;
        toret.last = NULL;

        return toret;
      }
    }
  }
}

struct blockheader * coalesce(struct memzone zone) {
  if((zone.first != NULL && zone.last != NULL) && (zone.first != zone.last)) {
    int size = (int)
      (((((byte *) zone.last) + zone.last->size) - (byte *) zone.first));

    zone.first->size = size;
    if(zone.last->next != NULL) {
      zone.first->next = zone.last->next;
    } else {
      heap_last = (byte *) zone.first;
      zone.first->next = NULL;
    }

    zone.first->lastop = DEBUG_LASTOP_COALESCE;
    zone.first->isfree = 0;

    return zone.first;
  } else {
    zone.first->lastop = DEBUG_LASTOP_COALESCE_SINGLEPICK;
    zone.first->isfree = 0;
    
    return zone.first;
  }

  return NULL;
}

void * woralloc(int n) {
  if(n <= 0) {
    return NULL;
  } else {
    struct memzone zone = find_contiguous(n);

    //See if the contiguous zone exists. If not, actually bump the heap size.
    if(zone.first != NULL && zone.last != NULL) {
      if(zone.first == zone.last) {
        zone.first->lastop = DEBUG_LASTOP_WORALLOCSINGLEPICK;
        zone.first->isfree = 0;
        
        return (void *) (((byte *) zone.first) + sizeof(struct blockheader));
      } else {
        struct blockheader *newarea = coalesce(zone);
        return (void *) (((byte *) newarea) + sizeof(struct blockheader));
      }
    } else {
      struct blockheader *new_header = (struct blockheader *) heap_unexp;
      new_header->next = NULL;
      new_header->isfree = 0;
      new_header->size = n;
      new_header->lastop = DEBUG_LASTOP_WORALLOCALLOC;

      if(heap_last != NULL) {
        struct blockheader *heap_last_struct = (struct blockheader *) heap_last;
        heap_last_struct->next = new_header;
        
        heap_last_struct->lastop = DEBUG_LASTOP_WORALLOCREARRANGE;
      }
      heap_last = (byte *) new_header;

      byte *toret = heap_unexp + sizeof(struct blockheader);
      heap_unexp += n + sizeof(struct blockheader);
    
      return (void *) toret;
    }
  }

  return NULL;
}

void woree(void *ptr) {
  if(ptr != NULL) {
    struct blockheader *header_free =
      (struct blockheader *) (((byte *) ptr) - sizeof(struct blockheader));

    header_free->isfree = 1;

    header_free->lastop = DEBUG_LASTOP_FREE;
  }
}

void *worcalloc(int nmemb, int size) {
  if(size >= 1 && nmemb >= 1) {
    int size_bytes = size * nmemb;

    void *ptr_toret = woralloc(size_bytes);
    if(ptr_toret == NULL) {
      return NULL;
    }

    byte *ptr_copy = (byte *) ptr_toret;
    for(int i = 0; i < size_bytes; i++) {
      ptr_copy[i] = 0;
    }

    return ptr_toret;
  } else {
    return NULL;
  }

  return NULL;
}

void *worealloc(void *ptr, int size) {
  if(ptr == NULL) {
    void *toret = NULL;
    if(size >= 1) {
      toret = woralloc(size);
    }

    return toret;
  } else if(size <= 0 && ptr != NULL) {
    woree(ptr);

    return NULL;
  } else {
    byte *nb = (byte *) ptr;
    struct blockheader *bh = (struct blockheader *) (nb - sizeof(struct blockheader));    

    // Split block
    if(size < bh->size) {
      if((bh->size - size) > (sizeof(struct blockheader) + 1)) { //CHANGEME if we need alignment
        struct blockheader *nbh = (struct blockheader *) (nb + size);

        nbh->size = bh->size - size - sizeof(struct blockheader);
        nbh->next = bh->next;
        nbh->isfree = 1;
        nbh->lastop = DEBUG_LASTOP_REALLOC;
        
        bh->next = nbh;
        bh->size = size;
        bh->lastop = DEBUG_LASTOP_REALLOC;
  
        return (void *) (((byte *) nbh) + sizeof(struct blockheader *));
      } else {
        return ptr;
      }
      // Alloc, copy, and free;
    } else {
      byte *oldptr = nb;
      byte *newptr = (byte *) woralloc(size);

      for(int i = 0; i < size; i++) {
        newptr[i] = oldptr[i];
      }

      woree(ptr);

      return (void *) newptr;
    }
  }

  return NULL;
}

#ifndef _WORALLOC_WASM
void wemdump(){
  int talloc = 0;
  int blocks = 0;
  struct blockheader *bottom = (struct blockheader *) heap_bottom;
  for ( ; ; ) {
    talloc += bottom->size;
    blocks += 1;
    
    printf("!BLOCK @ %016lx!\nfree: %d\nlast op: %d\nsize: %d\nnext: %016lx\n\n",
           (unsigned long) bottom,
           bottom->isfree, bottom->lastop,
           bottom->size,
           (unsigned long) bottom->next);
    if(bottom->next != NULL) {
      bottom = bottom->next;
    } else {
      printf("Total nominal size of alloc'd memory (in bytes): %d\n", talloc);
      printf("Total size between the pointers: %ld\n",
             (unsigned long) (heap_unexp - heap_bottom));
      printf("Total size of the block headers: %ld\n",
             sizeof(struct blockheader) * blocks);
      printf("Total real size, minus the block headers: %ld\n\n",
             (unsigned long) (heap_unexp - heap_bottom)
             - (sizeof(struct blockheader) * blocks));
      break;
    }
  }
}
#endif
