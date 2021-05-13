// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <linux/limits.h>
#include <signal.h>
#include <ctype.h>
#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)

#define HISTORY_DEPTH 10

char prevCommand[COMMAND_LENGTH] = "";
int commandPosition;
char history[HISTORY_DEPTH][COMMAND_LENGTH];
int historyRow;
int argCount;
bool exclamationError = false;
bool sigHandled = false;
/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{	
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */

//search the history array for nth command.
//replace buff with the nth command if it exists so it can be tokenized
void exclamation(char* buff){
	int i = 0;
	int x = 0;
	char rowNum[10000];
	//!! was entered
	if(buff[1] == '!' && buff[2] == '\0'){
		while(history[historyRow][i] != '	'){
			i++;
		}
		i++;
		strcpy(buff, &history[historyRow][i]);
		return;
	}
	//numeric argument given, search the table for the row
	else if(isdigit(buff[1]) != 0){
		for(i = 0; i < 10 && history[i][0] != 0; i++){
			//get the number for each row
			x = 0;
			while(history[i][x] != '	'){
				rowNum[x] = history[i][x];
				//printf("x is %d\n", x);
				x++;
			}
			rowNum[x] = '\0';
			//compare rowNum with the numeric argument given
			if(strcmp(&buff[1], rowNum) == 0){
				//copy the command into buff
				x++;
				strcpy(buff, &history[i][x]);
				return;
			}
		}
		//reached the end. The command was not found
		write(1, "Error, command not found\n", strlen("Error, command not found\n"));
		exclamationError = true;
		return;
	}
	//invalid arguments
	write(1,"Error invalid command\n", strlen("Error invalid command\n"));
	exclamationError = true;
}
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;
	int* argCountPtr = &argCount;
	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);
	if(sigHandled == true){
		return;
	}
	if (length < 0) {
		perror("Unable to read command from keyboard. Terminating.\n");
		exit(-1);
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}
	//is exclamation point is entered, change the buffer
	if(buff[0] == '!'){
		exclamation(buff);
	}
	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	*argCountPtr = token_count - 1;
	
	if (token_count == 0) {
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		*argCountPtr -= 1;
		tokens[token_count - 1] = 0;
	}
}

/**
 * Main and Execute Commands
 */

//Function to execute commands
int exitBuiltin(char *commands[]){
	if(argCount > 0){
		write(1, "Error, exit does not take arguments\n", strlen("Error, exit does not take arguments\n"));
		return 2;
	}
	return 0;
}
int pwdBuiltin(char *commands[]){
	if(argCount > 0){
		write(1, "Error pwd does not take arguments\n", strlen("Error pwd does not take arguments\n"));
		return 2;
	}
	char directory[PATH_MAX];
	getcwd(directory, sizeof(directory));
	write(1, directory, strlen(directory));
	write(1, "\n", 2);
	return 1;
}

int cdBuiltin(char *commands[]){
	static char prevDir[PATH_MAX];
	static char currDir[PATH_MAX];
	static int x = 0;

	//initialize prevDir and currDir
	while(x < 1){
		getcwd(currDir, sizeof(currDir));
		strcpy(prevDir, currDir);
		x++;
	}
	if(argCount > 1){
		write(1, "Error more than one argument provided\n", strlen("Error more than one argument provided\n"));
		return 2;
	}

	//source:https://www.tutorialspoint.com/unix/unix-environment.html
	if(argCount == 0){
		chdir(getenv("HOME"));
		strcpy(prevDir, currDir);
		getcwd(currDir, sizeof(currDir));
		return 1;
	}

	if(strncmp(&commands[1][0], "~", 1) == 0){
		if(strlen(commands[1]) == 1){
			chdir(getenv("HOME"));
			strcpy(prevDir, currDir);
			getcwd(currDir, sizeof(currDir));
			return 1;
		}
		
		char absBuffer[COMMAND_LENGTH] = "";
		char* home = getenv("HOME");
		strcat(absBuffer, home);
		strcat(absBuffer, &commands[1][1]);
		if(chdir(absBuffer) != 0){
			write(1,"Error: Directory does not exist\n", strlen("Error: Directory does not exist\n"));
			return 2;
		}
		strcpy(prevDir, currDir);
		getcwd(currDir, sizeof(currDir));
		return 1;
	}
	
	if(strcmp(commands[1], "-") == 0){
		if(argCount > 1){
			write(1, "Error: Invalid arguments\n", strlen("Error: Invalid arguments\n"));
		}
		chdir(prevDir);
		strcpy(prevDir, currDir);
		getcwd(currDir, sizeof(currDir));
		return 1;
	}
	if(chdir(commands[1]) != 0){
		write(1,"Error: Directory does not exist\n", strlen("Error: Directory does not exist\n"));
		return 2;
	}
	strcpy(prevDir, currDir);
	getcwd(currDir, sizeof(currDir));
	return 1;
}

int help(char* commands[], char* commandsBuiltin[], int numofCommands){
	//No argument is provided
	char* helpStatements[COMMAND_LENGTH] = {"is a builtin command that exits the program", "is a builtin command that displays the current working directory", 
	"is a builtin command that changes the current working directory", "is a builtin command to help", "is a builtin command that returns the last 10 commands entered"};
	if (argCount == 0){
		for(int i = 0; i < numofCommands; i++){
			write(1, commandsBuiltin[i], strlen(commandsBuiltin[i]));
			write(1, " ", 1);
			write(1, helpStatements[i], strlen(helpStatements[i]));
			write(1, "\n", 2);
		}
		return 1;
	}
	//One argument is provided
	//check if the argument matches a builtin
	else if(argCount == 1){
		for(int j = 0; j < numofCommands; j++){
			if(strcmp(commands[1],commandsBuiltin[j]) == 0){
				write(1, commands[1], strlen(commands[1]));
				write(1," ", 1);
				write(1, helpStatements[j], strlen(helpStatements[j]));
				write(1, "\n", 2);
				return 1;
			}
		}
		write(1, commands[1], strlen(commands[1]));
		write(1, " ",1);
		write(1, "is an external command or application\n", strlen("is an external command or application\n"));		
	}
	else{
		write(1,"Error, more than one argument provided\n", strlen("Error, more than one argument provided\n"));
		return 1;
	}
	return 1;
}
//display 10 most recent commands entered
int historyBuiltin(char* commands[]){
	//no arguments entered
	if(argCount > 0){
		write(1,"Error, history does not take arguments\n", strlen("Error, history does not take arguments\n"));
		return 2;
	}
	int i = 10;
	int x = historyRow+10;
	int row = x%10;
	//write out the row if its not empty
	while (i > 0 && strlen(&history[row][0]) != 0){
		write(1, &history[row][0], strlen(&history[row][0]));
		write(1,"\n", 2);
		i--;
		x--;
		row = x%10;
	}
	//write(1,"End of historyBuiltin\n", strlen("End of historyBuiltin\n"));
	return 1;
}
		
int executeBuiltins(char *commands[]){
	char* commandsBuiltin[] = {"exit", "pwd", "cd", "help", "history"};
	int numofCommands = sizeof(commandsBuiltin)/sizeof(commandsBuiltin[0]);
	if(strcmp(commands[0], "exit") == 0){
		return exitBuiltin(commands);
	}
	if(strcmp(commands[0], "pwd") == 0){
		return pwdBuiltin(commands);
	}
	if(strcmp(commands[0], "cd") == 0){
		return cdBuiltin(commands);
	}
	if(strcmp(commands[0], "help") == 0){
		return help(commands, commandsBuiltin, numofCommands);
	}
		//history command
	if(strcmp(commands[0], "history") == 0){
		return historyBuiltin(commands);
	}
	return 3;
}

void copyBuffer(char* historyBuffer, char* tokens[], _Bool* in_background){
	strcat(historyBuffer, tokens[0]);
	for(int i = 1; tokens[i] != NULL; i++){
		historyBuffer[strlen(historyBuffer)+1] = '\0';
		historyBuffer[strlen(historyBuffer)] = ' ';
		strcat(historyBuffer, tokens[i]);
	}
	//add &
	if(*in_background == true){
		historyBuffer[strlen(historyBuffer)+1] = '\0';
		historyBuffer[strlen(historyBuffer)] = ' ';
		strcat(historyBuffer, "&");
	}

}

//Takes tokens, and buffers for columns and adds them to history 2d array
void addHistory(char* tokens[], char* rowLabelBuffer, char* historyBuffer, _Bool* in_background){
	//Adding the label
	static int rowLabel = 1;
	static int recentRow = 0;
	int* historyRowPtr = &historyRow;
	//copy tokens into one string in historyBuffer
	//If its the same as the previous command entered then return without adding to history.
	copyBuffer(historyBuffer, tokens, in_background);
	if(strcmp(historyBuffer, prevCommand) == 0){
		return;
	}
	*historyRowPtr = recentRow%10;
	//convert char* using sprintf
	sprintf(rowLabelBuffer, "%d", rowLabel);
	//Add row label, then separate it from the command with a tab
	strcpy(&history[historyRow][0], rowLabelBuffer);	
	int i = strlen(&history[historyRow][0]);
	strcpy(&history[historyRow][i], "	");
	i++;
	commandPosition = i;
	//keep track of the previous command
	strcpy(prevCommand, historyBuffer);
	//starting just after the tab, add in the command to the row
	int j = 0;
	while(historyBuffer[j] != '\0'){
		history[historyRow][i] = historyBuffer[j];
		i++;
		j++;
	}
	rowLabel++;
	recentRow++;
}

/* Signal handler function */
void handle_SIGINT(){
	write(STDOUT_FILENO, "\n", strlen("\n"));
	char* help[3] = {"help"};
	executeBuiltins(help); 
	sigHandled = true;
}	
//dump out arguments
/*void clean(char* tokens[], _Bool *in_background){
	for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		if (*in_background == true) {
			write(STDOUT_FILENO, "Run in background.\n", strlen("Run in background.\n"));
		}
}*/
int main(int argc, char* argv[])
{
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];
	//bool firstIteration = false;
	while (true) {

		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		//Make shell prompt always display current working directory 
		_Bool in_background = false;
		exclamationError = false;

		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));
		int cwdlen = strlen(cwd);
		cwd[cwdlen] = '$';
		cwd[cwdlen+1] = ' ';
		cwd[cwdlen+2] = '\0';
		write(STDOUT_FILENO, cwd, strlen(cwd));

		char rowLabelBuffer[COMMAND_LENGTH] = "";
		char historyBuffer[COMMAND_LENGTH] = "";
		//handle signals
		//source: https://stackoverflow.com/questions/40231565/why-my-signal-handler-is-not-working-using-sigaction-function
		//https://unix.stackexchange.com/questions/80044/how-signals-work-internally
		sigHandled = false;
		struct sigaction handler;
		handler.sa_handler = &handle_SIGINT;
		handler.sa_flags = SA_RESETHAND;
		sigemptyset(&handler.sa_mask);
		sigaction(SIGINT, &handler, NULL);
		read_command(input_buffer, tokens, &in_background);
		if(sigHandled == true || exclamationError == true){
			continue;
		}
		int execvpStatus = 1;
		//User only pressed enter
		if(tokens[0] == NULL){
			continue;
		}
		
		//3: external command was run
		//2: builtin was entered with incorrect arguments
		//1: builtin successfully run, loop back to beginning
		//0: exit shell
		addHistory(tokens, rowLabelBuffer, historyBuffer, &in_background);
		
		//firstIteration = true;
		int builtinStatus = executeBuiltins(tokens);
		if(builtinStatus == 0){
			exit(0);
		}
		//dont fork if a builtin was run
		if(builtinStatus == 1 || builtinStatus == 2){
			//clean(tokens, &in_background);
			continue;
		}

		//********PART 1*********
		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */
		else{
			pid_t var_pid;
			var_pid = fork();
			if (var_pid < 0) {
				fprintf (stderr, "fork Failed");
				exit(-1);
			}

			//Execute the child process
			else if(var_pid == 0){
				execvpStatus = execvp(tokens[0], tokens);
				if (execvpStatus == -1){
					write(1, "Error unknown command\n", strlen("Error unknown command\n"));
					exit(-1);
				}
			}
			//The parent process
			else{
				//child does not run in background, so parent waits
				int status;
				if(in_background == false){
					//write(1,"not in background, call waitpid\n",33);
					waitpid(var_pid, &status, 0);
				}
			}
			// DEBUG: Dump out arguments:
			//clean(tokens, &in_background);
		}
	}
	return 0;
}