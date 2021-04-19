
#include <iostream>
#include <string>

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
const char* bin = "/bin";
const char* usrbin = "/usr/bin";
char* paths[BUFF_SIZE] = {(char*)bin, (char*)usrbin};  // default search path
int paths_len = 2;

// * Helper Functions * //
/**
 * Dispaly the required error message.
 * @param code ERROR Code.
 * Exit the process/program if code is 0 or 1.
 */
void throw_error(int code) {
    char error_message[30] = "An error has occurred\n\0";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
    if (code != 0 && code != 1) {
        return;
    }
    exit(code);
}

/**
 * Check the command is a build-in command.
 * @param command Command in string.
 * @return If this command is a build-in command.
 */
bool is_buildin(char* command) {
    return strcmp(command, "exit") == 0 ||
           strcmp(command, "cd") == 0 ||
           strcmp(command, "path") == 0 ||
           strcmp(command, "PATH") == 0;
}

/**
 * Remove spaces at both end of a string.
 * @param str The string to be trim.
 * @return The pointer to new string starting location.
 */
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

// * build-in commands * //
/**
 * The build-in cd command.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
void my_cd(int argc, char** argv) {
    if (argc != 2) {
        throw_error(0);
    }
    if (chdir(argv[1]) != 0) {
        throw_error(1);
    }
}

/**
 * The build-in path command.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
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

/**
 * The build-in exit command.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
void my_exit(int argc, char** argv) {
    if (argc != 1) {
        throw_error(0);
    }
    exit(0);
}

/**
 * Build-in command to check the current path variable.
 */
void PATH() {
    for (int i = 0; i < paths_len; i++) {
        cout << paths[i] << endl;
    }
}

// * Command Operations * //
struct Command{
    int argc;       // number of arguments in this command
    int fd_out;     // file descripter for stdout
    int fd_err;     // file descripter for stderr
    char path[BUFF_SIZE];   // path to command
    char* argv[BUFF_SIZE];  // argument vector of this command

    /** Deafult Constructor **/
    Command() {
        this->argc = 0;
        this->fd_out = -1;
        this->fd_err = -1;
        memset(this->path, 0, sizeof(this->path));
        memset(this->argv, 0, sizeof(this->argv));
    }

    /**
     * Overload Constructor
     * @param line a line of command string.
     */
    Command(char* line) {
        char* separated[BUFF_SIZE] = {};    // seperated line by > redirection
        int count = 0;                      // count of >, at most 1
        int can_access;
        bool is_empty_here = false;

        
        // ! check for redirection
        while ((separated[count] = strsep(&line, ">")) != NULL) {
            count++;
        }
        count--;    // the above loop will add 1 extra
        if (count > 1) {
            throw_error(2);
        } else if (count == 0) {  // no redirection
            
            this->fd_out = STDOUT_FILENO;
            this->fd_err = STDERR_FILENO;
        } else {
            
            // check empty command
            if (strcmp(separated[0], "") == 0 || strcmp(separated[1], "") == 0) {
                is_empty_here = true;
            }

            separated[1] = trim(separated[1]); // redirection target location

            // redirection only support 1 file,
            // if there is space in target location after trim
            // it means there are more than 1 file location
            if (string(separated[1]).find(" ") != string::npos) {
                is_empty_here = true;
            }

            mode_t S_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            int O_FLAG = O_CREAT | O_RDWR | O_TRUNC;
            int fd;

            // try to open the redirected output file,
            // give a default command if failed.
            if ((fd = open(separated[1], O_FLAG, S_MODE)) < 0) {
                // throw_error(1);
                is_empty_here = true;
            }

            this->fd_out = fd;
            this->fd_err = fd;
        }


        // ! read in the command
        this->argc = 0;                     // count of argument
        separated[0] = trim(separated[0]);  // command string

        // parse the command line with respect to > redirection.
        while ((this->argv[this->argc] = strsep(&separated[0], " ")) != NULL) {
            this->argc++;
        }

        // trip all
        for (int i = 0; i < this->argc; i++) {
            this->argv[i] = trim(this->argv[i]);
        }

        // skip if build-in command
        if (is_buildin(this->argv[0])){
            strcpy(this->path, this->argv[0]);
        } else {
            for (int i = 0; i < paths_len; i++) {
                this->path[0] = '\0';   // reset buffer

                // add command name to a path variable.
                strcpy(this->path, paths[i]);
                strcat(this->path, "/");
                strcat(this->path, this->argv[0]);

                can_access = access(this->path, X_OK);
                if (can_access == 0) {
                    break;
                }
                // no more path to check, then the command is empty.
                if (can_access == -1 && i == paths_len - 1) {
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

/**
 * Check if a command is empty.
 * @param command The command to check.
 * @return if command is empty.
 */
bool is_empty_command(Command command) {
    return command.fd_out == -1 || command.fd_err == -1;
}


/**
 * Execute a line of command.
 * @param commands A list of command pre-parsed by '&'.
 * @param len Length of commands.
 * @return ERROR Code. 0 if success.
 */
int apply_command(Command commands[], int len) {
    pid_t pids[BUFF_SIZE];
    int status;
    

    for (int i = 0; i < len; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            throw_error(1);
        } else if (pids[i] == 0) {
            pids[i] = getpid(); // save pid for concurrent command execution

            // child process
            // execute the command here
            if (is_empty_command(commands[i])) {
                throw_error(1);
            }
            
            // redirect to this command's output location
            dup2(commands[i].fd_out, 1);
            dup2(commands[i].fd_err, 2);


            // execute 
            execv(commands[i].path, commands[i].argv);

            close(commands[i].fd_out);
            close(commands[i].fd_err);
            exit(0);
        } 
    }
    for (int i = 0; i < len; i++) {
        waitpid(pids[i], &status, 0);
    }
    return 0;
}

/**
 * Print the full detail of a command to STDOUT.
 * @param command The command you want to check.
 */
void printCommand(Command command) {
    cout << ">>>==========>" << endl;
    cout << "argc:  "   << command.argc << endl;        // number of arguments in this command
    cout << "fd_out:    " << command.fd_out << endl;    // file descripter for stdout
    cout << "fd_err:    " << command.fd_err << endl;    // file descripter for stderr
    cout << "path:  " << command.path << endl;          // path to command
    cout << "argv:" << endl;
    for (int i = 0; i < command.argc; i++)
        cout << "   " << i << ": " << command.argv[i] << endl;
    cout << "<==========<<<" << endl;
}

// * main program * //
/**
 * Display the prompt at begining of line.
 * @param prompt Your prompt with ending charactor '\0'.
 */
#define SHOW_PROMPT(prompt) write(STDOUT_FILENO, prompt, strlen(prompt))

/**
 * Start this shell session, reading and executing.
 * @param fp Input file.
 * @return ERROR code. 0 if success.
 */
int start_shell(FILE* fp) {

    size_t cap = 0;
    ssize_t len;
    char* buff = NULL;

    // prompt for interactive mode
    char prompt[] = "wish> ";
    if (fp != stdin) {
        prompt[0] = '\0';
    }


    // read from file/STDIN line by line
    while (SHOW_PROMPT(prompt), (len = getline(&buff, &cap, fp)) > 0) {
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
        while ((command_buff[count] = strsep(&buff, "&")) != NULL) {
            count++;
        }

        // store the commands in Command struct and in array
        for (int i = 0; i < count; i++) {
            commands[i] = Command(command_buff[i]);

            #ifdef _SHOW_COMMAND_INFO_
            printCommand(commands[i]);
            #endif
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

