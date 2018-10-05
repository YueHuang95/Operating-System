/*Yue Huang, Dr Carroll CS570 due: 09/17/2018
 getword.c is the c file for getword() function
 getword() function get every word from the commmand input and put them into storage array one by one with a
 at the end of each word. getword() return length of a string to the caller function.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p2.h"
typedef enum{
    false, true
} bool;
extern int flag_ampersand;
/*handling '\','~',' ','\t','$',';','\n','|','>','<','<<'
 * returning negative word count if $
 * returning path if starting with ~
 * separate word and return 0 if there's ';' '\n' '$'
 * returning word count if ' ' '\t'
 * Separate word and return 1 if '|' '>' '<' '&'
 * separate word and return 2 if '<<'
 * separate word not returning if ' ' '\t'
 * not separate word if '\'
 * return -255 if EOF
 */
int getword(char *w)
{
    int iochar;
    bool backslash = false;
    bool negative = false;  //flag, used to determine whether a string starts with $
    int wordCount = 0;
    while ((iochar = getchar()) != EOF) {
        //case 1: maximum string length
        if (wordCount == 254) {
            ungetc(iochar, stdin);  //put back the 255th element
            * (w + wordCount) = '\0';
            return wordCount;
        }
        //case 2: preceding backslash
        else if (backslash == true) {
            backslash = false;
            if (iochar == '\n') {  //if a backslash preceding a newline, treating it as a space
                return printOutput(w, wordCount, negative);
            }
            * (w + wordCount) = iochar;
            wordCount++;
        }
        //case 3: all other cases
        else {
            //case 3.1: starting tilde
            if (iochar == '~' && wordCount == 0 && negative == false) {
                char * ptr = getenv("HOME");
                while(wordCount < strlen(ptr)) {
                    *(w + wordCount) = * (ptr + wordCount);
                    wordCount++;
                }
                continue;
            }
            //case 3.2
            if (iochar == ' ' || iochar == '\t') {
                if (wordCount != 0 || negative == true) {  //if it's not leading zero or trailling zeros, print out
                    return printOutput(w, wordCount, negative);
                }
                //if it's leading zeros or trailing zeros, reset flag and continue
                
                continue;
            }
            if (iochar == '$' && wordCount == 0 && negative == false) {
                negative = true;
                continue;
            }
            //case 3.3
            if (iochar == ';' || iochar == '\n' ) {
                if (wordCount != 0 || negative == true) {  //if string is not null or starting with '$'
                    ungetc(';',stdin);  //if a semicolon or a newline or a '|' follow directly by a string, print out the string
                    return printOutput(w, wordCount, negative);  //and put semicolon back because we also need to print out an empty string
                }
                *w = '\0';  //getting back and print out empty string
                return 0;
            }
            //case 3.4
            if (iochar == '|') {
                if (wordCount != 0 || negative == true) {
                    ungetc('|',stdin);  //get back '|' in order to print out '|'
                    return printOutput(w, wordCount, negative);  //print out preceding string
                }
                * w = '|';
                * (w + 1) = '\0';
                return 1;
            }
            //case 3.5
            if (iochar == '>') {
                if (wordCount != 0 || negative == true) {
                    ungetc('>',stdin);  //get back '|' in order to print out '|'
                    return printOutput(w, wordCount, negative);  //print out preceding string
                }
                * w = '>';
                * (w + 1) = '\0';
                return 1;
            }
            //case 3.6
            if (iochar == '<') {
                iochar = getchar();  //get the next character first
                if (iochar == '<') {  //if it's '<<'
                    if (wordCount != 0 || negative == true) {  //print out preceding string
                        ungetc('<',stdin);  //put back two '<' in order to print out n=2, s=[<<]
                        ungetc('<',stdin);
                        return printOutput(w, wordCount, negative);
                    }
                    * w = '<';
                    * (w + 1) = '<';
                    * (w + 2) = '\0';
                    return 2;
                }
                else {  //if it's only one '<'
                    ungetc(iochar, stdin);  //get back next character to stdin stream whatever it's
                    if (wordCount != 0 || negative == true) {  //print out preceding string
                        ungetc('<',stdin);  //put back one '<' in order to print out n=1, s=[<]
                        return printOutput(w, wordCount, negative);
                    }
                    * w = '<';
                    * (w + 1) = '\0';
                    return 1;
                }
            }  //end '<' if
            //case 3.7: if incoming char is '\'
            if (iochar == 92) {
                backslash = true;
                continue;
            }
            //Case 3.8(adding for program2): if incoming char is '&'
            //If its not at the end of the input, treat it as word separator, otherwise set a global flag
            //Notice here, \& and & should not be treated the same way. & implies that parent process do not wait for
            //child process while \& still be treated as a word separator
            if (iochar == '&') {
                if (wordCount != 0) {  //print out preceding words first
                    return printOutput(w, wordCount, negative);
                } else {
                    iochar = getchar();
                    if (iochar == '\n') {  //if & followed by a newline, return 1 with s="&"
                        ungetc('\n',stdin);
                        * w = '&';
                        * (w + 1) = '\0';
                        flag_ampersand = 1;
                        return 1;
                    }
                    else {
                        ungetc(iochar, stdin);
                        * w = '&';
                        * (w + 1) = '\0';
                        return 1;
                    }
                }
            }
            //staring normal parse
            * (w + wordCount) = iochar;
            wordCount++;
        }  //end else
    }  //end of while loop
    if (wordCount != 0 || negative == true) {  //if EOF follow directly by a string or by a '$', print out the string
        return printOutput(w, wordCount, negative);
    }
    * w = '\0';
    return -255;
}
int printOutput(char *w, int wordCount, bool negative) {
    * (w + wordCount) = '\0';  //put a null terminator at the end of each string in order for caller function to print it out
    if (negative == true) {  //if a string starts with $, then return negative word count;
        if (wordCount == 0) {
            return 0;
        }
        return -wordCount;
    } else {
        return wordCount;
    }
}
