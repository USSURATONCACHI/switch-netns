#ifndef BETTER_C_STD_PRETTIFY_PANIC_H_
#define BETTER_C_STD_PRETTIFY_PANIC_H_

#include <stdio.h>
#include <stdlib.h>

#define panic(...)                                                        \
    {                                                                     \
        fprintf(stderr, "\n+-+-+-+-+-+-+-+-+-+-\n");                      \
        fprintf(stderr, "Panic in file %s:%d, function %s: \n", __FILE__, \
                __LINE__, __func__);                                      \
        fprintf(stderr, __VA_ARGS__);                                     \
        fprintf(stderr, "\n");                                            \
        exit(-1);                                                         \
    }

#endif  // BETTER_C_STD_PRETTIFY_PANIC_H_
