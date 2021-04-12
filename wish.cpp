
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

/*** ERROR ***/
void throw_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

/*** build-in commands ***/
vector<string> paths = {"/bin/", "/usr/bin/"};
int my_exit() {
    exit(0);
}

// chdir()
// 0 or >1 args -> error
int my_cd(char** argv) {
    if (!argv[1] || argv[2]) {
        // cerr << "wish: \"cd\" only accepts 1 argumen." << endl;
        throw_error();
        return 1;
    }
    // try to chdir
    if (chdir(argv[1]) != 0) {
        throw_error();
        return 1;
    }
    return 0;   // success
}

int my_path(char** argv, int len) {
    vector<string> new_paths;
    for (int i = 1; i < len; i++) {
        new_paths.push_back(string(argv[i]));
    }
    paths = new_paths;
    return 0;
}

// testing
int PATH() {
    for (string path: paths) {
        cout<< path << endl;
    }
    return 0;
}




/*** functions acting on commands ***/
// unix API requires c-style string
bool is_command_char(char ch) {
    return isalpha(ch) || ch == '-' || ch == '/' || ch == '.';
}

string trim(string s) {
    int len = s.length();
    // trim left
    while (!is_command_char(s[0])) {
        s = s.substr(1);
        len--;
    }
    // trim right
    while (!is_command_char(s[len-1])) {
        s.pop_back();
        len--;
    }
    return s;
}

vector<string> parse_line(char* command, char delimiter, char end) {
    string line = "";
    vector<string> tokens;

    for (char* ch = command; *ch; ch++) {
        line += *ch;
        if (*ch == delimiter || *ch == '\n' || *ch == '\0') {
            tokens.push_back(trim(line) + end);
            line = "";
        }
    }
    return tokens;
}

pair<char**,int> parse_command(char* command) {
    vector<string> tokens = parse_line(command, ' ', '\0'); 
    int len = tokens.size();
    char** argv = new char*[len];
    for (int i = 0; i < len; i++) {
        argv[i] = (char*)tokens[i].c_str();
    }

    return make_pair(argv,len);
}

char* find_path(char* command){
    for (size_t i = 0; i < paths.size(); i++) {
        char* buff = new char[BUFF_SIZE];
        strcat(buff, (char*)paths[i].c_str());
        strcat(buff, command);
        if (access(buff, X_OK) == 0) {
            return buff;
        }
        delete buff; 
    }
    // cerr << "wish: command not found: " << command << endl;
    throw_error();
    exit(1);
}

int apply_command(char* line) {
    
    // parse the inpur line
    pid_t ret;
    vector<string> commands = parse_line(line, '&', '\n');
    vector<pair<char**,int>> args;
    for (size_t i = 0; i < commands.size(); i++) {
        args.push_back(parse_command((char*)commands[i].c_str()));
    }
    
    // check if it is build-in command
    // change to function pointer if time allows
    if (strcmp((args[0].first)[0], "exit") == 0)
        my_exit();
    if (strcmp((args[0].first)[0], "cd") == 0)
        return my_cd(args[0].first);
    if (strcmp((args[0].first)[0], "path") == 0)
        return my_path(args[0].first, args[0].second);
    if (strcmp((args[0].first)[0], "PATH") == 0)
        return PATH();
    
    // external commands
    for (size_t i = 0; i < args.size(); i++) {
        // apply the command
        ret = fork();
        if (ret < 0) {
            throw_error();
            return 1;
        } else if (ret == 0) {
            // child process,
            // excute the command here
            char** comm = args[i].first;
            char* path = find_path(comm[0]);
            execv(path, comm);
            delete path;
            delete comm;
        } else {
            // parent process,
            // wait until the child process finish
            wait(NULL);
        }
    }

    return 0;
}

/*** main program ***/
int start_shell(FILE* fp) {
    size_t len;
    // int rnt;
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
        apply_command(buff);
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





