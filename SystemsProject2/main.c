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

#include "LL.h"
#include "Queue.h"
#define BUFFSIZE 100
#define QSIZE 8



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
queue_t* fileQueue;
queue_t* directoryQueue;
char *fileSuffix = ".txt";
Node** WFDList;


Node* calcFreq(Node* head, int wc){
	Node* temp = head;
	while(temp != NULL){
		double freq = ((1.0)*temp->count)/wc;
		temp->frequency = freq;
		printf("Head: %f\n",temp->frequency);
		temp = temp->next;
	}
	return head;
}

Node* readWords(char* path){
	Node* head = NULL;
	int fd = open(path,O_RDONLY);
	int wc = 0;
	int bytes_read;
	strbuf_t word;
    sb_init(&word,1);
	char buff[1];
	while((bytes_read = read(fd,buff,1))>0){
		char c = buff[0];
		if(!ispunct(c) && !isspace(c)|| c == '-'){
			c = tolower(c);
			sb_append(&word,c);
		}else{
			if(isspace(c)){
				if(word.used-1 > 0){
					wc++;
					//printf("word: %s\n",word.data);
					head = insert(head,&word);						
					sb_destroy(&word);
					sb_init(&word,1);				
				}else{
					sb_destroy(&word);
					sb_init(&word,1);								
				}
			}
		}
		
		//printf("bytes: %d\n",bytes_read);
	}

	if(word.used -1 > 0){
		wc++;
		//printf("word: %s\n",word.data);
		head=insert(head,&word);					
		sb_destroy(&word);
		
	}else{
		sb_destroy(&word);
	}
	//printf("WC: %d\n",wc);
	head = calcFreq(head, wc);
	return head;
}

typedef struct {
	Node* WFD;
}targs;
void* fileWork(void *A){
	targs* args = A;
	Node* wfd = NULL;
	while(1){
		printf("fileWork: ");
		printQueue(fileQueue);
		printf("\n");
		char* currName = Fdequeue(fileQueue);
		Node* wfd = readWords(currName);
		if(currName == NULL){
			break;
		}
	}
	return wfd;
}
void* dirChecks(void *A){
	//struct targs* args = A;
	DIR* dirp;
	struct dirent* de;

	while(1){
		printf("dirWork: ");
		printQueue(directoryQueue);
		printf("\n");
		char* currName;
		currName = dequeue(directoryQueue);
		if(currName == NULL){
			break;
		}
		//printf("Thread [%d] takes %s\n",args->id,currName);
		strbuf_t* currPath = malloc(sizeof(strbuf_t));
		sb_init(currPath,50);
		sb_concat(currPath,currName);
		//dump(currPath);
		sb_concat(currPath,"/");
		//dump(currPath);
		dirp = opendir(currPath->data);
		while((de = readdir(dirp))){									
			if(de->d_type == DT_REG){

				strbuf_t* newPath = malloc(sizeof(strbuf_t));
				sb_init(newPath,currPath->used);
				char* currFile = de->d_name;
				sb_concat(newPath,currPath->data);
				sb_concat(newPath,currFile);
				
				//dump(newPath);
				
				int len = newPath->used;
				
				int slen = strlen(fileSuffix);
				char* temp = newPath->data;				
				//printf("temp: %s\n",temp);
				if(strcmp(temp+len-slen-1,fileSuffix)==0){					
					enqueue(temp,fileQueue);
				}			
				//sb_destroy(newPath);
				free(newPath);
			}else{
				
				char* currFile= de->d_name;
				char c = currFile[0];
				if(c != '.'){
					strbuf_t* newDPath = malloc(sizeof(strbuf_t));
					sb_init(newDPath,currPath->used);
					sb_concat(newDPath,currPath->data);
					sb_concat(newDPath,currFile);
					char* temp = newDPath->data;
					enqueue(temp,directoryQueue);
					//dump(newDPath);
					//sb_destroy(newDPath);
					free(newDPath);
				}

			}
		}
		sb_destroy(currPath);
		free(currPath);	
				
			
		closedir(dirp);
	}
		
	return 0;
}
//NEED TO KEEP TRACK OF HOW MANY THREADS ARE ACTIVELY PUTTING INTO DIRECTORY QUEUE FOR DIRECTORY CLOSE FILE CLOSE IS THE SAME
int main(int argc, char **argv)
{
	int directoryThreads = 1;
	int fileThreads = 1;
	int analysisThreads = 1;
	
	int i,fileargs,retval;
	retval = 0;
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

	//printf("dThreads: %d\nfThreads: %d\naThreads: %d\nSuffix: %s\n",directoryThreads,fileThreads,analysisThreads,fileSuffix);
	
	
	queueLL* headFile = NULL;
	queueLL* lastFile = NULL;
	queueLL* headDirectory = NULL;
	queueLL* lastDirectory = NULL;
	
	
	fileQueue = malloc(sizeof(queue_t));
	directoryQueue = malloc(sizeof(queue_t));
	init(fileQueue,headFile,lastFile,fileThreads);
	init(directoryQueue,headDirectory,lastDirectory,directoryThreads);
	int fc =0;
	int dc = 0;
	
	//struct targs* args;
	//args = malloc(directoryThreads*sizeof(args));

	//int totalWords = 0;
	Node* front = readWords(argv[1]);
	printf("List: \n");
	printList(front);
	
	freeList(front);
	
	
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
		//printf("%s\n",argv[fileargs]);
		//calcWFD(currfile);
	}
	pthread_t dThreads[directoryThreads];
	pthread_t fThreads[fileThreads];
	/*for(int i = 0; i < directoryThreads; i++){
		//args->id = i;
		pthread_create(&dThreads[i],NULL,dirChecks,NULL);
	}
	void* dirRet;
	for(int i = 0; i < directoryThreads; i++){
		pthread_join(dThreads[i],&dirRet);
	}*/

	/*WFDList = malloc(sizeof(Node*)*fileQueue->count);
	targs* args = malloc(sizeof(targs)*fileThreads);
	printQueue(fileQueue);	
	for(int i = 0; i < fileThreads; i++){
		args->WFD = WFDList[i];
		pthread_create(&fThreads[i],NULL,fileWork,NULL);
	}
	
	
	
	for(int i = 0; i < fileThreads; i++){
		pthread_join(fThreads[i],WFDList[i]);
	}*/
	printf("file: \n");
	printQueue(fileQueue);	
	

	
	
	destroy(fileQueue);
	destroy(directoryQueue);
	
	free(fileQueue);
	free(directoryQueue);
	if(retval == 1){
		return EXIT_FAILURE;
	}else
		return EXIT_SUCCESS;
}