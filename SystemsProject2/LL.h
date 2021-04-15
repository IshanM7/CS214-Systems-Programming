#include "strbuf.h"

typedef struct Node {
    char* word;
    double frequency;
    int count;
    struct Node* next;
} Node;


Node* insert(Node* head, strbuf_t* word);
void printList(Node *node);
void freeList(Node *node);
