#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "Queue.h"

void printFile(fileQueue_t* Q) {
	
    for (int i = 0; i < Q->count; i++) {
        printf("%s\t", Q->data[i]);
    }
	printf("\n");
    
}

int fileinit(fileQueue_t *Q){
	Q->count = 0;
	Q->head = 0;
	Q->open = 1;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	pthread_cond_init(&Q->write_ready, NULL);
	
	return 0;
}

int fileDestroy(fileQueue_t *Q){
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	pthread_cond_destroy(&Q->write_ready);

	return 0;
}
int fileClose(fileQueue_t *Q){
	pthread_mutex_lock(&Q->lock);
	Q->open = 0;
	pthread_cond_broadcast(&Q->read_ready);
	pthread_cond_broadcast(&Q->write_ready);
	pthread_mutex_unlock(&Q->lock);	

	return 0;
}
int fileDequeue(fileQueue_t* Q,char** item){
	pthread_mutex_lock(&Q->lock);


	while (Q->count == 0 && Q->open) {
		pthread_cond_wait(&Q->read_ready, &Q->lock);
	}

	if (Q->count == 0) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
    // printf("head %s\n", Q->data[Q->head]);
    
    *item = malloc(strlen(Q->data[Q->head]) + 1);
    strcpy(*item, Q->data[Q->head]);
	free(Q->data[Q->head]);
    --Q->count;
    ++Q->head;
    if (Q->head == QSIZE) Q->head = 0;
	
	pthread_cond_signal(&Q->write_ready);
	
	pthread_mutex_unlock(&Q->lock);


    // printf("str %s\n", *item);
	
	return 0;
}
int fileEnqueue(fileQueue_t* Q,char* item){
	pthread_mutex_lock(&Q->lock);
	while (Q->count == QSIZE && Q->open) {
		pthread_cond_wait(&Q->write_ready, &Q->lock);
	}

	if (!Q->open) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
	unsigned i = Q->head + Q->count;
	if (i >= QSIZE) i -= QSIZE;
	
	// Q->data[i] = item;
    int len = strlen(item) + 1;
    Q->data[i] = malloc(len);
    strcpy(Q->data[i], item);
	printf("enqueue: %s\n",Q->data[i]);
	++Q->count;
	
	pthread_cond_signal(&Q->read_ready);
	
	pthread_mutex_unlock(&Q->lock);

	
	return 0;
}


int init(queue_t *Q, queueLL* inHead, queueLL* inLast, int activeTs)
{
	Q->head = inHead;
	Q->last = inLast;
	Q->count = 0;
	Q->open = 1;
	Q->activeThreads = activeTs;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	return 0;
}
void printQueue(queue_t* Q){
	queueLL* temp = Q->head;
	while(temp != NULL){		
		printf("|%s|->",temp->data);		
		temp = temp->next;
	}
	printf("\n");
}

void enqueue(char* path, queue_t* Q){
	//queue_t* Q = queue;
	//pthread_mutex_lock(&Q->lock);
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
	pthread_cond_signal(&Q->read_ready);
	//pthread_mutex_unlock(&Q->lock);
}

char* dequeue(queue_t* Q){
	
	pthread_mutex_lock(&Q->lock);	
	if(Q->count == 0){
		//printf("Active: %d\n",Q->activeThreads);
		Q->activeThreads--;
		if(Q->activeThreads == 0){
			pthread_mutex_unlock(&Q->lock);
			pthread_cond_broadcast(&Q->read_ready);
			return NULL;
		}
		while(Q->count == 0 && Q->open == 1 && Q->activeThreads > 0){
			pthread_cond_wait(&Q->read_ready, &Q->lock);
		}
		if(Q->count == 0){
			pthread_mutex_unlock(&Q->lock);
			return NULL;
		}
		Q->activeThreads++;
	}
	queueLL* temp = Q->head;
	char* path = temp->data;		
	Q->head = temp->next;
	Q->count--;
	free(temp);

	
	pthread_mutex_unlock(&Q->lock);
	return path;

}
int destroy(queue_t *Q)
{
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);	
	return 0;
}
int qclose(queue_t *Q)
{
	pthread_mutex_lock(&Q->lock);
	Q->open = 0;
	pthread_cond_broadcast(&Q->read_ready);	
	pthread_mutex_unlock(&Q->lock);
	return 0;
}