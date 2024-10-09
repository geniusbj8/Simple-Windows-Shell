# Custom Shell for Windows

This project implements a simple command-line shell for Windows that supports command execution, piping, redirection, environment variable expansion, command history, and background execution.

## Features

- **Command Execution**: Execute commands using the `cmd.exe` shell.
- **Piping (`|`)**: Redirect the output of one command as the input to another.
- **Input and Output Redirection (`<`, `>`)**: Redirect input and output to files.
- **Background Execution (`&`)**: Run commands in the background.
- **Command History**: View your previous commands with the `history` command.
- **Environment Variables Expansion**: Automatically expands environment variables like `%PATH%`.
- **Signal Handling**: Gracefully handle `CTRL+C` without exiting the shell immediately.

## Getting Started

### Prerequisites

- A Windows system with a C compiler (e.g., MinGW, Visual Studio).
- Basic knowledge of Windows command-line commands.

### Compilation

To compile the shell, you can use any standard C compiler. For example, using GCC:

```bash
gcc -o myshell myshell.c

### Running the Shell

Running the Shell

### Basic Commands

myshell> dir
myshell> history
myshell> echo %PATH%
myshell> dir | findstr .exe
myshell> type < input.txt > output.txt
myshell> exit

