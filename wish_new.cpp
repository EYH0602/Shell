
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

#define BUFF_SIZE 200
#define EMPTY_COMMAND = -1

using namespace std;

// * system settings * //
char* paths[BUFF_SIZE] = {"/bin", "/usr/bin"};  // default search path
int paths_len = 2;

void throw_error(int code) {
    char error_message[30] = "An error has occurred\n\0";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
    if (code != 0 && code != 1) {
        // cout << "here!" << endl;
        return;
    }
    exit(code);
}

bool is_buildin(char* command) {
    return strcmp(command, "exit") == 0 ||
           strcmp(command, "cd") == 0 ||
           strcmp(command, "path") == 0 ||
           strcmp(command, "PATH") == 0;
}

// * string operations * //
char* trim(char* str) {
    // form head
    while (isspace(*str) && *str) {
        str++;
    }

    // if reach, return
    if (!*str) {
        return str;
    }

    // from tail, put a '\0' after the last nonspace char
    char* end = str + strlen(str) - 1;
    while (isspace(*end) && end != str) {
        end--;
    }

    *(end + 1) = '\0';
    return str;
}

// command structure
struct Command{
    int argc;       // number of arguments in this command
    int fd_out;     // file descripter for stdout
    int fd_err;     // file descripter for stderr
    char path[BUFF_SIZE];   // path to command
    char* argv[BUFF_SIZE];  // argument vector of this command

    Command() {
        this->argc = 0;
        this->fd_out = -1;
        this->fd_err = -1;
        memset(this->path, 0, sizeof(this->path));
        memset(this->argv, 0, sizeof(this->argv));
    }

    Command(char* line) {
        char* separated[BUFF_SIZE] = {};    // seperated line by > redirection
        int count = 0;                      // count of >, at most 1
        int can_access;
        bool is_empty_here = false;

        
        // ! check for redirection
        while ((separated[count] = strsep(&line, ">")) != NULL) {
            // cout << separated[count] << endl;
            count++;
        }
        count--;    // the above loop will add 1 extra
        // cout << count << endl;
        // cout << "[[[" << count << "]]]" << endl;
        if (count > 1) {
            throw_error(2);
        } else if (count == 0) {  // no redirection
            
            this->fd_out = STDOUT_FILENO;
            this->fd_err = STDERR_FILENO;
        } else {
            if (strcmp(separated[0], "") == 0 || strcmp(separated[1], "") == 0) {
                // throw_error(2);
                is_empty_here = true;
            }


            separated[1] = trim(separated[1]); // redirection target location

            // redirection only support 1 file,
            // if there is space in target location after trim
            // it means there are more than 1 file location
            if (string(separated[1]).find(" ") != string::npos) {
                // throw_error(0);
                is_empty_here = true;
            }

            mode_t S_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            int O_FLAG = O_CREAT | O_RDWR | O_TRUNC;
            int fd;

            if ((fd = open(separated[1], O_FLAG, S_MODE)) < 0) {
                // throw_error(1);
                is_empty_here = true;
            }

            this->fd_out = fd;
            this->fd_err = fd;
        }


        // ! read in the command
        this->argc = 0;                  // count of argument
        separated[0] = trim(separated[0]);  // command string

        while ((this->argv[this->argc] = strsep(&separated[0], " ")) != NULL) {
            this->argc++;
        }

        // cout << this->argc << endl;

        // trip all
        for (int i = 0; i < this->argc; i++) {
            this->argv[i] = trim(this->argv[i]);
            // cout << this->argv[i] << endl;
        }

        // skip if build-in command
        if (is_buildin(this->argv[0])){
            strcpy(this->path, this->argv[0]);
        } else {
            // cout << "here" << endl;
            for (int i = 0; i < paths_len; i++) {
                this->path[0] = '\0';   // reset buffer
                // printf("!!!-> %d\n", i);
                // for (int i = 0; i < paths_len; i++) {
                //     cout << paths[i] << endl;
                // }
                // printf("<- !!!\n");
                strcpy(this->path, paths[i]);
                strcat(this->path, "/");
                strcat(this->path, this->argv[0]);
                // cout << this->path << endl;
                can_access = access(this->path, X_OK);
                if (can_access == 0) {
                    break;
                }
                if (can_access == -1 && i == paths_len-1) {
                    is_empty_here = true;
                }
            }
        }

        if (is_empty_here){
            this->path[0] = '\0';   // reset buffer
            strcpy(this->path, this->argv[0]);
            // use -1 tp denote empty command
            this->fd_out = -1;
            this->fd_err = -1;
        }

    }
};

bool is_empty_command(Command com) {
    return com.fd_out == -1 || com.fd_err == -1;
}

bool has_empty(Command comms[], int len) {
    for (int i = 0; i < len; i++) {
        if (is_empty_command(comms[i])) {
            return true;
        }
    }
    return false;
}

// const Command EMPTY_COMMAND = Command();

// * build-in commands * //
void my_cd(int argc, char** argv) {
    if (argc != 2) {
        throw_error(0);
    }
    if (chdir(argv[1]) != 0) {
        throw_error(1);
    }
}

void my_path(int argc, char** argv) {
    // no arguments, reset search path to empty
    if (argc == 1) {
        memset(&paths[0], 0, sizeof(paths));
        paths_len = 0;
    }
    paths_len = 0;
    for (int i = 1; i < argc; i++) {
        paths[i-1] = strdup(argv[i]);
        paths_len += 1;
    }
}

void my_exit(int argc, char** argv) {
    if (argc != 1) {
        throw_error(0);
    }
    exit(0);
}

void PATH() {
    for (int i = 0; i < paths_len; i++) {
        cout << paths[i] << endl;
    }
}

void close_all_fd(Command commands[], int len) {
    for (int i = 0; i < len; i++) {
        if (commands[i].fd_out != STDOUT_FILENO) {
            close(commands[i].fd_out);
        }
        if (commands[i].fd_err != STDERR_FILENO) {
            close(commands[i].fd_err);
        }
    }
}

#define SHOW_PROMPT write(STDOUT_FILENO, prompt, strlen(prompt))

int apply_command(Command commands[], int len) {
    pid_t pids[BUFF_SIZE];
    int status;
    

    for (int i = 0; i < len; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            throw_error(1);
        } else if (pids[i] == 0) {
            // child process
            // execute the command here
            if (is_empty_command(commands[i])) {
                throw_error(1);
            }

            // int out = dup(fileno(stdout));
            // int err = dup(fileno(stderr));
            
            dup2(commands[i].fd_out, 1);
            dup2(commands[i].fd_err, 2);

            // if (commands[i].fd_out != STDOUT_FILENO) {
            //     char emp[BUFF_SIZE];
            //     memset(emp, 0, sizeof(emp));
            //     write(commands[i].fd_out, emp, BUFF_SIZE);
            // }

            // if (dup2(commands[i].fd_out, 1) == -1) {
            //     return errno;
            // }
            // if (dup2(commands[i].fd_err, 2) == -1) {
            //     return errno;
            // }


            // cout << commands[i].path << "  " << commands[i].argv[0] << endl;
            execv(commands[i].path, commands[i].argv);



            // fflush(stdout);
            // fflush(stderr);
            close(commands[i].fd_out);
            close(commands[i].fd_err);
            // dup2(out, fileno(stdout));
            // dup2(err, fileno(stderr));
            // close(out);
            // close(err);
            // exit(errno);
        } 
        else {
            wait(NULL);
        }
    }
    // for (int i = 0; i < len; i++) {
    //     waitpid(pids[i], &status, 0);
    // }
    return 0;
}

void printCommand(Command command) {
    cout << "argc:  "   << command.argc << endl;;      // number of arguments in this command
    cout << "fd_out:    " << command.fd_out << endl;;    // file descripter for stdout
    cout << "fd_err:    " << command.fd_err << endl;;    // file descripter for stderr
    cout << "path:  " << command.path << endl;;   // path to command
    cout << "argv:" << endl;
    for (int i = 0; i < command.argc; i++)
        cout << "   " << i << ": " << command.argv[i] << endl;
    cout << "==========" << endl;
}

// * main program * //
int start_shell(FILE* fp) {

    size_t cap = 0;
    ssize_t len;
    char* buff = NULL;

    // prompt for interactive mode
    char prompt[] = "wish> ";
    if (fp != stdin) {
        prompt[0] = '\0';
    }


    // // read from file/STDIN line by line
    while (SHOW_PROMPT, len = getline(&buff, &cap, fp) > 0) {
        buff = trim(buff); // move pointer to the first nonspace char 

        // skip empty line
        if (strcmp(buff, "") == 0) {
            continue;
        }
        // cout << "[][][] " << buff << endl;

        Command commands[BUFF_SIZE];
        char* command_buff[BUFF_SIZE];
        int count = 0;

        // separate commands with &
        while (command_buff[count] = strsep(&buff, "&")) {
            count++;
        }

        // cout << "<<<" << count << ">>>" << endl;

        // store the commands in Command struct and in array
        for (int i = 0; i < count; i++) {
            commands[i] = Command(command_buff[i]);
            // printCommand(commands[i]);
        }



        if (count == 1) {
            // check if it is build in command
            if (strcmp(commands[0].argv[0], "exit") == 0) {
                my_exit(commands[0].argc, commands[0].argv);
                continue;
            }
            if (strcmp(commands[0].argv[0], "path") == 0) {
                my_path(commands[0].argc, commands[0].argv);
                continue;
            }
            if (strcmp(commands[0].argv[0], "cd") == 0) {
                my_cd(commands[0].argc, commands[0].argv);
                continue;
                return 0;
            }
            if (strcmp(commands[0].argv[0], "PATH") == 0) {
                PATH();
                continue;
            }


            // not a buildin command
            if (paths_len == 0) {
                throw_error(2);
            }
            if (apply_command(commands, count) != 0) {
                throw_error(1);
            }

        } else {
            if (apply_command(commands, count) != 0) {
                throw_error(1);
            }
        }

        // close_all_fd(commands, count);
    }
    fclose(fp);
    return 0; 
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        throw_error(1);
    }
    FILE* fp = (argc == 1) ? stdin : fopen(argv[1], "r");


    if (!fp) {
        throw_error(1);
    }
    int rnt = start_shell(fp);
    return rnt;
}

