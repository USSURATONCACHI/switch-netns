#ifndef BETTER_C_STD_PRETTIFY_DEBUG_H_
#define BETTER_C_STD_PRETTIFY_DEBUG_H_

#define debugln(...)                                 \
    {                                                \
        printf("LOG (%s:%d): ", __FILE__, __LINE__); \
        printf(__VA_ARGS__);                         \
        printf("\n");                                \
    }
#define debug(...)                                   \
    {                                                \
        printf("LOG (%s:%d): ", __FILE__, __LINE__); \
        printf(__VA_ARGS__);                         \
    }
#define debugc(...)          \
    {                        \
        printf(__VA_ARGS__); \
    }

#endif  // BETTER_C_STD_PRETTIFY_DEBUG_H_
