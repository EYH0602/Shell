
#include <iostream>
#include <string>
#include <vector>
#include <utility>  // pair

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <wait.h>

#define BUFF_SIZE 4096

using namespace std;

/*** functions acting on commands ***/
// unix API requires c-style string
pair<char**, int> parser(char* command) {
    string line = "";
    vector<string> tokens;

    for (char* ch = command; *ch; ch++) {
        line += *ch;
        if (*ch == ' ' || *ch == '\n') {
            tokens.push_back(line);
            line = "";
        }
    }
    
    int len = tokens.size();
    char** argv = new char*[len];
    for (int i = 0; i < len; i++) {
        argv[i] = (char*)tokens[i].c_str();
    }

    return make_pair(argv, len);
}

char* find_path(char* command){
    char* buff = new char[BUFF_SIZE];
    char* buff2 = new char[BUFF_SIZE];
    char bin[] = "/bin/";
    char usrbin[]= "/usr/bin/";
    
    strcat(buff, bin);
    strcat(buff, command);
    buff[string(buff).length() - 1] = '\0';
    if (access(buff, X_OK) == 0) {
        delete buff2;
        return buff;
    }
    
    strcat(buff2, usrbin);
    strcat(buff2, command);
    buff2[string(buff2).length() - 1] = '\0';
    if (access(buff2, X_OK) == 0) {
        delete buff;
        return buff2;
    }

    cerr << "wish: command not found: " << command << endl;
    exit(1);
}

int apply_command(char* command) {
    printf("%s", command);
    if (strcmp("exit\n", command) == 0) {
        exit(0);
    }
    
    pair<char**, int> parsed = parser(command);
    char** argv = parsed.first;
    // int len = parsed.second;    // only for testing perpose
    
    // apply the command
    int ret = fork();
    if (ret < 0) {
        cerr << "fold failed" << endl;
        return 1;
    } else if(ret == 0) {
        // child process,
        // excute the command here
        char* path = find_path(argv[0]);
        execv(path, argv);
        delete path;
    } else {
        // parent process, wait here
        wait(NULL);
    }

    delete argv;
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





