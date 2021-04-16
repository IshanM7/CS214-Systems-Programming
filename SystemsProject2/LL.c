#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LL.h"

LL* insert(LL* head, strbuf_t* buff){
	if(head == NULL){
        head = malloc(sizeof(LL));
        head->word = malloc(buff->used);
        head->count = 1;
        head->next = NULL;
        strcpy(head->word,buff->data);
        //printf("Head1: %s\n",head->word);
        return head;
    }
    LL* ptr = head;
    LL* prev = NULL;
    while(ptr != NULL){
        //printf("Hi: %s ptr: %s\n",buff->data,ptr->word);
        
        
        if(strcmp(buff->data,ptr->word) == 0){
            ptr->count++;
            return head;
        }else if(strcmp(buff->data,ptr->word) < 0){
            LL* new = malloc(sizeof(LL));
            new->count = 1;
            new->word = malloc(buff->used);
            new->next = NULL;
            strcpy(new->word,buff->data);

            if(prev == NULL){
                new->next = ptr;
                head = new;
                //printf("Head2: %s\n",head->word);
                return head;
            }
            prev->next = new;
            new->next = ptr;
            return head;
        }else{
            prev = ptr;
            if(ptr->next == NULL){
                LL* new = malloc(sizeof(LL));
                new->count = 1;
                new->word = malloc(buff->used);
                new->next = NULL;
                strcpy(new->word,buff->data);
                ptr->next = new;
                //printf("Head3: %s\n",head->word);
                return head;
            }
            ptr = ptr->next;
        }
    }
    //printf("Head4: %s\n",head->word);
    return head;
}
void printList(LL *node)
{
  while (node != NULL)
  {
     printf("|%s %d %f|-> ", node->word,node->count,node->frequency);
     node = node->next;
  }
  printf("\n");
}
void freeList (LL* head) {
    LL* tempNode = head;
    while (head != NULL) {
        tempNode = head;
        head = head->next;
        free(tempNode->word);
        free(tempNode);
    }
}