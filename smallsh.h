#ifndef SMALLSH_H
#define SMALLSH_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>

//********GLOBALS*********//
#define MAX_INPUT_LENGTH 2048
#define MAX_ARGUMENT 512
#define MAX_ARGUMENT_CHAR 256
#define NUM_BUILT_IN_COMMANDS 3
int numOfArgs;

//******** FLAGS *********//
int backgroundModeFlag = 1;
int status = 0;
int exitFlag = 0;

//******** COMMANDS *********//
const char* BUILT_IN_COMMANDS[] = { "cd", "exit", "status" };
const char BACKGROUND_CMD = '&';
const char INPUT_REDIRECT = '<';
const char OUTPUT_REDIRECT = '>';

//******** FUNCTION PROTOTYPES *********//

//Initialize 
void initBuffers(char* inputBuffer, char* inputArgs[], char inRe[], char outRe[]);
void resetArgs(char* inputArgs[], int* redirectionNeeded);

//IO
void getInput(char* inputBuffer);
int tokenizeInput(char* inputBuffer, char* inputArgs[], char inputRedirect[], char outputRedirect[], pid_t parentPID);
void setRedirection(char input[], char output[]);
void toggleBackgroundMode();

//Execute
int isBuiltInFunction(char* arg);
void runBuiltInFunction(char* args[], int arg);
void executeArgs(char* args[], char input[], char output[], int redirectionNeeded, struct sigaction signal);


#endif
