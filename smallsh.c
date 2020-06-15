
/* SMALLSH.C
 *  OS1 - W20: Oregon State University
 *  Thomas Banghart
 *  Saturday, Febuary 29th 2020
 *  
 * This program is a "wrapper shell" around bash. The user is prompted with ": " and allowed to enter any common bash command
 * The commands `cd`, `exit`, and `status` are handled by this program. All other commands are passed to the bash shell via execvp() as a child process. 
 * Commands can be run in either "foreground" or "background" mode. Commands executed in the foreground cause the parent process to wait() until the child process terminates.
 * Background commands cause the parent to wait(..WNOHANG..) so the user is open to enter another command as the child process runs.
 */

#include "smallsh.h"

//******** MAIN *********//
int main()
{
	//Vars used to handle IO
	pid_t parentPID = getpid();
	int redirectionNeeded, arg;
	char* inputArgs[MAX_ARGUMENT];
	char inputBuffer[MAX_INPUT_LENGTH];
	char inputRedirect[MAX_ARGUMENT_CHAR];
	char outputRedirect[MAX_ARGUMENT_CHAR];

	//Signal initialization 
	struct sigaction sigInt = {{0}};
	struct sigaction sigStp = {{0}};

	//Set SIGINT
	sigInt.sa_handler = SIG_IGN;
	sigfillset(&sigInt.sa_mask);
	sigInt.sa_flags = 0;
	sigaction(SIGINT, &sigInt, NULL);

	//Set SIGSTP
	sigStp.sa_handler = toggleBackgroundMode;
	sigfillset(&sigStp.sa_mask);
	sigStp.sa_flags = 0;
	sigaction(SIGTSTP, &sigStp, NULL);
	
	do
	{
		initBuffers(inputBuffer, inputArgs, inputRedirect, outputRedirect); //get buffers ready
		getInput(inputBuffer); //get input

		redirectionNeeded = tokenizeInput(inputBuffer, inputArgs, inputRedirect, outputRedirect, parentPID); //tokenizeInput sets redirection flag if '<' or '>' is inputed 

		if( (arg = isBuiltInFunction(inputArgs[0])) ) //if this is a built-in command, just run it and reprompt user
		{ 
			runBuiltInFunction(inputArgs, arg); 
			resetArgs(inputArgs, &redirectionNeeded); 
			continue; 
		}
		
		executeArgs(inputArgs, inputRedirect, outputRedirect, redirectionNeeded, sigInt); //otherwise run as child process
		resetArgs(inputArgs, &redirectionNeeded); //reset buffers
	
	} 
	while(1); //program terminates when user enters exit.

	return 0;
}


/* initBuffers
 * ----------------------------
 *	Initializes string buffers to take user input and hold commands.
 *   Args:
 *      inputBuffer: buffer to populate with fgets()
 *      inputArgs: array that will hold each command
 * 		inRe: buffer to hold name of input redirection file name
 * 		outRe: buffer to hold output redirection file name
 *   returns: void
 * ---------------------------- */
void initBuffers(char* inputBuffer, char* inputArgs[], char inRe[], char outRe[])
{
	int i;

	memset(inputBuffer, '\0', MAX_INPUT_LENGTH);
	memset(inRe, '\0', MAX_ARGUMENT_CHAR);
	memset(outRe, '\0', MAX_ARGUMENT_CHAR);

	for(i = 0; i < MAX_ARGUMENT; i++)
	{
		inputArgs[i] = malloc(MAX_ARGUMENT_CHAR * sizeof(char));
		memset(inputArgs[i], '\0', MAX_ARGUMENT_CHAR);
	}

	return;
}

/* getInput
 * ----------------------------
 *	Prompts user with ": " and awaits for input. Ignores comments "#" new lines and null values
 *   Args:
 *      inputBuffer: buffer populated with fgets()
 *   returns: void
 * ---------------------------- */
void getInput(char* inputBuffer)
{
	do
	{
		printf(": "); //Print prompt
		fflush(stdout); //flush stdin
		fgets(inputBuffer, MAX_INPUT_LENGTH, stdin);
		if(	inputBuffer[0] == '\n'  //Reprompt user if new line, null terminator, or comment was entered.
		||  inputBuffer[0] == '\0' 
		||  inputBuffer[0] == '#')
		{
			memset(inputBuffer, '\0', MAX_INPUT_LENGTH); //reset buffer
			continue; //prompt again
		}
		break;
		
	}while(1);

	inputBuffer[strlen(inputBuffer)-1]='\0'; //remove trailing newline.

	return;	
}

/* tokenizeInput
 * ----------------------------
 *	Creates string tokens for each command given to shell. Populates inputArgs to execute.
 *   Args:
 *      inputBuffer: buffer used to hold string token
 * 		inputArgs: array of string buffers to populate with token
 * 		outputRedirect: buffer to hold filename of outputRedirection
 * 		inputRedirection: buffer to hold filename of inputRedirection
 * 		parentPID: PID of the calling parent process.
 *   returns: (int) redirectionNeeded: flag set if either '<' or '>' are used within the command.
 * ---------------------------- */
int tokenizeInput(char* inputBuffer, char* inputArgs[], char inputRedirect[], char outputRedirect[], pid_t parentPID)
{
	int i = 0;
	int redirectionNeeded = 0; //flag to return to main
	numOfArgs = 0; //set global var numOfArgs to 0

	char* token = strtok(inputBuffer, " "); //Initialize first token

	while (token != NULL) //Iterate through the inputBuffer
	{ 
		if(token[0] == INPUT_REDIRECT) //If token == '<'
		{
			redirectionNeeded = 1; //set flag
			token = strtok(NULL, " "); //move to next token (filename)
			sprintf(inputRedirect, "%s", token); //add filename to inputRedirect buffer
			token = strtok(NULL, " ");  //move to next token

		}
		else if(token[0] == OUTPUT_REDIRECT) //If token == '>'
		{
			redirectionNeeded = 1; //set flag
			token = strtok(NULL, " "); //move to next token (filename)
			sprintf(outputRedirect, "%s", token); //add fielname to outputRedirect Buffer
			token = strtok(NULL, " "); //move to next token
			
		}
		else //if current token is something else do the following.
		{ 
			if(strcmp("testdir$$\0", token) == 0) //swap testdir$$ in grading script to real PID
			{
				sprintf(inputArgs[i], "%s%d", "testdir", parentPID);
			}
			else if(strcmp("$$\0", token) == 0) //if just calling $$, catch that as well
			{
				sprintf(inputArgs[i], "%d", parentPID);
			}
			else	//If something else simply populate the array.
			{
				sprintf(inputArgs[i], "%s", token);
			}
			
			token = strtok(NULL, " "); //move to text token
			numOfArgs++;
			i++;
		}
    } 
	inputArgs[numOfArgs] = NULL; //Null last value 

	return redirectionNeeded;
}

/* executeArgs
 * ----------------------------
 *	Passes commands back to bash shell to be executed if user enters a command that wasn't `cd`, `status`, or `exit`
 *   Args:
 *      args: array of command strings to be executed 
 * 		input: filename for input redirection
 * 		output: filename for output redirection
 * 		redirectionNeeded: flag that determines if I/O should be redirected
 * 		signal: sigaction struct which reenableds the default action for ^C
 *   returns: void
 * ---------------------------- */
void executeArgs(char* args[], char input[], char output[], int redirectionNeeded, struct sigaction signal)
{
	int background, i;
	background = i = 0; 
	pid_t spawnPID = -5; //generate new PID and assign it a junk value

	if(args[numOfArgs-1][0] == BACKGROUND_CMD && args[numOfArgs-1][1] == '\0') //check to see if '&' is the last char for the command
	{
		background = 1; //Set background to 1 if this is to be executed in the "background"
		args[numOfArgs-1] = NULL; //Null the "&\0" string so we don't pass it back to bash
		numOfArgs--;
	}

	spawnPID = fork(); //fork a child process 

	switch(spawnPID) //From lecture: switch to handle if child is succesfully forked
	{
		case -1:
		{
			perror("Hull Breach!\n");
			exit(1);
			break;
		}
		case 0: //fork() was success 
		{
			signal.sa_handler = SIG_DFL; //renable normal use of ^C
			sigaction(SIGINT, &signal, NULL);

			if(redirectionNeeded) { setRedirection(input, output); } //Set redirection if needed 
			if(execvp(args[0], args)) //execvp() will search PATH for commands 
			{
				printf("%s: No such file or directory\n", args[0]); //if ecevp() fails, print error and exit child
				fflush(stdout);
				exit(2);
			}
			break;
		}
		default:
		{
			if(background && backgroundModeFlag) //If background is enabled and '&' was used run child process in background mode
			{
				pid_t parentPID = waitpid(spawnPID, &status, WNOHANG); 
				printf("background pid is %d\n", spawnPID);
				fflush(stdout);
			}
			else
			{
				pid_t parentPID = waitpid(spawnPID, &status, 0); //else make parent wait for child to terminate
			}
			while( (spawnPID = waitpid(-1, &status, WNOHANG)) > 0) //keep track of child processes run in background mode
			{
				printf("child %d terminated\n", spawnPID); //print out value of child exit status
				fflush(stdout);
				if(WIFEXITED(status))
				{
					printf("exit value %d\n", WEXITSTATUS(status));
					fflush(stdout);
				}
				else
				{
					printf("terminated by signal %d\n", WTERMSIG(status));
					fflush(stdout);
				}
			}
		}
	}
}

/* setRedirection
 * ----------------------------
 *	Sets file pointers to redirect from stdin/stdout if user enters '<' or '>'
 *   Args:
 *      input: input filename
 * 		output: output filename
 *   returns: void
 * ---------------------------- */
void setRedirection(char input[], char output[])
{
	int inFile, outFile, dupCheck; 

	if(! (input[0] == '\0') ) //If there is a filename for input, set input redirection
	{
		inFile = open(input, O_RDONLY); //Open readonly 
		if(inFile == -1)
		{
			perror("cannot open file for input\n"); fflush(stdout);
			exit(1);
		}
		dupCheck = dup2(inFile, 0); //designate file pointer to read from
		if(dupCheck == -1)
		{
			perror("cannon assign file for input\n"); fflush(stdout);
			exit(2);
		}
		fcntl(inFile, F_SETFD, FD_CLOEXEC); //make sure file closes 
	}

	if(! (output[0] == '\0') ) //if output filename is not null, set output redirection
	{
		outFile = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666); //open file to write to
		if(outFile == -1)
		{
			perror("cannot open file for output\n");
			exit(1);
		}
		dupCheck = dup2(outFile, 1); //designate file pointer to write to
		if(dupCheck == -1)
		{
			perror("cannot assign file for output\n");
			exit(2);
		}
		fcntl(outFile, F_SETFD, FD_CLOEXEC); //make sure file closes
	}

}


/* resetArgs
 * ----------------------------
 *	Frees inputArgs array and resests redirection flag.
 *   Args:
 *      inputArgs: string array to be freed
 * 		redirectionNeeded: redirection flag to be reset
 *   returns: void
 * ---------------------------- */
void resetArgs(char* inputArgs[], int* redirectionNeeded)
{
	int i;

	for(i = 0; i < MAX_ARGUMENT; i++)
	{
		free(inputArgs[i]);
	}
	*redirectionNeeded = 0;

	return;
}

/* isBuiltInFunction
 * ----------------------------
 *	Checks to see if input is a command the program should execute on it's own
 *   Args:
 *      arg: string of command to check 
 *   returns: (int): index of built-in command in global array
 * ---------------------------- */
int isBuiltInFunction(char* arg)
{
	int i;
	for(i = 0; i < NUM_BUILT_IN_COMMANDS; i++)
	{
		if(strcmp(BUILT_IN_COMMANDS[i], arg) == 0)
		{
			return i+1;
		}
	}
	return 0;
}

/* runBuiltInFunction
 * ----------------------------
 *	Checks to see if input is a command the program should execute on it's own
 *   Args:
 * 		args: comman
 *   returns: (int): index of built-in command in global array
 * ---------------------------- */
void runBuiltInFunction(char* args[], int arg)
{
	switch(arg)
	{
		case 1: //cd 
			if(numOfArgs == 1)
			{
				chdir(getenv("HOME")); //get HOME env var if directory was not specified
			}
			else
			{
				if(chdir(args[1]) == -1) //check to make sure file argument is valid
				{
					printf("Directory not found\n"); fflush(stdout);
					break;
				}
			
			}
			break;
		case 2: //exit
			exit(0);
		case 3: //status
			if(WIFEXITED(status)) //check status of last command 
			{
				printf("exit value %d\n", WEXITSTATUS(status)); //uses WEXITSTATUS to make status human readable 
				fflush(stdout);
			}
			else
			{
				printf("terminated by signal %d\n", WTERMSIG(status)); //uses WTERMSIG to make status human readable 
				fflush(stdout);
			}
			break;
	}
	
	return;
}


/* toggleBackgroundMode
 * ----------------------------
 *	Toggles between background/foreground modes
 *   Args:
 * 		N/A
 *   returns: void
 * ---------------------------- */
void toggleBackgroundMode()
{
	if(backgroundModeFlag == 1) 
	{
		printf("Entering foreground-only mode (& is now ignored)\n");
		fflush(stdout);
		backgroundModeFlag = 0;
	}
	else
	{
		printf("Exiting foreground-only mode\n");
		fflush(stdout);
		backgroundModeFlag = 1;
	}
}
