#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#define MAXWORDS 100
#include "getword.h"
#include <math.h>
typedef enum {
    false, true
} bool;
char *newargv[MAXWORDS];  //argument array
char s[MAXWORDS*STORAGE];  //storage array
char * input_file;
char * output_file;
int flag_in = 0;  //0 indicating there's no input flag
int flag_out = 0;   //0 indicating there's no output flag
int flag_pipeline = 0;   //0 indicating there's no pipeline flag
int flag_ampersand = 0;   //0 indicating there's no ampersand flag
int pipeIndex = 0;
int main(int argc, char * argv[])
{
    int num_arg;  //number of arguments returned by parse()
    int input_fd; //inpu file descriptor
    int output_fd;  //output file descriptor
    pid_t childpid;
    while(true) {
        printf(":570: ");  //giving the prompt
        num_arg = parse();
        if (num_arg == 255) {  //returning 255 if EOF
            break;
        }
        if (num_arg == -2) {
            fprintf(stderr, "Redirection error.\n");
            fflush(stderr);
            continue;
        }
    }

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
        else if (reValue == -254) {  //getting return value -254 if there's '&' at the end
            flag_ampersand = 1;
            continue;
        }
        else if(reValue == 0) {  //finish collecting words when see '$' '\n' ';'
            newargv[argc] = NULL;
            return argc;
        }
        else if(reValue == 1) {  //handle metacharacter '|' '>' '<'
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
                    argc++;
                    continue;
                }
            }
            if (s[length] == '>') {
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
                    argc++;
                    continue;
                }
            }
            else {  //otherwise treat this one length character as an argument
                newargv[argc++] = (s + length);
                length = length + abs(reValue) + 1;
                continue;
            }
        }
        else {  // encountering normal words
            newargv[argc++] = (s + length);
            length = length + abs(reValue) + 1;  //pay attention here, the return value might be negative because of '$'
        }
    }

}
