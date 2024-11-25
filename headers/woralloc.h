#ifndef _H_WALLOC
    #include "woralloc_cfg.h"

    #define _H_WALLOC

    #ifdef _WORALLOC_WASM
        #define NULL 0
    #else
        #include <stdio.h>

        void wemdump();
    #endif

    void *woralloc(int n);

    void woree(void *ptr);

    void *worcalloc(int nmemb, int size);

    void *worealloc(void *ptr, int size);
#endif