# The Worst Allocator

Walloc is a memory allocator for use in WebAssembly applications. It features
all the greats, such as `malloc`, `free`, `calloc`, and `realloc`. While you can use
it just like you would `malloc`, the definitions of user-facing functions are renamed, 
in order to avoid confusion.

## Functions

### `walloc`

Definition: `void *walloc(int size)`

Allocates the number of bytes in `size`, without initializing them,
and returns a pointer to the start of the memory. If `size` is 0 or less,
NULL is returned.

### `wree`

Definition: `void wree(void *ptr)`

Frees the memory pointed to by `ptr`, which must be a pointer passed by
`walloc`. Any pointer that is passed to it which is incorrectly displaced
will cause memory corruption.

### `wcalloc`

Definition: `void *wcalloc(int nmemb, int size)`

Allocates `nmemb` * `size` bytes, initializes them to 0, and returns a pointer to the
start of the memory. If either `nmemb` or `size` are 0 or less, NULL is returned.

### `wrealloc`

Definition: `void *realloc(void *ptr, int size)`

Replaces the memory block at `ptr` with a memory block of at least `size` bytes. Returns
the pointer to the memory region. After that, `ptr` may or may not still point to valid
memory, as `realloc`'s behavior depends on if splitting or coalescing memory is possible.
If `size` is smaller than the original size of the memory block, the contents after the last
byte of the new size may be destroyed.

If `ptr` is NULL, this function will behave like `walloc`. If `size` is less than 1, this function
will be equivalent to calling `wree` on `ptr`.

This function also frees the original block, if the memory is moved. Any additional blocks created
from splitting a larger block are also freed.

## How it works

Most implementations of `malloc` are abstractions on top of existing allocators provided by
operating system kernels, which use techniques
such as paging and have their own means to manage memory during process runtimes. Of course, we
don't have that here, and we don't even know the real size of the heap, which means we can't
pre-partition memory.

In the absence of these features, `walloc` follows a tried and tested approach of just having 
memory be a linked list of blocks which starts at the bottom of the heap. Blocks are created
by asking the heap for more memory and moving the heap pointer forwards, and are coalesced
and split via iterating the list.

Of course, this means a user who allocates memory unwisely while attempting to save it might
end up accomplishing the exact opposite, because they'll end up allocating a byte amount similar
(or even smaller) to the amount already taken up by the block header, which currently hovers around 24 bytes. Don't feel bad for using 1K at a time, you're in a web browser!

Another thing to note is that `walloc` does not align allocations to either a power of 2 or a number
divisible by 2. If you allocate 103 bytes, you will get 103 bytes. This can translate to small
performance losses when it comes to operating on unaligned regions.

It also probably has a couple bugs. Don't be afraid to report them!

## Using

With the "headers" folder in the same directory, merely compile Walloc
in conjunction with the rest of your program, like this:

```C
clang --target=wasm32 -nostdlib -o walloc_test.wasm -Wl,--no-entry -Wl,--export-all -Wall program.c walloc.c
```

## Licensing

The Worst Allocator ("walloc") Copyright (c) 2024 The Walloc Contributors.
All rights reserved.

The Worst Allocator ("walloc") is free software: you can redistribute it 
and/or modify it under the terms of the 
GNU Lesser General Public License, Version 3, as published
by the Free Software Foundation.

The Worst Allocator ("walloc") is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
License for more details.