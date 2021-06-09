#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "strbuf.h"

#ifndef DEBUG
#define DEBUG 0
#endif


int sb_concat(strbuf_t *list, char *str){
    int add_length = strlen(str)+1;
    int new_length = add_length+list->used;
    size_t size;
    if((new_length) > list->length){
        size = new_length-1;
        char *p = realloc(list->data, sizeof(char) * size);
        if (!p) return 1;

        list->data = p;
        list->length = size;
        if (DEBUG) printf("Increased size to %lu\n", size);
    }
    int i;
    int c = 0;
    for(i = list->used-1; i < new_length-1; i++){
        list->data[i] = str[c];
        c++;
    }
    
    list->used = new_length-1;
    return 0;
}

int sb_insert(strbuf_t *list, int index, char item){
    
    
    size_t size;
    if (list->used == list->length || index>=list->length-1) {
        if((list->length*2) <= index){
            size = index+2;
        }
        else{
            size = list->length*2;
        }
        char *p = realloc(list->data, sizeof(char) * size);
        if (!p) return 1;

        list->data = p;
        list->length = size;
        if (DEBUG) printf("Increased size to %lu\n", size);
    }
    //printf("\nIndex: %d\tUsed: %d\n",index,list->used);
    if(index < list->used){
        int i;
        for(i = list->used; i > index; i--){
            list->data[i] = list->data[i-1];
        }
        list->data[index] = item;
        list->used++;
    }
    else{
        list->data[index] = item;
        char temp = list->data[index+1];
        list->data[index+1] = temp;
        list->data[list->used-1] = list->data[index+1];
        list->data[index+1] = '\0';
        
        list->used = index+2;
    }
    
    return 0;
}
int sb_init(strbuf_t *L, size_t length)
{
    L->data = malloc(sizeof(int) * length);
    if (!L->data) return 1;

    L->length = length;

    L->data[0] = '\0';
    L->used = 1;
    return 0;
}

void sb_destroy(strbuf_t *L)
{
    free(L->data);
}


int sb_append(strbuf_t *sb, char item)
{
    if (sb->used == sb->length) {
       size_t size = sb->length * 2;
       char *p = realloc(sb->data, sizeof(char) * size);
       if (!p) return 1;

       sb->data = p;
       sb->length = size;
    }
    
    sb->data[sb->used] = sb->data[sb->used-1];
    sb->data[sb->used-1] = item;

    ++sb->used;

    return 0;
}


int sb_remove(strbuf_t *L, char *item)
{
    if (L->used == 0) return 1;

    --L->used;

    if (item){ 
        *item = L->data[L->used-1];
        L->data[L->used-1] = '\0';
    }

    return 1;
}
char sb_get(strbuf_t *L, int index){
    return L->data[index];
}