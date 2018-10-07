/*Yue Huang, Dr Carroll CS570 due: 10/10/2018
 p2.c simulates how regular shell(command line intepreter) works. It consists with three large code blocks
 which are signal handler, parse functino- used to parse input commands into argument array and return
 argument numbers, main function-used to mimic how shell will do under certain circumstance such as input,
 output redirection, background process handling, directory redirection, read and write files using pipeline.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#define MAXWORDS 100
#include "getword.h"
#include <math.h>
#include "p2.h"
extern int flag_ampersand = 0;
typedef enum {
    false, true
} bool;
char *newargv[MAXWORDS];  //argument array
char s[MAXWORDS*STORAGE];  //storage array
char * input_file;
char * output_file;
int flag_in;  //0 indicating there's no input flag
int flag_out;  //0 indicating there's no output flag
int flag_pipeline;   //0 indicating there's no pipeline flag
int pipeIndex;
void myhandler(int signum);
int parse();
int main(int argc, char * argv[])
{
    int num_arg;  //number of arguments returned by parse()
    int input_fd; //inpu file descriptor
    int output_fd;  //output file descriptor
    pid_t childpid;
    int cd_correct;
    (void) signal(SIGTERM,myhandler);
	setpgid(0,0); /*set grounp id to separate p2 from running shell*/
    while(true) {
        flag_in = 0;
        flag_out = 0;
        flag_pipeline = 0;
        pipeIndex = 0;
        fflush(stdin);
        flag_ampersand = 0;
        printf(":570: ");  //giving the prompt
        num_arg = parse();
        if (num_arg == 255) {  //returning 255 if EOF
            break;
        }
        if (num_arg == 0) {  //no command line argument, give prompt again
            continue;
        }
        if (num_arg == -2) {
            fprintf(stderr, "Ambiguous output redirect.\n");
            fflush(stderr);
            continue;
        }
        if (num_arg == -3) {  //returning -3 meaning there are more than 1 pipe
            fprintf(stderr, "Multiple pipeline not allowed.\n");
            fflush(stderr);
            continue;
        }
        if(strcmp(newargv[0],"cd") == 0){ /*handle buildin command cd*/

			if(num_arg > 2){ /*handle cd argument more than 1 error*/
				fprintf(stderr,"chdir: Too many arguments.\n");
				fflush(stderr);
				continue;
			}

			if(newargv[1] == NULL){ /*with no parameters cd $HOME*/
				cd_correct = chdir(getenv("HOME"));

				if(cd_correct != 0){ /*fail to go home chdir failed*/
					perror("chdir: cannot go back to HOME ditrctory.");
					continue;
				}

				continue;
			}

			else{ /*cd to given path*/
				cd_correct = chdir(newargv[1]);

				if(cd_correct != 0){ /*fail to go to given path*/
					perror("path name error.");
					continue;
				}
				continue;
			}

		}
		if (flag_pipeline != 0) {  //handling pipeline condition
            fflush(stdout);
            fflush(stderr);
            childpid = fork();
            if(childpid < 0){ //if fork fail, print out error message and continue
					perror("fork failed.");
					continue;
            }
            if (childpid == 0) {  //code block that child process will execute
                pid_t grandChild_pid;
                int mypipefd[flag_pipeline * 2];
                int reValue;
                reValue = pipe(mypipefd);
                if (reValue == -1){
                    perror("Pipeline error");
                    exit(8);
                }
                grandChild_pid = fork();
                if(grandChild_pid < 0) {
                    perror("Grand Child fork failed.");
					continue;
                }
                if (grandChild_pid == 0) {

                }
                if(flag_ampersand == 1){ //this is a background job should redirect stdin to dev/null
                    int dev_null = open("/dev/null",O_WRONLY); //open dev/null file descriptor
                    dup2(dev_null, STDIN_FILENO);
                }
                if(flag_out == 1){ //if child process has output redirection, child process will not have input redirection because its guaranteed that it reads its input from mypipefd[0]
                    mode_t mode = S_IRWXU | S_IRWXG | S_IROTH;
                    output_fd = open(output_file, O_WRONLY | O_CREAT | O_EXCL, mode);
                    dup2(output_fd, STDOUT_FILENO); //redirect the stdout to the file
                    close(output_fd);  //close the output_fd file descriptor
                }
                dup2(mypipefd[0], STDIN_FILENO);
                close(mypipefd[0]);
                close(mypipefd[1]);
                if(execvp(newargv[0],newargv) < 0){
                    char str[80];
                    strcpy(str,"Execute failed!: ");
                    strcat(str,newargv[0]);
                    perror(str);  /*if execute failed print error and exit*/
                    _exit(9);
                }
            }  //end of child code block
            if(flag_ampersand == 1){ //if the child's job is a back ground job, then parent does not wait for it and reissue prompt again
				printf("%s [%d]",newargv[pipeIndex + 1],childpid); //print the command name and the child's pid
				continue;
			}

			while(true){ //parent wait for child finish its job
				pid_t pid;
				pid  = wait(NULL);

				if(pid == childpid){
				break;
				}
			}
            continue;
		}
		//handle normal situation
		fflush(stdout);
		fflush(stderr);
		childpid = fork();
        if(childpid < 0){ //fork failed
			fprintf(stderr,"fork failed.\n");
			fflush(stderr);
			continue;
		}
		if(childpid == 0){ //fork a child to do jobs

			if(flag_ampersand == 1){ //this is a background job should redirect stdin to dev/null
				int dev_null = open("/dev/null",O_WRONLY); //open dev/null file descriptor
				dup2(dev_null, STDIN_FILENO);
			}

			if(flag_in == 1){ //handle input redirection

				if((input_fd = open(input_file, O_RDONLY)) < 0){ //open the input file
					perror("Open failed!");
					_exit(1);
				}

				if(dup2(input_fd, STDIN_FILENO) < 0){ //redirect the stdin to the file
					perror("Redirection failed!");
					_exit(2);
				}

				if(close(input_fd) < 0){ //close the input_fd file descriptor
					perror("Close failed!");
					_exit(3);
				}
			}  //end handling input redirection

			if(flag_out == 1){ //handle output redirection
				mode_t mode = S_IRWXU | S_IRWXG | S_IROTH;
                //open the output file descriptor, if the file name already exist, then open failed with EEXIST, otherwise create the output file with given mode
				if((output_fd = open(output_file, O_WRONLY | O_CREAT | O_EXCL, mode)) < 0){
					printf("%s: File exists", output_file);
					_exit(4);
				}

				if(dup2(output_fd, STDOUT_FILENO) < 0){ /*redirect the stdout to the file*/
					perror("Redirection failed!");
					_exit(5);
				}

				if(close(output_fd) < 0){ /*close the output_fd file descriptor*/
					perror("Close failed!");
					_exit(6);
				}
			}  //end handling output redirection

			if(execvp(newargv[0],newargv) < 0){ /*child execute the command*/
				perror("Execute failed!");  /*if execute failed print error and exit*/
				_exit(7);
			}

			_exit(0); /*exit normally*/

		}  //end of child job

        if(flag_ampersand == 1){ //if the child's job is a back ground job, then parent does not wait for it and reissue prompt again
			printf("%s [%d]\n",newargv[0],childpid);
			continue;
		}

        while(true){ //parent wait for child finish its job
			pid_t pid;
			pid  = wait(NULL);

			if(pid == childpid){ /*wait till the child exit*/
				break;
			}
		}
    }
    killpg(getpgrp(),SIGTERM);
	printf("p2 terminated.\n");
	exit(0);
}
int parse() {
    int argc = 0;  //number of arguments
    int length = 0;  //used to record how many characters in the storage array, s[]
    int reValue;  //return value from getword()

    while(true) {
        reValue = getword(s + length);  //we're not overwrite s array, so we need length here
        if (reValue == -255) {  //returning 255 if EOF
            newargv[argc] = NULL;
            return abs(reValue);
        }
        else if(reValue == 0) {  //finish collecting words when see '$' '\n' ';'
            newargv[argc] = NULL;
            return argc;
        }
        else if(reValue == 1) {  //handle metacharacter '|' '>' '<' '&'
            if (s[length] == '<') {
                if (flag_in >= 1) {  //if there are more than one '<'
                    return -2;  //-2 indicating redirection error
                }
                else {
                    flag_in = 1;
                    length = length + 2;
                    reValue = getword(s + length);  //immediately checking the following word
                    //invalid file name ';' '\n' '$' '|' '<' '>' EOF, '<<' be treated as a word so it's valid in this case
                    if (reValue == 0 ||
                        s[length] == '|' ||
                        s[length] == '<' ||
                        s[length] == '>' ||
                        reValue == -255) {
                        return -2;  //-2 indicating redirection error
                    }
                    input_file = (s + length);  //pointing input ptr to the string on s string
                    length = length + abs(reValue) + 1;
                    continue;
                }
            }
            else if (s[length] == '>') {
                if (flag_out >= 1) {  //redirectiion error occurrs when there is more than one '>'
                    return -2;  //-2 indicating redirection error
                }
                else {
                    flag_out = 1;
                    length = length + 2;  // ">\0" there are two character so we need to add 2
                    reValue = getword(s + length);
                    if (reValue == 0 ||
                        reValue == -255 ||
                        s[length] == '|' ||
                        s[length] == '<' ||
                        s[length] == '>' ) {
                        return -2;  //-2 indicating redirection error
                    }
                    output_file = (s + length);
                    length = length + abs(reValue) + 1;
                    continue;
                }
            }
            else if (s[length] == '|') {
                if (flag_pipeline >= 1) {  //if there's alreayd one pipe
                    return -3;
                }
                flag_pipeline = 1;
                pipeIndex = argc;
                newargv[argc++] = NULL;
                length = length + 2;
                continue;
            }
            else if(s[length] == '&' && flag_ampersand == 1) {  //if we see & preceding a newline
                newargv[argc] = NULL;
                return argc;
            }
            else {  //otherwise treat this one length character as an argument
                newargv[argc++] = (s + length);
                length = length + abs(reValue) + 1;
                continue;
            }
        }  //end handling metacharacter '|' '>' '<' '&'
        else {  // encountering normal words
            newargv[argc++] = (s + length);
            length = length + abs(reValue) + 1;  //pay attention here, the return value might be negative because of '$'
        }
    }

}

void myhandler(int signum){ //only catch the signal but do nothing
}
