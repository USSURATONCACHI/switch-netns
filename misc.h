#ifndef BETTER_C_STD_PRETTIFY_MISC_H_
#define BETTER_C_STD_PRETTIFY_MISC_H_

#define unreachable() __builtin_unreachable()
#define forget(value)

// Suppresses warning about unused variable.
//
// Removing argument names, like this...:
//    void f(void*)
//    ------------^
// ...is not an official thing in C11.
#define unused(var) var = var

#define CONCAT2(s1, s2) s1##s2
#define CONCAT(s1, s2) CONCAT2(s1, s2)

#define STR2(s) #s
#define STR(s) STR2(s)

#define LEN(array) (sizeof(array) / sizeof(array[0]))

#define SWAP(type, a, b) \
    {                    \
        type tmp = (a);  \
        (a) = (b);       \
        (b) = tmp;       \
    }

#define foreach_extract(item, vec, condition, code)          \
    {                                                        \
        int i;                                               \
        for (i = 0; i < (vec).length and (condition); i++) { \
            item = (vec).data[i];                            \
            code;                                            \
        }                                                    \
                                                             \
        (vec).length -= i;                                   \
        for (int j = 0; j < (vec).length; j++)               \
            (vec).data[j] = (vec).data[j + i];               \
    }

#endif  // BETTER_C_STD_PRETTIFY_MISC_H_
