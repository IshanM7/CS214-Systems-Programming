#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "strbuf.h"
#define BUFFSIZE 1000

//initialize constant arrays that we will need for storing and writing
const char write_space[1] = {' '};
const char write_line[1] = {'\n'};
const char write_para[2] = {'\n','\n'};
char buff[BUFFSIZE];


void dump(strbuf_t *list){
    int i;
    printf("[%2lu/%2lu] ",list->used, list->length);
    for(i = 0; i < list->used; i++){
        printf("%02x ",(unsigned char) sb_get(list,i));
    }
    for(; i < list->length; i++){
        printf("__ ");
    }
    printf("\n");
}
int isEscape(char ch){
    if(ch == '\t' || ch == '\n' || ch == '\v' || ch == '\f' || ch == '\r')
        return 1;
    else
        return 0;


}
int wrap(int out_fd, int in_fd, int width){
    int bytes_read,i,retval,printline_len,newline_count,consecutive_whitespace,prevline_is_blank,first_word;
    strbuf_t word;
    sb_init(&word,1);

    retval = 0;
    printline_len = 0;
    newline_count = 0;
    consecutive_whitespace = 0;
    prevline_is_blank = 0;
    first_word = 0;
    bytes_read = read(in_fd, buff, BUFFSIZE);
    while(bytes_read > 0){
        for(i = 0; i < bytes_read; i++){
            if(!isspace(buff[i])){//Used to account for if there are whitespaces before the first word
                first_word = 1;
            }
            if(isspace(buff[i]) && first_word == 1){
                    if(word.used > width){// word is longer than width retval = 1 to return error
                        retval = 1;
                        write(out_fd,write_line,1);
                        write(out_fd,word.data,word.used-1);
                        write(out_fd,write_line,1); 
                        sb_destroy(&word);
                        sb_init(&word,1);
                        printline_len = 0;
                    }
                    else{                              
                        if(consecutive_whitespace == 0){//Avoids issues with multiple white spaces 
                            if(printline_len > width){
                                write(out_fd,write_line,1);
                                printline_len = word.used-1;
                            }
                            write(out_fd,word.data,word.used-1);//print word using word.used-1 because null terminator is included in word.used
                            //dump(&word);
                            sb_destroy(&word);
                            sb_init(&word,1);
                            
                            
                            if(printline_len+1 <= width && i > 0){//add a space if there is space in the width
                                write(out_fd,write_space,1);
                                printline_len++;
                            }                                                
                        }
                    }
                
                consecutive_whitespace++;
                if(buff[i] == '\n'){//count new lines to see if we must add a new paragraph
                    newline_count++;
                    if(newline_count >= 2 && prevline_is_blank == 0){
                        write(out_fd,write_para,2);
                        printline_len = word.used-1;
                        newline_count = 0;
                        prevline_is_blank++;
                    }
                }
            }
            else{//character not whitespace add to word
                if(!isspace(buff[i])){
                    sb_append(&word,buff[i]);
                    printline_len++;
                    newline_count = 0;
                    consecutive_whitespace = 0;
                    prevline_is_blank = 0;
                }

            }   
        }
        bytes_read = read(in_fd, buff, BUFFSIZE);

    }
    //print last word read
    if(word.used-1 > 0){
        if(word.used-1 > width){//last word is larger than line width
            retval = 1;
            write(out_fd, write_line, 1);
            write(out_fd, word.data, word.used-1);
        }
        else if (printline_len <= width){//last word fits in line
            write(out_fd, word.data, word.used-1);
        }else{//last word doesnt fit line write on new line
            write(out_fd, write_line, 1);
            write(out_fd, word.data, word.used-1);
        }
    
        
    }
    sb_destroy(&word);
    write(out_fd, write_line, 1);
         
    return retval;
}
int main(int argc, char** argv){
    

    if(argc == 1 || argc > 3){
        perror("Wrong number of arguments\n");
        return EXIT_FAILURE;
    }
    int width = atoi(argv[1]);
    if(argc == 2){//Read from stdin
        int error = wrap(1,0,width);
        if(error == 1){
            perror("There was a word longer than the given width\n");
            return EXIT_FAILURE;
        }
    }
    else{//We have a file or a directory
        char* path = argv[2];
        struct stat argcheck;
        stat(path,&argcheck);
        if(S_ISREG(argcheck.st_mode)){//argument is a file 
            int fd = open(path, O_RDONLY);
            if(fd == -1){
                perror("There was an error with the file\n");
                return EXIT_FAILURE;
            }

            int error = wrap(1,fd,width);
            if(error == 1){
                perror("There was a word longer than the given width\n");
                return EXIT_FAILURE;
            }
        }else if(S_ISDIR(argcheck.st_mode)){//argument is a directory
            DIR* dirp = opendir(path);
            struct dirent* de;

            char* curr_filename;
            int final_error;
            while((de = readdir(dirp))){
                curr_filename = de->d_name;
                if(de->d_type == DT_REG){
                    if(curr_filename[0] != '.' && (strncmp("wrap.", curr_filename, 5) != 0)){
                        //Create a path to the file being read at the moment
                        int currpath_len = strlen(de->d_name)+strlen(path)+2;
                        char* currpath = malloc(sizeof(char) * currpath_len);
                        strcpy(currpath,path);
                        strcat(currpath,"/");
                        strcat(currpath,de->d_name);

                        //create name for new wrap file
                        int wrap_len = strlen(de->d_name)+strlen(path)+7;
                        char* wrapname = malloc(sizeof(char) * wrap_len);
                        strcpy(wrapname,path);
                        strcat(wrapname,"/wrap.");
                        strcat(wrapname,de->d_name);
                        
                        //open the two files and send to wrap to create
                        int outfd = open(wrapname, O_RDWR|O_TRUNC|O_CREAT, 0666);
                        if(outfd == -1){
                            perror("There was an error with the file\n");
                            return EXIT_FAILURE;
                        }
                        int fd = open(currpath,O_RDONLY);
                        if(fd == -1){
                            perror("There was an error with the file\n");
                            return EXIT_FAILURE;
                        }
                        int error = wrap(outfd,fd,width);
                        //if there's a file with error continue wrapping files in directory but save that there was an error
                        if(error == 1) 
                            final_error = 1;                                                
                        
                        free(currpath);
                        free(wrapname);
                    }
                
                }
            }
            closedir(dirp);
            if(final_error == 1){
                perror("There was a word longer than the given width\n");
                return EXIT_FAILURE;
            }
        }
    }
    
    

}