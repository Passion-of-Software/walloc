#ifndef _H_WALLOC
    #include "woralloc_cfg.h"

    #define _H_WALLOC

    #ifdef _WORALLOC_WASM
        #define NULL 0
    #else
        #include <stdio.h>

        void wemdump();
    #endif

    void *walloc(int n);

    void wree(void *ptr);

    void *wcalloc(int nmemb, int size);

    void *wrealloc(void *ptr, int size);
#endif