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
#include <math.h>
#include "LL.h"
#include "Queue.h"
#define BUFFSIZE 100
#define QSIZE 8


typedef struct{
	LL* head;
	char* name;
	int totalWords;
	struct WFD* next;
}WFD;

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
fileQueue_t* fileQueue;
queue_t* directoryQueue;
char *fileSuffix = ".txt";

void printWFDRepo(WFD* fileNodeHead) {
    WFD* ptr = fileNodeHead;
    while (ptr != NULL) {
        printf("%s: %d\t", ptr->name, ptr->totalWords);
		printList(ptr->head);
        ptr = ptr->next;
    }
    printf("\n");
}
void freeWFDRepo(WFD* fileNodeHead) {
    WFD* tempNode = fileNodeHead;
    while (fileNodeHead != NULL) {
        tempNode = fileNodeHead;
        fileNodeHead = fileNodeHead->next;
        free(tempNode->name);
        freeList(tempNode->head);
        free(tempNode);
    }
}
int WFDLength(WFD* wfd){
	int ret = 0;
	WFD* ptr = wfd;
	while(ptr != NULL){
		ret++;
		ptr = ptr->next;
	}
	return ret;
}
int insertWFDEntry(WFD** wfd,LL* head,char* path,int wc){
	if (*wfd == NULL) {
		WFD* new = malloc(sizeof(WFD));
		new->head = head;
		new->name = malloc(strlen(path)+1);
		strcpy(new->name, path);
		new->next = NULL;
		new->totalWords = wc;
        *wfd = new;
        return 0;
    }
	WFD* ptr = *wfd;
	WFD* prev = NULL;
	while(ptr != NULL){
		if(strcmp(path, ptr->name) < 0){
			WFD* new = malloc(sizeof(WFD));
			new->head = head;
			new->name = malloc(strlen(path)+1);
			strcpy(new->name, path);
			new->next = NULL;
			new->totalWords = wc;
			if(prev == NULL){
				new->next = ptr;
				*wfd = new;
				return 0;
			}else{
				prev->next = new;
				new->next = ptr;
				return 0;
			}
		}else{
			prev = ptr;
			if(ptr->next == NULL){
				WFD* new = malloc(sizeof(WFD));
				new->head = head;
				new->name = malloc(strlen(path)+1);
				strcpy(new->name, path);
				new->next = NULL;
				new->totalWords = wc;
				ptr->next = new;
				return 0;
			}
			ptr = ptr->next;
		}
		
	}
   
    return 0;
	
}


LL* calcFreq(LL* head, int wc){
	LL* temp = head;
	while(temp != NULL){
		double freq = ((1.0)*temp->count)/wc;
		temp->frequency = freq;		
		temp = temp->next;
	}
	return head;
}

LL* readWords(char* path, WFD** wfd){
	LL* head = NULL;
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
	insertWFDEntry(wfd,head,path,wc);
	return head;
}

typedef struct {
	int id;
	WFD** wfdRepo;
	fileQueue_t* fileQueue;
}fargs;
typedef struct {
	int id;
	fileQueue_t* fileQueue;
	queue_t* directoryQueue;
}dargs;

void* fileWork(void *A){
	fargs* args = (fargs*)A;
	int id = args->id;
	fileQueue_t* fileQueue = args->fileQueue;
	WFD** wfd = args->wfdRepo;
	sleep(.3);
	while(fileQueue->count > 0){
		
		char* currName;
	
		fileDequeue(fileQueue,&currName);
		printf("Thread[%d] takes: %s\n",id,currName);
		//printf("CURRENT: %s\n",currName);
		readWords(currName,wfd);
	
		free(currName);
	}	
	return;
}




void* dirChecks(void *A){
	//struct targs* args = A;
	DIR* dirp;
	struct dirent* de;
	dargs* args = (dargs*)A;
	int id = args->id;
	fileQueue_t* fileQueue = args->fileQueue;
	while(1){		
		char* currName;
		currName = dequeue(directoryQueue);
		if(currName == NULL){
			break;
		}
		//printf("Thread [%d] takes %s\n",args->id,currName);
		printf("DThread[%d] takes: %s\n",id,currName);

				
		dirp = opendir(currName);
		while((de = readdir(dirp))){									
			if(de->d_type == DT_REG){
				char* currFile = malloc(strlen(currName)+strlen(de->d_name)+2);
				strcpy(currFile,currName);
				strcat(currFile,"/");
				strcat(currFile,de->d_name);
												
				int len = strlen(currFile);				
				int slen = strlen(fileSuffix);
				char* temp = currFile;				
				//printf("temp: %s\n",temp);
				if(strcmp(temp+len-slen,fileSuffix)==0){					
					fileEnqueue(fileQueue,temp);
				}			
				
				free(currFile);
			}else{				
				char* curr = de->d_name;
				char c = curr[0];
				if(c != '.'){
					char* currFile = malloc(strlen(currName)+strlen(de->d_name)+2);
					strcpy(currFile,currName);
					strcat(currFile,"/");
					strcat(currFile,de->d_name);
					char* temp = currFile;					
					enqueue(temp,directoryQueue);
					//free(temp);
					
				}

			}
		}											
		closedir(dirp);
	}
		
	return 0;
}
typedef struct {
	LL* file1head;
	LL* file2head;
	char* file1;
	char* file2;
	int combinedCount;
	double JSDbetween;
}pair;
typedef struct {
	int id;
	pair** pairList;
	int start;
	int end;
}anargs;

double meanFrequency(LL* file1, LL* file2, char* word) {
    double freq1 = 0;
	double freq2 = 0;
	double retVal;
	LL* ptr = file1;
	while(ptr != NULL){
		if(strcmp(ptr->word,word)==0){
			freq1 = ptr->frequency;
		}
		ptr = ptr->next;
	}
	LL* ptr2 = file2;
	while(ptr2 != NULL){
		if(strcmp(ptr2->word,word)==0){
			freq2 = ptr2->frequency;
		}
		ptr2 = ptr2->next;
	}
	retVal = .5*(freq1+freq2);
	return retVal;
}

double jsdBetweenFiles(LL* head1, LL* head2){
	double kld1 = 0;
	double kld2 = 0;
	LL* ptr1 = head1;
	LL* ptr2 = head2;
	while (ptr1 != NULL) {
        double meanFreq = meanFrequency(head1, head2, ptr1->word);
        kld1 += (ptr1->frequency * log2(ptr1->frequency / meanFreq));
        ptr1 = ptr1->next;
    }
	while (ptr2 != NULL) {
        double meanFreq = meanFrequency(head2, head1, ptr2->word);
        kld2 += (ptr2->frequency * log2(ptr2->frequency / meanFreq));
        ptr2 = ptr2->next;
    }

	double retval = sqrt(.5*kld1+.5*kld2);
	return retval;
}
void* createJSD(void* A){
	anargs* args = A;
	pair** pairList = args->pairList;
	int start = args->start;
	int end = args->end;
	int id = args->id;
	//printf("[%d] start: %d end: %d\n",id,start,end+start);
	for(int i = start; i < start+end;i++){
		pair* currPair = pairList[i];
	
	
		currPair->JSDbetween = jsdBetweenFiles(currPair->file1head, currPair->file2head);

	}
}
int compareFunc(const void* pair1,const void* pair2){
	pair* x = (pair*)pair1;
	pair* y = (pair*)pair2;
	
	printf("p1: %d p2: %d\n",(*x).combinedCount,(*y).combinedCount);
	

	return -1;
}
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

	printf("dThreads: %d\nfThreads: %d\naThreads: %d\nSuffix: %s\n",directoryThreads,fileThreads,analysisThreads,fileSuffix);

	queueLL* headDirectory = NULL;
	queueLL* lastDirectory = NULL;
	
	WFD* wfdRepo = NULL;
		
	fileQueue_t fQueue;
	fileinit(&fQueue);	
	directoryQueue = malloc(sizeof(queue_t));	
	init(directoryQueue,headDirectory,lastDirectory,directoryThreads);

	int fc =0;
	int dc = 0;		
	
	for(fileargs; fileargs < argc; fileargs++){
		char* currfile = argv[fileargs];
		struct stat argcheck;
        stat(currfile,&argcheck);
		if(S_ISREG(argcheck.st_mode)){//argument is a file	
			fc++;
			fileEnqueue(&fQueue,currfile);
			 					
        }else if(S_ISDIR(argcheck.st_mode)){//argument is a directory
			dc++;
			enqueue(currfile, directoryQueue);
		}else{
			retval = 1;
		}
	}
	//Create Directory Threads
	pthread_t dThreads[directoryThreads];
	dargs* darg = malloc(sizeof(dargs)*directoryThreads);
	for(int i = 0; i < directoryThreads; i++){
		darg[i].id = i;
		darg[i].fileQueue = &fQueue;
		darg[i].directoryQueue = directoryQueue;
		pthread_create(&dThreads[i],NULL,dirChecks,&darg[i]);
	}
	//Create File Threads
	pthread_t fThreads[fileThreads];
	fargs* farg = malloc(sizeof(fargs)*fileThreads);

	for(int i = 0; i < fileThreads; i++){
		farg[i].id = i;
		farg[i].fileQueue = &fQueue;
		farg[i].wfdRepo = &wfdRepo;
		pthread_create(&fThreads[i],NULL,fileWork,&farg[i]);
	}
	//printf("WAit\n");
	//Join Directory Threads
	void* dirRet;
	for(int i = 0; i < directoryThreads; i++){
		pthread_join(dThreads[i],&dirRet);		
	}
	qclose(directoryQueue);
	//Join File Threads
	printf("here\n");
	void* fileRet;
	for(int i = 0; i < fileThreads; i++){
		pthread_join(fThreads[i],&fileRet);		
	}
	fileClose(&fQueue);


	int numFiles = WFDLength(wfdRepo);
	printf("numFile: %d\n",numFiles);
	printWFDRepo(wfdRepo);

	int totalCombinations = numFiles*(numFiles-1)/2;
	printf("totalC: %d\n",totalCombinations);
	
	pair** filePairs = malloc(totalCombinations*(sizeof(pair*)));
	WFD* ptr = wfdRepo;
	int c = 0;
	while(ptr != NULL){
		WFD* ptr2 = ptr->next;
		while(ptr2 != NULL){
			pair* currPair = malloc(sizeof(pair));
			currPair->file1 = ptr->name;
			currPair->file1head = ptr->head;
			currPair->file2 = ptr2->name;
			currPair->file2head = ptr2->head;
			currPair->combinedCount = ptr->totalWords + ptr2->totalWords;
			currPair->JSDbetween = 0;
			filePairs[c] = currPair;
			c++;
			ptr2 = ptr2->next;
		}
		ptr = ptr->next;
	}
	int rem = totalCombinations % analysisThreads;
	int split = totalCombinations/analysisThreads;
	int startIndex = 0;


	pthread_t aThreads[analysisThreads];
	anargs* anarg = malloc(sizeof(anargs)*analysisThreads);
	for(int i = 0; i < analysisThreads; i++){
		anarg[i].id = i;
		anarg[i].pairList = filePairs;
		anarg[i].start = startIndex;
		anarg[i].end = split;
		if(rem > 0){
			anarg[i].end++;
			rem--;
		}
		startIndex += anarg[i].end;
	}
	for(int i = 0; i < analysisThreads; i++){
		pthread_create(&aThreads[i],NULL,createJSD,&anarg[i]);
	}
	void* aRet;
	for(int i = 0; i < analysisThreads; i++){
		pthread_join(aThreads[i],&aRet);
	}
	printf("C: %d\n",c);
	
	
	qsort(filePairs,totalCombinations,sizeof(filePairs),compareFunc);
	for(int i = 0; i < totalCombinations; i++){
		printf("JSD: %f\t%s\t%s\t%d\n",filePairs[i]->JSDbetween,filePairs[i]->file1,filePairs[i]->file2,filePairs[i]->combinedCount);
		free(filePairs[i]);
	}
	
	fileDestroy(&fQueue);
	destroy(directoryQueue);	
	free(anarg);
	free(farg);
	free(darg);
	free(directoryQueue);
	freeWFDRepo(wfdRepo);
	if(retval == 1){
		return EXIT_FAILURE;
	}else
		return EXIT_SUCCESS;
	
}