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
	int activeThreads;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
} queue_t;
typedef struct {
	char* data[QSIZE];
	unsigned count;
	unsigned head;
	int open;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
	pthread_cond_t write_ready;
} fileQueue_t;

int init(queue_t *Q, queueLL* inHead, queueLL* inLast, int activeTs);
void printQueue(queue_t* Q);
void enqueue(char* path, queue_t* Q);
char* dequeue(queue_t* Q);
int destroy(queue_t *Q);
int qclose(queue_t *Q);

int fileDestroy(fileQueue_t *Q);
int fileClose(fileQueue_t *Q);
int fileinit(fileQueue_t *Q);
int fileDequeue(fileQueue_t* Q,char** item);
int fileEnqueue(fileQueue_t* Q,char* item);
void printFile(fileQueue_t* Q);