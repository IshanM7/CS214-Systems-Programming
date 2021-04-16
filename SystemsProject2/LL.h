#include "strbuf.h"

typedef struct LL {
    char* word;
    double frequency;
    int count;
    struct LL* next;
} LL;


LL* insert(LL* head, strbuf_t* word);
void printList(LL *node);
void freeList(LL *node);
