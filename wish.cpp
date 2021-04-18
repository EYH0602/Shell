
#include <iostream>
#include <string>
#include <vector>
#include <utility>  // pair

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
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
    if (!argv[1]) {
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

int my_path(vector<string> paths_in) {
    paths_in.erase(paths_in.begin());
    paths = paths_in;
    // for (int i = 1; argv[i] != NULL; i++) {
    //     new_paths.push_back(string(argv[i]));
    // }
    return 0;
}

// testing
int PATH() {
    for (string path: paths) {
        string path_with_newline = path + '\n';
        write(STDOUT_FILENO, path_with_newline.c_str(), path_with_newline.length());
    }
    return 0;
}




/*** functions acting on commands ***/
// unix API requires c-style string
bool is_command_char(char ch) {
    return isalpha(ch) || 
           isdigit(ch) ||
           ch == '-' || 
           ch == '/' || 
           ch == '.' || 
           ch == '>' || 
           ch == '!' ||
           ch == '?';
}

string trim(string s) {
    int len = s.length();
    // trim left
    while (!is_command_char(s[0]) && len > 0) {
        s = s.substr(1);
        len--;
    }
    // trim right
    while (!is_command_char(s[len-1]) && len > 0) {
        s.pop_back();
        len--;
    }
    return s;
}

vector<string> parse_line(char* command, char delimiter, char end) {
    string line = "";
    vector<string> tokens;
    bool hasBegin = false;

    for (char* ch = command; *ch; ch++) {
        if (!hasBegin && *ch == ' ')
            continue;
        hasBegin = true;
        line += *ch;
        if (*ch == delimiter || *ch == '\n' || *ch == '\0') {
            tokens.push_back(trim(line) + end);
            line = "";
        }
    }
    return tokens;
}

char** parse_command(char* command, bool is_buildin) {
    vector<string> tokens = parse_line(command, ' ', '\0'); 
    int len = tokens.size();
    char** argv = new char*[len];
    for (int i = 0; i < len; i++) {
        char* comm = (char*)tokens[i].c_str();
        comm[tokens[i].length()] = '\0';
        argv[i] = comm;
    }
    if (is_buildin)
        argv[len] = NULL;
    return argv;
}

char* find_path(char* command){
    for (size_t i = 0; i < paths.size(); i++) {
        char* buff = new char[paths[i].length() + 1];
        strcpy(buff, paths[i].c_str());
        // string path_with_end = paths[i] + '/';
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

string first(string sentence) {
    string res = "";
    for (char ch : sentence) {
        if (ch == ' ' || ch == '\n') {
            break;
        }
        res += ch;
    }
    return res;
}

int fd = STDOUT_FILENO;

int update_file_descriptor(string& command) {
    vector<string> words;
    string comm_new = "";
    int count = 0;
    bool hasBegin = false;

    for (char ch : command) {
        if (!hasBegin && ch == ' ')
            continue;
        hasBegin = true;
        if (ch == '>') {
            if (comm_new != "")
                words.push_back(trim(comm_new));
            words.push_back(">");
            comm_new = "";
            count++;
            continue;
        }
        comm_new += ch;
        if (ch == '\n' || ch == '\0') {
            if (comm_new != "\n" && comm_new != "\0" && comm_new != " ") {
                words.push_back(trim(comm_new));
            }
            comm_new = "";
        }
    }
    // check valid syntax
    if (count > 1) {
        throw_error(); 
        exit(0);
    }
    // for (size_t i = 0; i < words.size(); i++) {
    //     cout << i << ": " << words[i] << endl;
    // }

    // cout << words.size() << endl;

    comm_new = "";
    for (size_t i = 0; i < words.size(); i++) {
        // cout << words[i] << "    " << (words[i] == ">") << endl;
        if (words[i] == ">") {
            if (i+1 >= words.size() || i == 0) {
                throw_error();
                exit(0);
            }
            if (words[i+1].find(" ") != string::npos) {
                throw_error();
                exit(0);
            }

            mode_t S_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            int O_FLAG = O_CREAT | O_RDWR;
            fd = open(words[i+1].c_str(), O_FLAG, S_MODE);
            if (fd < 0) {
                throw_error();
                return 1;
            }

            dup2(fd,1);
            dup2(fd,2);
            close(fd);
            break;
        }
        comm_new += words[i];
    }
    command = comm_new + '\n';
    return 0;
}

int apply_command(char* line) {
    // parse the input line
    pid_t ret;
    vector<string> commands = parse_line(line, '&', '\n');
    // for (string cm : commands) {
    //     cout << "=>" << cm << "<=" << endl;
    // }
    

    // check if it is build-in command
    // change to function pointer if time allows
    if (first(commands[0]) == "exit") {
        if (commands[0] != "exit\n") {
            throw_error();
            exit(0);
        }
        my_exit();
    }
    if (first(commands[0]) == "cd")
        return my_cd(parse_command((char*)commands[0].c_str(), true));
    if (first(commands[0]) == "path")
        return my_path(parse_line((char*)commands[0].c_str(), ' ', '/'));
    if (first(commands[0]) == "PATH")
        return PATH();

    
    // external commands
    for (int i = 0; i < (int)commands.size(); i++) {
        // apply the command
        ret = fork();
        if (ret < 0) {
            throw_error();
            return 1;
        } else if (ret == 0) {
            // child process,
            // excute the command here
            
            // check if there is redirection
            update_file_descriptor(commands[i]);
            char** argv = parse_command((char*)commands[i].c_str(), false);
            char* path = find_path(argv[0]);
            strcpy(argv[0], path);
            for (int i = 0; argv[i] != NULL; i++) {
                cout << ">>" << argv[i] << "<<" << endl;
            }
            execv(path, argv);
            delete path;
            delete argv;
            exit(0);
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
    char prompt[] = "wish> ";
    bool hasStarted = false;

    // read from file line by line
    while (1) {
        // prompt if interactive mode
        if (fp == stdin)
            write(STDOUT_FILENO, prompt, 6);
        // read in from stdin or file, stop if EOF
        if(getline(&buff, &len, fp) == EOF) {
            // cout << "EOF" << endl;
            //  exit(1);
            if (!hasStarted) {
                throw_error();
                return 1;
            }
            return 0;
        }
        if (len == 0) {
            throw_error();
            return 1;
            // cout << 0 << " len" << endl;
        }
        hasStarted = true;
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
        throw_error();
        return 1;
    }

    rnt = start_shell(fp);

    return rnt;
}





