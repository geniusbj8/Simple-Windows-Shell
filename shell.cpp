#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

#define HISTORY_SIZE 10

// Global variables for command history
char history[HISTORY_SIZE][256];
int history_count = 0;

// Function declarations
void execute_command(char *command);
void execute_pipe(char *command1, char *command2);
void add_to_history(char *command);
void print_history();
void expand_environment_variables(char *command);
BOOL ctrl_handler(DWORD fdwCtrlType);

// Main function: Shell loop
int main() {
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrl_handler, TRUE);  // Set up signal handler

    char command[256];  // Buffer for storing user input
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        fgets(command, sizeof(command), stdin);

        // Remove the newline character from the input
        command[strcspn(command, "\n")] = 0;

        // Exit the shell if the user types "exit"
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Handle command history
        if (strcmp(command, "history") == 0) {
            print_history();
            continue;
        }

        // Expand any environment variables in the command
        expand_environment_variables(command);

        // Add command to history
        add_to_history(command);

        // Execute the command
        execute_command(command);
    }

    return 0;
}

// Function to execute a command with optional piping, redirection, and background processing
void execute_command(char *command) {
    int background = 0;
    HANDLE hInput = NULL, hOutput = NULL;

    // Check if the command should be run in the background
    char *background_pos = strstr(command, "&");
    if (background_pos != NULL) {
        *background_pos = 0;  // Remove the '&' from the command
        background = 1;
    }

    // Check if there is a pipe in the command
    char *pipe_pos = strstr(command, "|");
    if (pipe_pos != NULL) {
        *pipe_pos = 0; // Split the command at the pipe
        char *command1 = command;
        char *command2 = pipe_pos + 1;
        execute_pipe(command1, command2);
        return;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // Initialize the startup info structure
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;  // Use custom standard handles for redirection

    // Check for input redirection ('<')
    char *input_redirect = strstr(command, "<");
    if (input_redirect != NULL) {
        *input_redirect = 0;  // Split command from redirection part
        char *input_file = strtok(input_redirect + 1, " ");
        hInput = CreateFile(input_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hInput == INVALID_HANDLE_VALUE) {
            printf("Cannot open input file: %s\n", input_file);
            return;
        }
        si.hStdInput = hInput;
    }

    // Check for output redirection ('>')
    char *output_redirect = strstr(command, ">");
    if (output_redirect != NULL) {
        *output_redirect = 0;  // Split command from redirection part
        char *output_file = strtok(output_redirect + 1, " ");
        hOutput = CreateFile(output_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOutput == INVALID_HANDLE_VALUE) {
            printf("Cannot open output file: %s\n", output_file);
            return;
        }
        si.hStdOutput = hOutput;
    }

    // Prepare the command by appending it to cmd.exe
    char cmd_with_prefix[512];
    snprintf(cmd_with_prefix, sizeof(cmd_with_prefix), "cmd.exe /C %s", command);

    // Create the process to execute the command via cmd.exe
    if (!CreateProcess(NULL, cmd_with_prefix, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        printf("CreateProcess failed (%d)\n", GetLastError());
        return;
    }

    // If not in background, wait for the process to finish
    if (!background) {
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    // Close handles
    if (hInput != NULL) CloseHandle(hInput);
    if (hOutput != NULL) CloseHandle(hOutput);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// Function to execute two commands connected by a pipe
void execute_pipe(char *command1, char *command2) {
    STARTUPINFO si1, si2;
    PROCESS_INFORMATION pi1, pi2;
    HANDLE hRead, hWrite;

    // Create the pipe
    if (!CreatePipe(&hRead, &hWrite, NULL, 0)) {
        printf("Pipe creation failed.\n");
        return;
    }

    // Initialize the STARTUPINFO structures for both processes
    ZeroMemory(&si1, sizeof(si1));
    si1.cb = sizeof(si1);
    si1.hStdOutput = hWrite;  // Set output of first process to pipe
    si1.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&si2, sizeof(si2));
    si2.cb = sizeof(si2);
    si2.hStdInput = hRead;  // Set input of second process to pipe
    si2.dwFlags |= STARTF_USESTDHANDLES;

    // Create first process (command1)
    if (!CreateProcess(NULL, command1, NULL, NULL, TRUE, 0, NULL, NULL, &si1, &pi1)) {
        printf("Failed to create process for command 1.\n");
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return;
    }

    // Create second process (command2)
    if (!CreateProcess(NULL, command2, NULL, NULL, TRUE, 0, NULL, NULL, &si2, &pi2)) {
        printf("Failed to create process for command 2.\n");
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return;
    }

    // Close unnecessary handles
    CloseHandle(hWrite);  // The second process will read from the pipe
    CloseHandle(hRead);   // The first process writes to the pipe

    // Wait for both processes to finish
    WaitForSingleObject(pi1.hProcess, INFINITE);
    WaitForSingleObject(pi2.hProcess, INFINITE);

    // Close process and thread handles
    CloseHandle(pi1.hProcess);
    CloseHandle(pi1.hThread);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);
}

// Function to add a command to the history
void add_to_history(char *command) {
    if (history_count < HISTORY_SIZE) {
        strcpy(history[history_count++], command);
    } else {
        // Shift all commands up to make room for the new one
        for (int i = 1; i < HISTORY_SIZE; i++) {
            strcpy(history[i - 1], history[i]);
        }
        strcpy(history[HISTORY_SIZE - 1], command);
    }
}

// Function to print the command history
void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

// Function to expand environment variables in a command
void expand_environment_variables(char *command) {
    char buffer[256];
    ExpandEnvironmentStrings(command, buffer, sizeof(buffer));
    strcpy(command, buffer);
}

// Signal handler for CTRL+C
BOOL ctrl_handler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
            printf("\nCTRL+C detected. Use 'exit' to close the shell.\n");
            return TRUE;
        default:
            return FALSE;
    }
}

