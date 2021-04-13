#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include "strbuf.h"
#define BUFFSIZE 100
#define QSIZE 8



typedef struct {
	char *data;
	struct queueLL* next;
} queueLL;
typedef struct {
	queueLL *head;
	queueLL *last;
	int count;
	int open;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
} queue_t;



int init(queue_t *Q, queueLL* inHead, queueLL* inLast)
{
	Q->head = inHead;
	Q->last = inLast;
	Q->count = 0;
	Q->open = 1;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	return 0;
}
void printQueue(queue_t* Q){
	queueLL* temp = Q->head;
	while(temp != NULL){
		
		printf("\t%s\n",temp->data);
		
		temp = temp->next;
	}
}
void enqueue(char* path, queue_t* Q){
	if(Q->head == NULL){
		Q->head = malloc(sizeof(char*)*strlen(path));
		Q->head->data = path;
		Q->head->next = NULL;
		Q->last = Q->head;
	}else{
		queueLL* temp = malloc(sizeof(queueLL));
		temp->data = path;
		temp->next = NULL;
		Q->last->next = temp;
		Q->last = temp;
	}
	Q->count++;
}
int destroy(queue_t *Q)
{
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);	
}
int qclose(queue_t *Q)
{
	pthread_mutex_lock(&Q->lock);
	Q->open = 0;
	pthread_cond_broadcast(&Q->read_ready);	
	pthread_mutex_unlock(&Q->lock);
	return 0;
}
typedef struct{
	strbuf_t word;	
	int count;
	double freq;
	struct LL* next;
}LL;
typedef struct{
	LL* head;
	char* name;
	struct WFD* next;
}WFD;


void initWFD(WFD* wfd){
	wfd = malloc(sizeof(WFD));
	wfd->head = NULL;
	wfd->name = 0;
	wfd->next = NULL;

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

LL calcWFD(int fd,char*filename,WFD* wfd){
	LL* current = NULL;
	while(wfd != NULL){
		char* wfdName = wfd->name;
		if(strcmp(wfdName,filename) == 0){
			current = wfd->head;
			break;
		}else{
			wfd = wfd->next;
		}
	}
	
	int bytes_read,i;
	int firstword = 0;
	int consecutive_whitespace = 0;
	int totalWords = 0;
	strbuf_t word;
	sb_init(&word,1);
	char buff[BUFFSIZE];
	while((bytes_read = read(fd,buff,BUFFSIZE)) > 0){
		printf("read: %d\n",bytes_read);
		for(i = 0; i < bytes_read; i++){
			if(isspace(buff[i])){
				if(consecutive_whitespace == 0){//Avoids issues with multiple white spaces					
					totalWords++;
					dump(&word);
					

					sb_destroy(&word);
					sb_init(&word,1);														
				}
				
				consecutive_whitespace++;
			}else{
				if(buff[i] == '-' || !ispunct(buff[i])){
					char c = tolower(buff[i]);
					sb_append(&word,c);
					consecutive_whitespace = 0;
				}
			}
		}
	}
	if(word.used-1 > 0){
		dump(&word);
		totalWords++;
		sb_destroy(&word);
	}
    
    printf("total words: %d\n",totalWords);
}


void printRepo(WFD * wfd){
	while(wfd != NULL){
		printf("|%s| -> ",wfd->name);
		wfd = wfd->next;
	}
	printf("\n");
}

//NEED TO KEEP TRACK OF HOW MANY THREADS ARE ACTIVELY PUTTING INTO DIRECTORY QUEUE FOR DIRECTORY CLOSE FILE CLOSE IS THE SAME
int main(int argc, char **argv)
{
	int directoryThreads = 1;
	int fileThreads = 1;
	int analysisThreads = 1;
	char *fileSuffix = ".txt";
	int i,fileargs,retval;

	for(i = 1; i < argc; i++){
		char* curr = argv[i];		
		//Check for the optional arguments first and cannot repeat
		if(curr[0] == '-'){			
			if(curr[1] == 'd'){
				directoryThreads = atoi(argv[i]+2);
				if(directoryThreads==0){
					perror("Invalid value in -f argument: ");
					exit(1);
				}
			}
			else if(curr[1] == 'f'){				
				fileThreads = atoi(argv[i]+2);
				if(fileThreads==0){
					perror("Invalid value in -f argument: ");
					exit(1);
				}
			}
			else if(curr[1] == 'a'){
				analysisThreads = atoi(argv[i]+2);
				if(analysisThreads==0){
					perror("Invalid value in -a argument: ");
					exit(1);
				}
			}
			else if(curr[1] == 's'){
				
				fileSuffix = argv[i]+2;
				if(fileSuffix==0){
					perror("Invalid value in -s argument: ");
					exit(1);
				}
			}
			else{
				perror("Invalid argument after the - must be d,a,f, or s\n");
				exit(1);
			}
		}else{
			fileargs = i;
			break;
		}
	}

	printf("dThreads: %d\nfThreads: %d\naThreads: %d\nSuffix: %s\n",directoryThreads,fileThreads,analysisThreads,fileSuffix);
	
	queue_t* fileQueue;
	queue_t* directoryQueue;
	queueLL* headFile = NULL;
	queueLL* lastFile = NULL;
	queueLL* headDirectory = NULL;
	queueLL* lastDirectory = NULL;
	WFD* wfdRepo = NULL;
	fileQueue = malloc(sizeof(queue_t));
	directoryQueue = malloc(sizeof(queue_t));
	init(fileQueue,headFile,lastFile);
	init(directoryQueue,headDirectory,lastDirectory);
	int fc =0;
	int dc = 0;
		
	for(fileargs; fileargs < argc; fileargs++){
		char* currfile = argv[fileargs];
		struct stat argcheck;
        stat(currfile,&argcheck);
		if(S_ISREG(argcheck.st_mode)){//argument is a file	
			fc++;
			enqueue(currfile, fileQueue);
			 					
        }else if(S_ISDIR(argcheck.st_mode)){//argument is a directory
			dc++;
			enqueue(currfile, directoryQueue);
		}else{
			retval = 1;
		}
		printf("%s\n",argv[fileargs]);
		//calcWFD(currfile);
	}
	printf("fileQueue: %d\n",fileQueue->count);
	printQueue(fileQueue);
	//printRepo(wfdRepo);	
	
	destroy(fileQueue);
	destroy(directoryQueue);
	return EXIT_SUCCESS;
}