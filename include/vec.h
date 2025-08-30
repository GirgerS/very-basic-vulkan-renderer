#ifndef CUSTOM_DYN_ARR
#define CUSTOM_DYN_ARR

// Vector implementation should have the following structure:
/*
    typedef struct {
        T *elems;
        size_t len;
        size_t capacity;
    } ExampleVector;

    where T is a Generic Type 
*/

#define vec_append(arr, elem, err)  \
    do { \
        if ((arr)->len >= (arr)->capacity) { \
            (arr)->data = realloc((arr)->data, (arr)->capacity * 2 * sizeof(*(arr))); \
            assert((arr)->data != NULL && "Error: Out of memory"); \
        } \
        (arr)->data[((arr)->len)++] = elem; \
    } while(0)
#endif

#define vec_new(arr, cap, err)   \
    do {                                                     \
        (arr)->data = malloc((cap) * sizeof(*(arr)));                \
        assert((arr)->data != NULL && "Error: Out of memory"); \
        (arr)->capacity = cap;                                     \
        (arr)->len = 0;                                           \
    } while(0)


