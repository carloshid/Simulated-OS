# Simulated-OS

Simple simulated operating system created as part of an operating systems course. Includes multi-process scheduling and memory management with demand paging.

To run the program and enter commands directly:
1. Go to the src directory
2. make mysh framesize=X varmemsize=Y
3. ./mysh

To run the program and use a set of commands stored in a file:
1. Go to the src directory
2. make mysh framesize=X varmemsize=Y
3. ./mysh < FILE

X and Y represent the size of the frame store and the variable store in memory respectively. FILE is the name of the file to run.

### Supported commands
| Command             | Description                                                          |
| ------------------- | -------------------------------------------------------------------- |
| help                | Displays all the commands                                            |
| quit                | Exits the program                                                    |
| set VAR VALUE       | Creates or updates VAR in memory to VALUE                            |
| print VAR           | Displays the value of VAR if it exists                               |
| run FILE            | Excutes the file FILE                                                |
| echo STRING         | Displays STRING                                                      |
| echo $VAR           | Displays the value of VAR if it exists                               |
| my_ls               | Lists all the files in the current directory                         |
| my_mkdir DIR        | Creates a directory named DIR                                        |
| my_touch FILE       | Creates a file named FILE                                            |
| my_cd DIR           | Changes the current directory to DIR                                 |
| exec P1 [P2 P3] POL | Executes up to 3 files P1, P2, P3 according to scheduling policy POL |
