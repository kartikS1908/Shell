#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h>

// parse: a simple function to parse a line of text.
// Parameters:
// - char *line: a string containing characters to be parsed
// - int *n: this will contain the number of tokens parsed when the
//           function returns.
// - char *delim: this contains the characters that will be used as
//           the delimiters.
// Return value:
// - A pointer to an array of strings containing the tokens parsed,
//   with the last element being the NULL pointer.
// - Or, NULL if the parsing failed for some reasons.
//
// Note: this function allocates memory dynamicall using malloc/realloc.
//       Make sure you free them if they are no longer in use, using the free_token
//       function (see below).

const int MAX_PATH_COUNT = 1024;
const int MAX_INPUT_SIZE = 1024;
const int MAX_BUFF_SIZE = 1024;

// FIx implicit declaration warning for circular function.
void parse_command(char* input, char** path);
/*
Parser function. Source myshell.c from COMP3300 lab-03. 
*/
char **parse(char *line, int *n, char *delim)
{
    int sz = 32;
    char **a = malloc(sz * sizeof(char *));
    assert(a != NULL);
    memset(a, 0, sz * sizeof(char *));

    int i = 0;
    *n = 0;
    char *token;

    while (line)
    {
        token = strsep(&line, delim);
        if (token == NULL)
            break;
        if (token[0] == '\0')
            continue;
        if (i >= sz - 1)
        {
            ++sz;
            a = realloc(a, sz * sizeof(char *));
        }
        a[i] = strdup(token);
        ++i;
    }
    a[i] = NULL;
    *n = i;
    return a;
}

// Free the tokens allocated in the arraow tok[]. Source myshell.c from COMP3300 lab-03.
void free_tokens(char *tok[], int n)
{
    for (int i = 0; i < n; ++i)
    {
        if (tok[i])
            free(tok[i]);
        tok[i] = NULL;
    }
}

int _DEBUG = 0;
void ERROR(int errnum, const char *format, ...)
{
    if (_DEBUG)
    {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        if (errnum > 0)
            fprintf(stderr, " : %s", strerror(errnum));
        fprintf(stderr, "\n");
        return;
    }

    fprintf(stderr, "An error has occurred\n");
}


/*
All commands are executed here. (Other than internal commands)
*/
void process_cmd(char **tokens, int *n, char *path[], char *filename)
{
    int pid = fork();
    if (pid < 0)
    {
        ERROR(1, "An error has occurred\n");
        exit(1);
    }
    if (pid == 0)
    {   
        // Output redirection!! 
        if (filename != NULL)
        {
            int fp = 0;
            // Deal with whitespace and new line character at the end of file name. 
            while (filename[fp] != '\n' && filename[fp] != ' ')
                fp++;
            filename[fp] = '\0';

            // Create file and redirect the STDOUT to the file.
            int fd;
            if ((fd = creat(filename, 0644)) < 0)
            {
                ERROR(0, "An error has occered\n");
                exit(0);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // Deal with whitespace and new line character at the end of command args.
        int i = 0;
        while (tokens[(*n) - 1][i] != '\n' && tokens[(*n) - 1][i] != '\0')
            i++;
        tokens[(*n) - 1][i] = '\0';

        int j = 0;
        // Executable found in the current working directory. 
        if (!access(tokens[0], X_OK))
        {
            execv(tokens[0], tokens);
            exit(0);
        }
        else
        {
            // Go through the path variables. See if the executable exists.  
            while (path[j] != NULL)
            {
                // Construct the command to be executed. 
                char *command = malloc(sizeof(command));
                command = strcat(command, path[j]);
                command = strcat(command, "/");
                command = strcat(command, tokens[0]);
                // Executable found!!
                if (!access(command, X_OK))
                {
                    char *myargs[MAX_INPUT_SIZE];
                    i = 1;
                    // Deal with whitespaces. 
                    int count_white = 0;
                    myargs[0] = command;
                    while (tokens[i] != NULL)
                    {
                        if (!strcmp(tokens[i], ""))
                        {
                            count_white++;
                            i++;
                            continue;
                        }
                        myargs[i - count_white] = tokens[i];
                        i++;
                    }
                    // Ensure Null termination.
                    myargs[i - count_white] = NULL;
                    execv(myargs[0], myargs);
                    exit(0);
                }
                j++;
            }
        }
        // Command not found.
        ERROR(0, "An error has occured\n");
        exit(0);
    }
    else
    {   
        // wait for all children.
        while (wait(NULL) > 0)
            ;
    }
}

// Intrenal commands. 
void process_command_self(char **tokens, char *path[])
{
    // Exit gracefully.
    if (!strcmp("exit\n", tokens[0]))
        exit(0);
    // Incorrect use of exit.
    if (!strcmp("exit", tokens[0]) && (tokens[1] != NULL && strcmp("\n", tokens[1])))
    {
        ERROR(0, "An error has occured\n");
    }
    // cd with no args.
    else if (!strcmp("cd\n", tokens[0]))
    {
        ERROR(0, "An error has occurred\n");
    }
    // correct use of cd. Including dealing with whitespaces. 
    else if (!strcmp("cd", tokens[0]))

    {
        if(tokens[2]) {
            ERROR(0, "An error has occurred\n");
            return; 
        }
        int c = 0;
        while (tokens[1][c] != '\n')
        {
            c++;
        }
        tokens[1][c] = '\0';
        if (chdir(tokens[1]) == -1)
        {
            ERROR(0, "An error has occurred\n");
            return;
        }
    }
    // Set path to null.
    else if (!strcmp(tokens[0], "path\n"))
    {
        int i = 0;
        while (path[i] != NULL)
        {
            path[i] = NULL;
            i++;
        }
    }
    // Path to be set. Overwrites the current path. 
    else if (!strcmp(tokens[0], "path"))
    {
        int i = 0;
        while (path[i] != NULL)
        {
            path[i] = NULL;
            i++;
        }
        i = 1;
        while (tokens[i] != NULL)
        {
            path[i - 1] = tokens[i];
            i++;
        }
        int j = 0;
        while (path[i - 2][j] != '\n')
        {
            j++;
        }
        path[i - 2][j] = '\0';
    }
    return;
}
// Deal with pipes. 
void parse_pipes(char *input, char **path)
{
    // Save the initial file descriptors. 
    int f_in = dup(STDIN_FILENO);
    int f_out = dup(STDOUT_FILENO);
    // Tokenize the various commands to be piped. 
    char *del = "|";
    int n_pipes;
    char **tok_pipes = parse(input, &n_pipes, del);
    int i;
    // Fork processes for each pipe. remember the previous out. redirect in of next towards it. 
    for (i = 0; i < n_pipes - 1; i++)
    {
        // Pipe array.
        int pd[2];
        pipe(pd);
        int pid;
        if ((pid = fork()) == 0)
        {

            dup2(pd[1], STDOUT_FILENO);
            parse_command(tok_pipes[i], path);
            exit(0);
        }
        else
        {
            dup2(pd[0], STDIN_FILENO);
            close(pd[1]);
        }
    }
    // Last pipe. 
    parse_command(tok_pipes[n_pipes - 1], path);
    // Restore everything. Otherwise you will get a segfault the next time you try to get input. 
    dup2(f_in, STDIN_FILENO);
    dup2(f_out, STDOUT_FILENO);
    return;
}

// Function to carry out the main input parsing. CHeck for internal commands, piping, redirection and parallel commands. 
void parse_command(char *input, char **path)
{
    int n;
    char *del = " ";
    // Keep copies of the input. Multiple tokenizations necessary for checking for pipes and redirection. Parse function modifies the input string.  
    char input_copy[MAX_BUFF_SIZE];
    char input_copy_pipes[MAX_BUFF_SIZE];
    strcpy(input_copy, input);
    strcpy(input_copy_pipes, input);
    char **tokens = parse(input, &n, del);
    // Check for internal commands. 
    if (!strcmp(tokens[0], "exit\n") || !strcmp(tokens[0], "cd") || !strcmp(tokens[0], "cd\n") ||
        !strcmp(tokens[0], "path") || !strcmp(tokens[0], "path\n") || !strcmp(tokens[0], "exit"))
        process_command_self(tokens, path);
    // Not an internal command. 
    else
    {
        // check for redirection. 
        if (strstr(input_copy, ">"))
        {
            int n_redirect;
            char **tokens_redirect = parse(input_copy, &n_redirect, ">");
            // All invalid cases of redirection. 
            if (n_redirect != 2 || !strcmp(tokens_redirect[1], "\n") || strstr(tokens_redirect[1], ">"))
            {
                ERROR(0, "An error has occured\n");
                return;
            }
            // redirection valid. 
            else
            {
                int file_count = 0;
                char **file_check = parse(tokens_redirect[1], &file_count, " ");
                if (file_count == 2)
                {
                    if (!strcmp(file_check[1], "\n"))
                    {
                        file_count--;
                    }
                }
                // Redirection to more than 1 file(or zero files) error. 
                if (file_count != 1)
                {
                    ERROR(0, "An error has occured\n");
                    return;
                }
                n_redirect = 0;
                //  tokenize the command arguments (excluding the file.)
                char **toks = parse(tokens_redirect[0], &n_redirect, " ");
                // Check for piping. 
                if (strstr(input_copy_pipes, "|") == NULL)
                {
                    // No pipes. Call the executor routine.
                    process_cmd(toks, &n_redirect, path, file_check[0]);
                }
                // pipes found. 
                else
                {
                    parse_pipes(input_copy_pipes, path);
                }
                free_tokens(toks, n_redirect);
                return;
            }
        }
        // No redirection found. 
        // Check for pipes.
        if (strstr(input_copy_pipes, "|") == NULL)
            process_cmd(tokens, &n, path, NULL);
        else
        {
            // No pipes found. 
            parse_pipes(input_copy_pipes, path);
        }
        free_tokens(tokens, n);
    }
}
// Deal with parallel commands. Some weird behaviour noticed. Outlined in report. 
void parse_command_parallel(char *input, char *path[])
{
    //  Parallel commands found. 
    if (strstr(input, "&") != NULL)
    {
        int n;
        char *del = "&";
        char **tokens = parse(input, &n, del);
        // Spawn a new process for each of the parallel commands. Wait for all of them in the parent. 
        for (int i = 0; i < n; i++)
        {
            if (fork() == 0)
            {
                parse_command(tokens[i], path);
                exit(0);
            }
        }
        while (wait(NULL) > 0)
            ;
        return;
    }
    // No parallel commands found. 
    else
    {
        parse_command(input, path);
    }
}



int main(int argc, char *argv[])
{
    char *input = NULL;
    // Path is implemented as an array of strings. Max number of path variables = 1024. 
    char *path[MAX_PATH_COUNT];
    // Init path at bin. 
    path[0] = "/bin";

    size_t n;
    // INTERACTIVE!!
    if (argc == 1)
        while (1)
        {
            // print the current working directory. 
            char cwd[1024];
            printf("%s", getcwd(cwd, sizeof(cwd)));
            printf("\n");
            // prompt. 
            printf("anubis> ");
            // get input. 
            if (!getline(&input, &n, stdin))
                break;
            // Send to upper most layer of abstraction. More details in the report about how different levels interleave. 
            parse_command_parallel(input, path);
            // set input back to NULL.
            input = NULL;
            // Break at EOF.
            if (feof(stdin))
                break;
        }
    // Batch mode. 
    else if (argc == 2)
    {
        // open file.  (in read mode).
        FILE *fd = fopen(argv[1], "r");
        if (fd == NULL)
        {
            ERROR(1, "An error has occured\n");
            exit(1);
        }
        while (1)
        {
            if (getline(&input, &n, fd) == -1)
                exit(0);
            parse_command_parallel(input, path);
            input[0] = '\0';
            // break at end of file.
            if (feof(stdin))
                break;
        }
    }
    // anubis given more than 1 argument. 
    else
    {
        ERROR(1, "An error has occured\n");
        exit(1);
    }

    return 0;
}
