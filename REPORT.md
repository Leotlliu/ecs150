# Instruction
The command can be split into 3 situations. First, the basic situation, where 
commands are only split by spaces. Second, the command contains a file. And the 
command becomes basic part plus the file part. Third, the command contains pipe.
Then, the commands become a sequence of basic command. 

Pipeline: We created a child from the shell. And this child served as a process
generator. The child will create many child again that create the process. And
finally it returns to the main shell function.

## struct Command
The command struct is used to divide the recieved command line.
* cmd of the Command struct is are the commands to be operated such as (echo, cd
, ..)
* args[MAX_ARGUMET] of the Command struct will store the entire line of commands
 from user input, including the command itself(same as cmd) and the arguments. 
 (args[0] will store the same value as cmd).
* filename
## struct Node
The node struct is used for the linked lists when implementing the Directory 
stack.
* curstack[CMMDLINE_MAX] will store the value of the node, each of the values is
 the entire path to the current directory (ex. /Home/Desktop/ECS150)
* *next is the pointer to the element next on the linked list
## basic_command_split(error)
This function split the basic command line, which is read into *buffer. 
then convert that into a Command struct. 
## extract_filename (error)
This function splits the command line when there is a file. And this function
uses basic_command_split.
## pipe_split(error)
This function splits the command line when there are pipes. And this function
uses basic_command_split and extract filename
## run_command (error)
This function is the basic funtion to run the command.
## print_dirs(void)
This function will be called when a dirs command is detected and print out all 
nodes within the Directory stack.
## pushd(Node)
This function receives the head and a new Node which wish to be added to the 
existing linked-list stack, it returns a new linked-list with the added new 
node.
## popd(Node)
This function receives the current head of the linked-list, iterating through 
it and removing(pop) the last element of the linked-list
## changeDir(void)
This function is called when a cd command is in need, arguments are the Command 
object and current working directory(cwd)
* If no other arguments are passed or a '~' is parsed, the directory is set to 
the HOME directory.
* If a '/' is parsed, the directory is changed to the ROOT directory
* if a '..' is parsed, the directory returns to the previous
* otherwise, the cwd is reformatted with added '/' and name of new directory, 
and passed to chdir
# Main
* **head_out** is where the linked-list for the directory stack is stored
* **once**, along with an(if statement) inside the **while** loop is to 
initialize the value of **head_out** on only the first run, setting it to 
the cwd
### Build-in functions
#### !strcmp(command.args[0], 'pwd')
* check if the cwd is valid, if so, print it.
#### !strcmp(command.args[0], 'cd')
* This part of the main function gets the cwd and calls the **changeDir** 
function above
#### !strcmp(command.args[0], 'pushd')
* creates a new Node and pass it to the pushd function along with the head
#### !strcmp(command.args[0], 'popd')
* calls the popd function
#### !strchr(buffer, '|')) 
* Standard input redirection(without piping)
#### pid
* When pid == 0 it is the child process, when pid > 0 it is the parent process