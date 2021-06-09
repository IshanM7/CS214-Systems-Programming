#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include "strbuf.h"
#define BACKLOG 5

int running = 1;
pthread_mutex_t lock;

struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};
typedef struct LL{
    char* key;
    char* value;
    struct LL* next;
}LL;

LL* head = NULL;
void *echo(void *arg);
LL* insert(char* key,char* value,int len, FILE* fout);
LL* updateKey(LL* head, char* key,char* value);
LL* deleteKey(LL* head, char* key, int len, FILE* fout);
int keyInLL(LL* head,char* key,int len);
int keyInLL2(LL* head,char* key,int len);
void freeLL(LL* head);
void printLL(LL* head);
int codeGet(FILE* fin, FILE* fout);
int codeSet(FILE* fin, FILE* fout);
int codeDel(FILE* fin, FILE* fout);



void handler(int signal)
{
	running = 0;
}


int main(int argc, char **argv)
{
    
	if (argc != 2) {
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    int p = atoi(argv[1]);
    if(p <= 0){
        printf("port number must be greater than 0\n");
		exit(EXIT_FAILURE);
    }

    struct addrinfo hint, *info_list, *info;
    struct connection *con;
    int error, sfd;
    pthread_t tid;
    char* port = argv[1];
    if(pthread_mutex_init(&lock,NULL)!=0){
        perror("error initilizing lock: ");
        return EXIT_FAILURE;
    }
    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    	
    error = getaddrinfo(NULL, port, &hint, &info_list);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    
    for (info = info_list; info != NULL; info = info->ai_next) {
        sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        
        // if we couldn't create the socket, try the next method
        if (sfd == -1) {
            continue;
        }

        
        if ((bind(sfd, info->ai_addr, info->ai_addrlen) == 0) &&
            (listen(sfd, BACKLOG) == 0)) {
            break;
        }
        
        close(sfd);
    }

    if (info == NULL) {        
        fprintf(stderr, "Could not bind\n");
        pthread_mutex_destroy(&lock);

        return -1;
    }

    freeaddrinfo(info_list);

	struct sigaction act;
	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGINT, &act, NULL);
	
	sigset_t mask;
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);


    
    printf("Waiting for connection\n");
	while (running) {
    	
		con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);

        
        
        con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
        
        if (con->fd == -1) {
            perror("accept");
            continue;
        }
                
        error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
        if (error != 0) {
        	fprintf(stderr, "sigmask: %s\n", strerror(error));
        	abort();
        }

		
        error = pthread_create(&tid, NULL, echo, con);
		
        if (error != 0) {
            fprintf(stderr, "Unable to create thread: %d\n", error);
            close(con->fd);
            free(con);
            continue;
        }
		
        pthread_detach(tid);
        
        error = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
        if (error != 0) {
        	fprintf(stderr, "sigmask: %s\n", strerror(error));
        	abort();
        }
    
    }

	puts("No longer listening.");
	pthread_detach(pthread_self());
    free(con);
    freeLL(head);
    pthread_mutex_destroy(&lock);
	pthread_exit(NULL);
    return EXIT_SUCCESS;
}

void *echo(void *arg)
{
    char host[100], port[10];
    struct connection *c = (struct connection *) arg;
    int error;

	
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    	
    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(c->fd);
        return NULL;
    }
    
    printf("[%s:%s] connection\n", host, port);


    FILE* fin = fdopen(dup(c->fd),"r");
    FILE* fout = fdopen(c->fd,"w");
    
    int final = 0;
    char* code = malloc(10);
    int func = 0;
    int errorGet = 0;
    int errorSet = 0; 
    int errorDel = 0;
    while(fscanf(fin,"%s\n",code) > 0){
        if(strcmp(code,"GET") == 0){
            func = 1;
        }else if(strcmp(code,"SET") == 0){
            func = 2;
        }else if(strcmp(code,"DEL") == 0){
            func = 3;
        }else{            
            printf("BAD INPUT CLOSING CONNECTION\n");
            fprintf(fout,"ERR\nBAD\n");
            fflush(fout);
            fclose(fin);
            fclose(fout);
            close(c->fd);
            free(c);
            free(code);
            pthread_mutex_unlock(&lock);
            final = 1;
            return NULL;
        }
        if(func == 1){
            errorGet = codeGet(fin,fout);            
        }
        if(func == 2){
            errorSet = codeSet(fin,fout); 
        }
        if(func == 3){
            errorDel = codeDel(fin,fout); 
        }
        if(errorGet == -1 || errorSet == -1 || errorDel == -1){
            printf("THERE WAS AN ERROR CLOSING CONNECTION\n");
            fclose(fin);
            fclose(fout);
            close(c->fd);
            free(c);
            free(code);
            
            pthread_mutex_unlock(&lock);
            return NULL;            
        }
    }
    if(final == 1){
        printf("BAd input\n");
    }


    printf("[%s:%s] got EOF\n", host, port);

    close(c->fd);
    free(c);
    free(code);
    return NULL;
}
LL* insert(char* key,char* value, int len,FILE* fout){
    LL* new = malloc(sizeof(LL));    
    if(new == NULL){
        fprintf(fout,"SRV\n");
        fflush(fout);
        return NULL;
    }
    new->key = malloc(strlen(key)+1);
    new->value = malloc(strlen(value)+1);
    strcpy(new->key,key);
    strcpy(new->value,value);
    if(head == NULL){
        head = new;
        head->next = NULL;
        return head;   
    }else{
        LL* ptr = head;
        LL* prev = NULL;
        while(ptr != NULL){

            if(strncmp(key,ptr->key,len) < 0){
                if(prev == NULL){
                    new->next = ptr; 
                    head = new;
                    return head;
                }else{
                    prev->next = new;
                    new->next = ptr;
                    return head;
                }
            }else{

                prev = ptr;
                if(ptr->next == NULL){

                    ptr->next = new;
                    new->next = NULL;    
                    return head;
                }
                ptr = ptr->next;
            }
        }
        //new->next = head;
        return head;
    }
}
int keyInLL(LL* head,char* key,int len){
    int ret = 0;
    LL* ptr = head;
    while(ptr != NULL){
        //printf("ptr: %ld key: %d\n",strlen(ptr->key),len);
        if((strlen(ptr->key) == len) && (strncmp(ptr->key,key,len) == 0)){
            return 1;
        }
        ptr = ptr->next;
    }
    return ret;

}
LL* updateKey(LL* head, char* key, char* value){
    LL* ptr = head;
    while(ptr != NULL){
        if(strcmp(ptr->key,key) == 0){            
            ptr->value = realloc(ptr->value, strlen(value)+1);
            strcpy(ptr->value,value);
            break;
        }
        ptr = ptr->next;
    }
    return head;
}
LL* deleteKey(LL* head, char* key,int len, FILE* fout){
    LL* ptr = head;
    LL* prev = NULL;
    while(ptr != NULL){
        if((strlen(ptr->key) == len-1) && (strncmp(ptr->key,key,len-1) == 0)){
            break;
        }             
        prev = ptr;
        ptr = ptr->next;
    }
    if(prev == NULL){
        fprintf(fout,"OKD\n%ld\n%s",strlen(ptr->value),ptr->value); 
        fflush(fout);
        head = ptr->next;
        free(ptr->key);
        free(ptr->value);
        free(ptr);                
        return head;
    }else{
        fprintf(fout,"OKD\n%ld\n%s",strlen(ptr->value),ptr->value); 
        fflush(fout);
        prev->next = ptr->next;
        free(ptr->key);
        free(ptr->value);
        free(ptr);
        return head;
    }
}


int codeGet(FILE* fin, FILE* fout){
    pthread_mutex_lock(&lock);
    int ret = 0;
    int len;
    int get = 0;
    int err = fscanf(fin,"%d\n",&len);
    if(err <= 0 || len <= 0){        
        fprintf(fout,"ERR\nBAD\n");
        fflush(fout);  
        //printf("THERE WAS AN ERROR CLOSING CONNECTION\n");
        return -1;
    }
    strbuf_t key;
    sb_init(&key,len+1);

    for(int i = 0; i < len; i++){
        char c = getc(fin);
        if(c == '\n' && i != len-1){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout);
            sb_destroy(&key);    
            return -1;
        }
        if(c != '\n' && i == len-1){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout);
            sb_destroy(&key);
            return -1;
        }
        if(c == EOF || c == '\0'){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout); 
            sb_destroy(&key);
            return -1;
        }
        sb_append(&key, c);
    }    
    
    LL* ptr = head;
    while(ptr!=NULL){    
        //printf("ptr:%ld key:%d\n",strlen(ptr->key),len-1);    
        if((strlen(ptr->key) == len-1) && (strncmp(ptr->key,key.data,len-1) == 0)){            
            get = 1;
            fprintf(fout,"OKG\n%ld\n%s",strlen(ptr->value),ptr->value);
            fflush(fout);
            break;
        }
        ptr = ptr->next;
    }
    if(get == 0){
        fprintf(fout,"KNF\n"); 
        fflush(fout);
    } 
    sb_destroy(&key);
    pthread_mutex_unlock(&lock);
    return ret; 
}

int codeSet(FILE* fin, FILE* fout){
    pthread_mutex_lock(&lock);
    int ret = 0;
    int len;
    
    int err = fscanf(fin,"%d\n",&len);
    if(err <= 0 || len <= 0){        
        fprintf(fout,"ERR\nBAD\n");
        fflush(fout);        
        //printf("THERE WAS A SET ERROR CLOSING CONNECTION\n");
        return -1;
    }
    strbuf_t key;
    strbuf_t val;
    sb_init(&val,len+1);
    sb_init(&key,len+1);
    int firstarg = 0;
    int keylen = 0;
    for(int i = 0; i < len; i++){
        char c = getc(fin);
        if(c == '\n' && i != len-1){
            if(firstarg == 0){
                firstarg = 1;//We have read the key we want to set
                continue;
            }else{//Input too short for first word
                fprintf(fout,"ERR\nLEN\n");
                fflush(fout);
                sb_destroy(&key);
                sb_destroy(&val);
                return -1;
            }            
        }
        if(c != '\n' && i == len-1){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout);
            sb_destroy(&key);
            sb_destroy(&val);
            return -1;
        }
        if(c == EOF){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout);
            sb_destroy(&key);
            sb_destroy(&val);
            return -1;
        }
        if(firstarg == 0){//create first arg
            sb_append(&key,c);
            keylen++;
        }else{//create second arg
            sb_append(&val,c);
        }
    }
    if(val.used <= 1){
        fprintf(fout,"ERR\nLEN\n");
        fflush(fout);
        sb_destroy(&key);
        sb_destroy(&val);
        return -1;
    }
    fprintf(fout,"OKS\n");
    fflush(fout);
    int update = 0;
    update = keyInLL(head,key.data,keylen);    
    if(update == 1){        
        head = updateKey(head, key.data,val.data);
    }else{        
        head = insert(key.data,val.data,keylen,fout);
    }
    printLL(head);
    sb_destroy(&key);
    sb_destroy(&val);
    pthread_mutex_unlock(&lock);
    return ret;

}
int codeDel(FILE* fin, FILE* fout){
    pthread_mutex_lock(&lock);
    int ret = 0;
    int len;
    
    int err = fscanf(fin,"%d\n",&len);
    if(err <= 0 || len <= 0){        
        fprintf(fout,"ERR\nBAD\n");
        fflush(fout);       
        //printf("THERE WAS AN ERROR CLOSING CONNECTION\n");
        return -1;
    }
    strbuf_t key;
    sb_init(&key,len+1);
    for(int i = 0; i < len; i++){
        char c = getc(fin);
        if(c == '\n' && i != len-1){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout);
            sb_destroy(&key);    
            return -1;
        }
        if(c != '\n' && i == len-1){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout);
            sb_destroy(&key);
            return -1;
        }
        if(c == EOF){
            fprintf(fout,"ERR\nLEN\n");
            fflush(fout);
            sb_destroy(&key);
            return -1;
        }
        sb_append(&key, c);
    }
    int exists = 0; 
    LL* ptr = head;
    //LL* prev = NULL;
    while(ptr!=NULL){    
        //printf("del ptr:%ld key:%d\n",strlen(ptr->key),len-1);    
        if((strlen(ptr->key) == len-1) && (strncmp(ptr->key,key.data,len-1) == 0)){            
            exists = 1;           
            break;
        }
        //prev = ptr;
        ptr = ptr->next;
    }    
    if(exists == 1){
        head = deleteKey(head,key.data,len,fout);        
        //fprintf(fout,"OKD\n"); 
        //fflush(fout);
    }else{
        fprintf(fout,"KNF\n"); 
        fflush(fout);
    }
    printLL(head);
    sb_destroy(&key);
    pthread_mutex_unlock(&lock);
    return ret;

}
void printLL(LL* head){
    printf("List: \n");
    while(head != NULL){
        printf("%s -> %s",head->key,head->value);
        head = head->next;
    }
    printf("\n");
}
void dump(strbuf_t *list){
    int i;
    printf("[%2lu/%2lu] ",list->used, list->length);
    for(i = 0; i < list->used; i++){
        printf("%c ",(unsigned char) sb_get(list,i));
    }
    for(; i < list->length; i++){
        printf("__ ");
    }
    printf("\n");
}

void freeLL(LL* head){
    while(head != NULL){
        LL* temp = head;
        head = head->next;
        free(temp->key);
        free(temp->value);
        free(temp);
    }
}
