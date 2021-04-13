#include <stdlib.h>
typedef struct {
    size_t length;
    size_t used;
    char *data;
} strbuf_t;

int sb_init(strbuf_t *, size_t);
void sb_destroy(strbuf_t *);
int sb_append(strbuf_t *, char item);
int sb_remove(strbuf_t *, char *);
int sb_insert(strbuf_t *list, int index, char item);
char sb_get(strbuf_t* list, int index);
int sb_concat(strbuf_t *sb, char *str);
