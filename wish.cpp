
#include <iostream>
#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

#define BUFF_SIZE 4096

using namespace std;

/*** functions acting on commands ***/
char** parser(char* command) {
    string line = "";
    vector<string> tokens;

    for (char* ch = command; *ch; ch++) {
        line += *ch;
        if (*ch == ' ' || *ch == '\n') {
            tokens.push_back(line);
            line = "";
        }
    }
    
    int len = tokens.length();
    char* argv[len];
    for (int i = 0; i < len i++) {
        argv[i] = tokens[i].c_str();
    }

    return argv;
}

int apply_command(char* command) {
    printf("%s", command);
    if (strcmp("exit\n", command) == 0) {
        exit(0);
    }
    
    vector<string> argv = parser(command);
    return 0;
}

/*** main program ***/
int start_shell(FILE* fp) {
    size_t len;
    int rnt;
    char* buff = NULL;

    // read from file line by line
    while (1) {
        // prompt if interactive mode
        if (fp == stdin)
            printf("wish> ");
        // read in from stdin or file, stop if EOF
        if(getline(&buff, &len, fp) == EOF)
            return 0;
        // apply the command and get the return value
        buff[len] = '\0';
        if ((rnt = apply_command(buff)) != 0)
            return rnt;
    }
    // otherwise the commands are applied successfully
    return 0;
}

int main(int argc, char* argv[]) {
    int rnt;

    // mode
    //  argc == 1 -> interactive mode, read from stdin
    //  otherwise -> batch mode, read in the batch file
    FILE* fp = (argc == 1) ? stdin : fopen(argv[1], "r");

    if (!fp) {
        return 1;
    }

    rnt = start_shell(fp);

    return rnt;
}





