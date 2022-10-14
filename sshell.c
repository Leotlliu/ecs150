#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define MAX_ARGUMENT 16
#define MAX_TOKEN_LEN 32
#define MAX_PIPES 4
#define FAIL_CD -1

// The basic command structure
// cmd: the command entered by user
// args: arguments followed by the command
// filename: will be used whenever there is a '<' or '>'
struct Command {
        char * cmd;
        char *args[MAX_ARGUMENT+1];
        char *filename;
};

struct Node {
        char curstack[CMDLINE_MAX];
        struct Node *next;
};

// The pipeline structure
// cmds: commands list
// pipes: pipe list
struct Pipelines {
        struct Command *cmds[MAX_PIPES];
        int *pipes[MAX_PIPES];
};

// Error structure
// flag: when equal to 1, means having an error
// msg: error message
struct Error {
        int flag;
        char msg[CMDLINE_MAX];
};

void print_prompt(){
        printf("sshell$ ");
        fflush(stdout);
}

// split the command without files and pipes into tokens,
// and put them in the Command struct.
struct Error basic_command_split(struct Command *command, char *buffer) {
        int counter = 0;
        char *arg;
        struct Error error;

        command->cmd = strtok(buffer, " ");
        command->args[counter] = command->cmd;
        counter++;
        
        // reference:
        // https://stackoverflow.com/questions/23456374/why-do-we-use-null-in-strtok
        arg = strtok(NULL, " ");
        while(arg) {
                // Test if the argument are too much
                if (counter >= MAX_ARGUMENT){
                        error.flag = 1;
                        strcpy(error.msg, "Error: too many process arguments\n");
                        break;
                }
                command->args[counter] = arg;
                counter++;
                arg = strtok(NULL, " ");
        }
        if (error.flag != 1)
                command->args[counter] = NULL;
        return error;
}

// Will be used when a '<' or '>' is detected in the command
// Parse the command and put them into the Command structure
struct Error extract_filename(struct Command *command, char *buffer) {
        char *basic_command;
        char *filename;
        struct Error error;

        if(strchr(buffer, '>')){
                if (*buffer == '>'){
                        error.flag = 1;
                        strcpy(error.msg, "Error: missing command\n");
                        return error;
                }
                basic_command = strtok(buffer, ">"); 
                filename = strtok(NULL, ">");
                filename = strtok(filename, " "); // make sure no space in filename
                if (filename == NULL) {
                        error.flag = 1;
                        strcpy(error.msg, "no output file\n");
                        return error;
                }
                command->filename = filename;
                error = basic_command_split(command, basic_command);
                if (command->cmd == NULL){
                        error.flag = 1;
                        strcpy(error.msg, "Error: missing command\n");
                        return error;
                }
        } else if (strchr(buffer, '<')) {
                basic_command = strtok(buffer, "<");
                if (*buffer == '<'){
                        error.flag = 1;
                        strcpy(error.msg, "Error: missing command\n");
                        return error;
                }
                command->filename = strtok(NULL, "<");
                filename = strtok(command->filename, " ");
                if (filename == NULL) {
                        error.flag = 1;
                        strcpy(error.msg, "no input file\n");
                        return error;
                }
                error = basic_command_split(command, basic_command);
                if (command->cmd == NULL){
                        error.flag = 1;
                        strcpy(error.msg, "Error: missing command\n");
                        return error;
                }
        } 
        return error;
}


// Will be used if there is a pipe line
struct Error pipe_split(struct Pipelines *pipeline, char *buffer, int *counter) {
        char* nl;
        char *basic_command[MAX_PIPES];
        struct Error error;

        nl = strchr(buffer, '\0');

        if ((*buffer == '|') || (*(nl-1) == '|')){
                error.flag = 1;
                strcpy(error.msg, "Error: missing command\n");
                return error;
        }
        
        basic_command[0] = strtok(buffer, "|");
        for(*counter = 1; *counter < MAX_PIPES; (*counter)++){
                basic_command[*counter] = strtok(NULL, "|");
        }
        
        for(*counter = 0; *counter < MAX_PIPES; (*counter)++){
                if (!basic_command[*counter]){
                        break;
                } 
        }

        for(int i = 0; i < *counter; i++){
                if ((i != (*counter - 1)) && (strchr(basic_command[i], '<') || strchr(basic_command[i], '>'))) {
                        error.flag = 1;
                        strcpy(error.msg, "Error: mislocated output redirection\n");
                        return error;
                }
                pipeline->cmds[i] = malloc(sizeof(struct Command));
                if (i == (*counter - 1) && strchr(basic_command[i], '>')){
                        error = extract_filename(pipeline->cmds[i], basic_command[i]);
                } else {
                        error = basic_command_split(pipeline->cmds[i], basic_command[i]);
                }
        }
        return error;
}


struct Error run_command(struct Command *command, char *buffer){
        int pid;
        int fd;
        int status;
        struct Error error;
        if (strchr(buffer, '<')) {
                fd = open(command->filename, O_RDONLY);
                if (fd == -1){
                        error.flag = 1;
                        strcpy(error.msg, "Error: cannot open input file\n");
                        return error;
                }
        }
        if (strchr(buffer, '>')) {
                fd = open(command->filename, O_WRONLY);
                if (fd == -1){
                        error.flag = 1;
                        strcpy(error.msg, "Error: cannot open output file\n");
                        return error;
                }
        }

        pid = fork();
        if (pid == 0) {
                // child
                if (strchr(buffer, '<')) {
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                }
                if (strchr(buffer, '>')) {
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                }
                
                execvp(command->cmd,command->args);
                //perror("execv");
                exit(1);
        } else if (pid > 0){
                // parent
                wait(&status);
                if(status != 0)
                        fprintf(stderr, "Error: command not found\n");
                fprintf(stderr, "+ completed \'%s\' [%d]\n", buffer, WEXITSTATUS(status));
        } else {
                perror("fork");
                exit(1);
        }
        return error;
}

struct Node *reverse_list(struct Node *head) {
        struct Node *front = NULL;
        struct Node *back = NULL;
        struct Node *cur = head;
        while(cur) {
                back = cur->next;
                cur->next = front;
                front = cur;
                cur = back;
        }
        return front;
}
void print_dirs(struct Node *head) {
        struct Node* start = reverse_list(head);
        if(head == NULL) {
                fprintf(stdout, "\n");
                return;
        }
        while(start != NULL) {
                fprintf(stdout, "%s\n", start->curstack);
                start = start->next;
        }
}
struct Node *pushd(struct Node *newNode, struct Node *head) {
        struct Node *pushNode = head;
        if (pushNode == NULL) {
                return newNode;
        }
        else {
                pushNode->next = pushd(newNode, (pushNode->next));
                return pushNode;
        }
}
struct Node *popd(struct Node *head) {
        if (head == NULL) {
                return head;
        }
        else if(head->next == NULL) { //found the last node on stack
                return NULL;
        }
        else {
                head->next = popd(head->next);
                return head;
        }
}
void changeDir(struct Command command, char cwd[]) {
        char curdirectory[CMDLINE_MAX];
        if (command.args[1] == NULL) {
                sprintf(curdirectory, "%s", getenv("HOME"));
                chdir(curdirectory);
        }
        else if (command.args[2] != NULL) {
                fprintf(stderr, "err: Failed to cd, incorrect number of arguments.\n");
        }
        else if (!strcmp(command.args[1], "..")) {
                chdir("..");
        }
        else if (!strcmp(command.args[1], "~")) {
                sprintf(curdirectory, "%s", getenv("HOME"));
                chdir(curdirectory);
        }
        else if (!strcmp(command.args[1], "/")) {
                sprintf(curdirectory, "%s", getenv("ROOT"));
                chdir(curdirectory);
        }
        else {
                //getcwd(cwd, sizeof(cwd));
                strcat(cwd, "/");
                strcat(cwd, command.args[1]);
                if (chdir(command.args[1]) == FAIL_CD) {
                        fprintf(stderr, "Directory not found: %s\n", command.args[1]);
                        return;
                }
                else {
                        chdir(cwd);
                }
        }
}

int main(void)
{
        char buffer[CMDLINE_MAX];
        char buffercpy[CMDLINE_MAX];
        struct Node *head_out = (struct Node*)malloc(sizeof(struct Node)); //head of directory stack
        /*char cwd_out[MAX_TOKEN_LEN];
        getcwd(cwd_out, sizeof(cwd_out));
        strcpy(head->curstack, cwd_out);
        head-> next = NULL;*/
        static bool once = true;


        while (1) {
                char *nl;  // Is used in replacing \n with \0
                char *fileinindex;  // Is used to store the pointer of '<' in command
                char *fileoutindex; // Is used to store the pointer of '>' in command
                // int retval;
                char cwd[MAX_TOKEN_LEN]; // Is used to store the directory address
                pid_t pid; // Is used to store the child's PID
                struct Command command; // Command variable when no pipe
                int status; // Store child's return value
                int pipeCounter = 0;
                struct Error error;
                
                //struct Node *head = (struct Node*)malloc(sizeof(struct Node)); //head of directory stack
                if (once) {
                        once = false;
                        getcwd(cwd, sizeof(cwd));
                        strcpy(head_out->curstack, cwd);
                        head_out-> next = NULL;
                }

                /* Print prompt */
                print_prompt();

                /* Get command line */
                fgets(buffer, CMDLINE_MAX, stdin);
                strcpy(buffercpy, buffer);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", buffer);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(buffer, '\n');
                if (nl)
                        *nl = '\0';
                
                nl = strchr(buffercpy, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(buffer, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed 'exit' [0]\n");
                        break;
                }
                // pwd
                if (!strcmp(buffer, "pwd")) {
                        if(getcwd(cwd, sizeof(cwd)) == NULL) {
                                fprintf(stderr, "Fail to get the current working file\n");
                                continue;
                        } else {
                                fprintf(stdout, "%s\n", cwd);
                                continue;
                        }
                }
                //cd
                basic_command_split(&command, buffercpy);
                if (!strcmp(command.args[0], "cd")) {
                        getcwd(cwd, sizeof(cwd));
                        changeDir(command, cwd);
                        continue;
                }
                strcpy(buffercpy, buffer);

                //directory stack
                if (!strcmp(command.args[0], "pushd")) {
                        if (chdir(command.args[1]) == FAIL_CD) {
                                fprintf(stderr, "Directory not found: %s\n", command.args[1]);
                                continue;
                        }
                        strcpy(cwd, head_out->curstack);
                        strcat(cwd, "/");
                        strcat(cwd, command.args[1]);
                        struct Node *newNode = (struct Node*)malloc(sizeof(struct Node));
                        strcpy(newNode->curstack, cwd);
                        newNode->next = NULL;
                        head_out = pushd(newNode, head_out);
                        continue;
                }
                if (!strcmp(command.args[0], "popd")) {
                        if (head_out->next == NULL) {
                                fprintf(stdout, "Unable to pop\n");
                        }
                        else {
                                head_out = popd(head_out);
                                chdir("..");
                        }
                        continue;
                }
                if (!strcmp(command.args[0], "dirs")) {
                        print_dirs(head_out);
                        continue;
                }

                // not built in
                if(!strchr(buffer, '|')) {
                        // normal case, no pipe

                        // test if there is a file that need.
                        fileinindex = strchr(buffer, '<');
                        fileoutindex = strchr(buffer, '>');
                        
                        if (!(fileinindex||fileoutindex)){
                                // the command has no file;
                                //printf("n %s\n", buffer);
                                error = basic_command_split(&command, buffercpy);
                                //printf("%s\n", command.cmd);
                                //printf("%s\n", command.args[0]);
                                
                        } else {
                                // the command contains a file;
                                error = extract_filename(&command, buffercpy);
                                //printf("f %s\n", buffer);
                        }
                        // run the command
                        if (error.flag == 1){
                                fprintf(stderr, "%s", error.msg);
                                continue;
                        } else {
                                error = run_command(&command, buffer);
                                if (error.flag == 1) {
                                        fprintf(stderr, "%s", error.msg);
                                        continue;
                                }
                        }
                } else { // the command contains pipe
                        struct Pipelines pipeline;
                        error = pipe_split(&pipeline, buffercpy, &pipeCounter);
                        
                        if (error.flag == 1){
                                fprintf(stderr, "%s", error.msg);
                                continue;
                        }

                        

                        // for(int i = 0; i < pipeCounter; i++){
                        //         printf("%s\n", pipeline.cmds[i]->cmd);
                        // }
                        // generate pipes
                        for(int i = 0; i < (pipeCounter - 1); i++){
                            pipeline.pipes[i] = malloc(2*sizeof(int));
                            //perror(malloc);
                            pipe(pipeline.pipes[i]);
                        }
//                             for (int i = 0; i < pipeCounter; i++){
//         printf("%d %d\n", pipeline.pipes[i][0], pipeline.pipes[i][1]);
//     }
                        
                        pid = fork();
                        if (pid == 0) {
                                // child

                                for(int i = 0; i < pipeCounter; i++){
                                        if(fork() != 0){ //child - parent
                                                // parent does nothing.
                                                
                                                wait(&status);
                                                 printf("%d p\n", i);
                                                 fflush(stdout);
                                                
                                                if(status != 0){
                                                        // if processes generator didn't return normally
                                                        // return an error
                                                        //printf("%d error", status);
                                                        break;
                                                }
                                        } else { // child - child
                                                // child keeps generating processes
                                                if(i == 0) { //first command : cmds[0]
                                                        dup2(pipeline.pipes[i][1], STDOUT_FILENO);
                                                        close(pipeline.pipes[i][1]);

                                                } else if (i == (pipeCounter - 1)){ // last command
                                                        dup2(pipeline.pipes[i-1][0], STDIN_FILENO);
                                                        close(pipeline.pipes[i-1][0]);

                                                } else { // commands in the middle
                                                        dup2(pipeline.pipes[i-1][0], STDIN_FILENO);
                                                        dup2(pipeline.pipes[i][1], STDOUT_FILENO);
                                                        close(pipeline.pipes[i][1]);
                                                        close(pipeline.pipes[i-1][0]);
                                                }
                                                printf("%d p\n", i);
                                                        fflush(stdout);
                                                execvp(pipeline.cmds[i]->cmd,pipeline.cmds[i]->args);
                                                perror("execv");
                                                exit(1);
                                        }
                                }
                                if(status != 0)
                                        exit(1);
                                exit(0);
                                
                                
                        } else if (pid > 0){
                                // parent
                                wait(&status);
                                fprintf(stderr, "+ completed \'%s\' [%d]\n", buffer, WEXITSTATUS(status));
                        } else {
                                perror("fork");
                                exit(1);
                        }
                

                }
        }

        return EXIT_SUCCESS;
}
