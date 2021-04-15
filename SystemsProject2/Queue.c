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
	pthread_mutex_lock(&Q->lock);
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
	pthread_mutex_unlock(&Q->lock);
}
char* Fdequeue(queue_t* Q){
	
	pthread_mutex_lock(&Q->lock);	
	
	if(Q->count == 0){
		return NULL;
	}
	queueLL* temp = Q->head;
	char* path = temp->data;		
	Q->head = temp->next;
	Q->count--;
	free(temp);

	
	pthread_mutex_unlock(&Q->lock);
	return path;

}
char* dequeue(queue_t* Q){
	
	pthread_mutex_lock(&Q->lock);	
	if(Q->count == 0){
		printf("Active: %d\n",Q->activeThreads);
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