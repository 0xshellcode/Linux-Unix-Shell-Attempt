#include <stdio.h> //For input & output
#include <stdlib.h> //Contains a lot of useful functions
#include <string.h> //Manipulating arrays of characters
#include <unistd.h> //Provides access to the POSIX operating system API
#include <sys/types.h>//Defines a collection of typedef symbols and structures
#include <sys/wait.h> //Use wait systemcall, for process

//Constant
#define MAX_LINE 80 // Max 80 chararters per command

char* history[10][MAX_LINE / 2];
int stop_history[10];
int megaBuffer = 0;


// Functions
void awake_history() {
    int i,j;
    for(i = 0; i < 10; i++) {
        for(j = 0; j < (MAX_LINE / 2); j++) {
            history[i][j] = NULL;
        }
        stop_history[i] = 0;
    }
}

void clean_history() {
    int i, j;
    for(i = 0;i < 10 && i < megaBuffer; i++) {
        for(j = 0;history[i][j] != NULL;j++) {
            if(history[i][j])
                free(history[i][j]);
        }
    }
}

void show_history() {
    int i,j;
    for(i = 0; i < 10 && i < megaBuffer;i++) {
        int index;
        if(megaBuffer > 10)
            index = megaBuffer - 9 + i;
        else
            index = i + 1;
        printf("[%d] ",index);
        for(j=0;history[(index-1)%10][j] != NULL; j++) {
            printf("%s ",history[(index - 1) % 10][j]);
        }
        if(stop_history[(index - 1) % 10] == 0) {
            printf("&");
        }
        printf("\n");
    }
}

char** history_function(char **args, int *needWait) {

    int i;

    if(args[1]==NULL && strcmp(args[0],"!!")==0) {
        if(megaBuffer > 0){
            strcpy(args[0],history[(megaBuffer - 1) % 10][0]); // strcpy(); Copies the string pointed to, by src to dest.
            for(i = 1; history[(megaBuffer - 1) % 10][i]!=NULL;i++) {
                args[i] = (char*)malloc((MAX_LINE + 1)*sizeof(char));
                strcpy(args[i],history[(megaBuffer - 1) % 10][i]); // strcpy(); Copies the string pointed to, by src to dest.
            }
            args[i] = NULL;
            *needWait = stop_history[(megaBuffer - 1) % 10];
        } else {
            printf("Commands not found in history\n");
            return args;
        }
    } else if(args[1] == NULL && args[0][0] == '!') {
        int index;
        char *cmdPointer=&(args[0][1]);
        if(sscanf(cmdPointer,"%d",&index)==1) {  // sscanf(); Reads formatted input from a string
            if(index > 0 && megaBuffer > index - 1 && index > megaBuffer - 9) {
                strcpy(args[0],history[(index - 1) % 10][0]); // strcpy(); Copies the string pointed to, by src to dest.
                for(i=1;history[(index - 1) % 10][i]!=NULL;i++) {
                    args[i] = (char*)malloc((MAX_LINE + 1)*sizeof(char));
                    strcpy(args[i],history[(index - 1) % 10][i]); // strcpy(); Copies the string pointed to, by src to dest.
                }

                args[i]   = NULL;
                *needWait = stop_history[(index - 1) % 10];

            } else {
                printf("Command not found in history (This is out of range)\n");
                return args;
            }
        } else {
            printf("Command not found in history (invalid index)\n");
            return args;
        }
    }


    for(i = 0; i < (MAX_LINE / 2) && history[megaBuffer % 10][i] != NULL; i++)
        free(history[megaBuffer % 10][i]);
    for(i = 0; args[i] != NULL;i++) {
        history[megaBuffer % 10][i] = args[i];
    }
    history[megaBuffer % 10][i] = args[i];
    stop_history[megaBuffer % 10] = *needWait;
    return history[(megaBuffer++) % 10];
}


//Main Function

int main(void) {
    int running = 1;
		char *args[MAX_LINE / 2];	/* command line (of 80) has max of 40 arguments */

    awake_history();
    while (running) {
        printf("osh → ");
        fflush(stdout); //Flushes the output buffer of a stream

        pid_t littleBoy; //Signed integer type

        char command_line[MAX_LINE + 1];
        char *cmdPointer = command_line; //Pointer of the command_line variable
        int arrayPosition = 0;
        if(scanf("%[^\n]%*1[\n]",command_line) < 1) {
            if(scanf("%1[\n]",command_line) < 1) {
                printf("The standard input failed\n");
                return 1;
            }
            continue; // It performs the next iteration
        }

        while(*cmdPointer == ' ' || *cmdPointer == '\t')
            cmdPointer++; //Increment (pointer address)
        while(*cmdPointer != '\0') { // '\0' to indicate the termination of a character string (Like a NULL)
            char *temporalBuffer = (char*)malloc((MAX_LINE+1)*sizeof(char)); //read-only parts of the memory, and making temporalBuffer a pointer to that makes
						//any writing operation on this memory illegal, malloc is used to request some amount of extra memory and convert that to char
            args[arrayPosition] = (char*)malloc((MAX_LINE + 1)*sizeof(char));
            int retention  = sscanf(cmdPointer,"%[^ \t]",args[arrayPosition]);  // sscanf(); Reads formatted input from a string
            cmdPointer    += strlen(args[arrayPosition]); // strlen(); Gives the exact length of a string without taking into account the value '\0'
            if(retention < 1){
                printf("Invalid Command\n");
                return 1;
            }
            retention = sscanf(cmdPointer,"%[ \t]",temporalBuffer);  // sscanf(); Reads formatted input from a string
            if(retention > 0)
                cmdPointer += strlen(temporalBuffer);
            arrayPosition++;
            free(temporalBuffer); //Deallocate memory
        }


        int waiting = 1;
        if(strlen(args[arrayPosition - 1]) == 1 && args[arrayPosition - 1][0] == '&') { //Concurrency
            waiting = 0;
            free(args[arrayPosition-1]); //Deallocate memory
            args[arrayPosition - 1]=NULL;
        } else {
            args[arrayPosition]=NULL;
        }
        if(strcmp(args[0],"exit")==0){ // strcmp(); Compares the string in args[0] with the argument (in this case is exit)
            clean_history();  //Call to the function to clear the history
            return 0;
        }
        //History Computation
        if(args[1]==NULL && strcmp(args[0],"history")==0) {
            show_history();
            continue; //It performs the next iteration
        }
        char **argcmdPointer = history_function(args, &waiting); // Takes the memory address of a pointer

        //Fork child to Execute args
        littleBoy = fork();
        if(littleBoy<0) {
            printf("Fork Failed\n");
            return 1;
        } else if(littleBoy==0) {
            if(execvp(argcmdPointer[0],argcmdPointer)) { //execvp stands for vector array path
                printf("Invalid Command\n");
                return 1;
            }
        } else {
            if(waiting) {
                while(wait(NULL) != littleBoy);
            }
            else {
                printf("[1]%d\n",littleBoy);
            }
        }
    }

	return 0;
}
