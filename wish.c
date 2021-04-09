
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define BUFF_SIZE 4096

char* path = "/bin";

/***** build-in commands *****/
void myexit(int exit_code);
int mycd(int argc, char* argv[]);
int mypath(int argc, char* argv[]);


/***** redirection *****/
int redirection(int argc, char* argv[], char* file);


/***** parallel commands *****/
int parallel(char* argv_list);

/***** main program *****/
/*
 * apply a command
 * @args 
 */
int apply_command(char* command) {
    printf("%s", command);
    if (strcmp("exit\n", command) == 0) {
        exit(0);
    }
    return 0;
}

int start_shell(FILE* fp) {
    size_t len;
    ssize_t read;
    char* buff = NULL;
    const char* prompt = (fp == stdin) ? "wish> " : "";
    int rnt;

    // read the file line by line
    while (1) {
        // prompt if interactive mode
        if (fp == stdin)    printf("%s", prompt);
        // read in from stdin or file
        if ((read = getline(&buff, &len, fp)) == -1)
            return read;
        // apply the command and get the return value
        rnt = apply_command(buff);
        if (rnt != 0)   return rnt;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    
    int rnt;
    // mode
    //   argc == 1 -> interactive mode, read from stdin
    //   otherwise -> batch mode, read in the batch file
    FILE* fp = (argc == 1) ? stdin : fopen(argv[1], "r");
    if (!fp) {
        return 1;
    }
    rnt = start_shell(fp);

    return rnt;
}

