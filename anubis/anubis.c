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
char ** parse(char *line, int *n, char * delim)
{
    int sz=32;
    char ** a = malloc(sz * sizeof(char *)); 
    assert(a != NULL); 
    memset(a, 0, sz*sizeof(char*));

    int i=0; 
    *n = 0; 
    char *token; 

    while(line)
    {
        token = strsep(&line, delim); 
        if(token == NULL) break;
        if(token[0] == '\0') continue;
        if(i >= sz-1) {
            ++sz; 
            a = realloc(a, sz*sizeof(char*));
        }
        a[i] = strdup(token);
        ++i;
    }
    a[i] = NULL;
    *n = i; 
    return a; 
}

// Free the tokens allocated in the arraow tok[]
void free_tokens(char *tok[], int n)
{
    for(int i=0; i < n; ++i)
    {
        if(tok[i]) free(tok[i]);
        tok[i] = NULL;
    }
}

// Use this _DEBUG variable to control the error messages printed
// by the ERROR function.
//  _DEBUG=0: print only one error message; 
//  _DEBUG=1: print custom messages.
// Make sure you set _DEBUG to 0 for the submitted version. 

int _DEBUG = 0;

void ERROR(int errnum, const char * format, ... )
{
    if(_DEBUG) {
        va_list args;
        va_start (args, format);
        vfprintf(stderr, format, args);
        va_end (args);
        if(errnum > 0) fprintf(stderr, " : %s", strerror(errnum));
        fprintf(stderr, "\n"); 
        return; 
    }

    fprintf(stderr, "An error has occurred\n"); 
}

// TODO: implement your shell!

void parse_command_parallel(char* input, char* path[])
{
    if(strstr(input,"&")!=NULL)
    {
        int n,status,pid;
        char *del = "&";
        char** tokens = parse(input,&n,del);
        // printf("%i",n);

        for(int i = 0; i<n; i++)
        {
        if(fork() == 0)     
        {
            parse_command(tokens[i],path);   
            exit(0);
        }
        }
    while (wait(NULL) > 0);
        return;
    }  
    else 
    {
        parse_command(input,path);
    }
}

void parse_command(char *input,char* path[])
{
    int n;
    char* del = " ";
    char* input_copy[MAX_BUFF_SIZE];
    char* input_copy_pipes[MAX_BUFF_SIZE];
    strcpy(input_copy,input);
    strcpy(input_copy_pipes,input);
    char** tokens = parse(input,&n,del);
    if(!strcmp(tokens[0],"exit\n") || !strcmp(tokens[0],"cd") || !strcmp(tokens[0],"cd\n") || 
        !strcmp(tokens[0],"path")|| !strcmp(tokens[0],"path\n") || !strcmp(tokens[0],"exit")) 
        process_command_self(tokens,path);
    else {
        if(strstr(input_copy,">"))
        {
            int n_redirect;
            char** tokens_redirect = parse(input_copy,&n_redirect,">");
            if(n_redirect!=2 || !strcmp(tokens_redirect[1],"\n") || strstr(tokens_redirect[1],">"))
            {
                ERROR(0,"An error has occured\n");
                return;
            }

            else{
                int file_count = 0;
                char** file_check = parse(tokens_redirect[1],&file_count," ");
                if(file_count!=1)
                {
                    ERROR(0,"An error has occured\n");
                    return;
                }
                n_redirect = 0;
                char** toks = parse(tokens_redirect[0],&n_redirect," ");
                // parse_cmd_pipes(toks,&n_redirect,path,file_check[0],input_copy_pipes);
                if(strstr(input_copy_pipes,"|")==NULL) process_cmd(toks,&n_redirect,path,file_check[0]);
                else{
                    parse_pipes(input_copy_pipes,path);
                }
                free_tokens(toks,n_redirect);
                return;
            }

        }
        // parse_cmd_pipes(tokens,&n,path,NULL,input_copy_pipes);
        if(strstr(input_copy_pipes,"|")==NULL) process_cmd(tokens,&n,path,NULL);
        else{
            parse_pipes(input_copy_pipes,path);
                }
        free_tokens(tokens,n);
    }
}

void parse_pipes(char* input,char* path[])
{
    int pid;
    int fd_in = dup(STDIN_FILENO);
    int fd_out = dup(STDOUT_FILENO); 
    int n_pipes;
    char* del = "|";
    char** tok_pipes = parse(input,&n_pipes,del);
    
    for(int i = 0; i<n_pipes-1; i++)
    {
        int pd[2];
        pipe(pd);
        pid=fork();
        if(pid==0) {
            dup2(pd[1], STDOUT_FILENO); 
            parse_command(tok_pipes[i],path);
            exit(0);
        }
        dup2(pd[0], STDIN_FILENO);
        close(pd[0]);
        close(pd[1]);
    }
 
    parse_command(tok_pipes[n_pipes-1],path);
    free_tokens(tok_pipes,path);
    int w1; 
    int r1;
    r1=waitpid(pid, &w1,0);
    dup2(fd_in, STDIN_FILENO);
    dup2(fd_out, STDOUT_FILENO);
    return;
}

void process_cmd(char **tokens,int *n,char* path[],char* filename)
{
    int pid = fork();
    if(pid<0)
    {
        ERROR(1,"An error has occurred\n");
        exit(1);
    }
    if(pid==0)
    { 
        if(filename!=NULL)
        {
        int fp = 0;
        while(filename[fp]!='\n') fp++;
        filename[fp] = NULL;
        int fd1;
        if ((fd1 = creat(filename, 0644)) < 0) {
            perror("Couldn't open the output file");
            exit(0);
        }           
        dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
        close(fd1);
        }


        int i = 0;
        while(tokens[(*n)-1][i]!='\n' && tokens[(*n)-1][i]!='\0') i++;
        tokens[(*n)-1][i] = NULL;

        int j = 0;
        if(!access(tokens[0],X_OK))
        {
            char* myargs[MAX_INPUT_SIZE]; 
            i = 0;
            while(tokens[i]!=NULL)
            {
                myargs[i] = tokens[i];
                i++;
            }
            myargs[i] = NULL;
            execv(myargs[0],myargs);
            exit(0);
        }
        else
        {
        while(path[j]!=NULL)
        {
            char* command = malloc(sizeof(command));
            command = strcat(command,path[j]);
            command = strcat(command,"/");
            command = strcat(command,tokens[0]);
    
            if(!access(command,X_OK))
            {
                char* myargs[MAX_INPUT_SIZE]; 
                i = 1;
                int count_white = 0;
                myargs[0] = command;
                while(tokens[i]!=NULL)
                {
                    if(!strcmp(tokens[i],""))
                    {
                        count_white++;
                        i++;
                        continue;
                    }
                    myargs[i-count_white] = tokens[i];
                    i++;
                }

                myargs[i-count_white] = NULL;
                execv(myargs[0],myargs);
                exit(0);
            }
            j++;
        }
        }
        ERROR(0,"An error has occured\n");
        exit(0);
    }
    else{
    wait(NULL);
    }
}

void process_command_self(char **tokens,char* path[])
{
     if(!strcmp("exit\n",tokens[0])) exit(0);
     if(!strcmp("exit",tokens[0]) && tokens[1]!=NULL) 
     {ERROR(0,"An error has occured\n");}
     else if(!strcmp("cd\n",tokens[0]))
     {
        ERROR(0,"An error has occurred\n");
     }
     else if(!strcmp("cd",tokens[0])) 
        {   
            int c = 0;
            while(tokens[1][c] != '\n')
            {
                c++;
            }
            tokens[1][c] = NULL;
            if(chdir(tokens[1])==-1)
            {
              ERROR(0,"An error has occurred\n");
            }
        }
    else if(!strcmp(tokens[0],"path\n"))
    {
        int i = 0;
        while(path[i]!=NULL)
        {
            path[i] = NULL;
            i++;
        }
    }
    else if(!strcmp(tokens[0],"path"))
    {
        int i = 0;
        while(path[i]!=NULL)
        {
            path[i] = NULL;
            i++;
        }
        i = 1;
        while(tokens[i]!=NULL)
        {
            path[i-1] = tokens[i];
            i++;
        }
        int j = 0;
        while(path[i-2][j]!='\n')
        {
            j++;
        }
        path[i-2][j] = NULL;

    }
    return;
}

int main(int argc, char *argv[])
{
    char* path[MAX_PATH_COUNT];
    path[0] = "/bin";
    char *input=NULL;
    size_t n; 

    if(argc==1)
    while(1)
    {
        char cwd[1024];
        printf("%s",getcwd(cwd,sizeof(cwd)));
        printf("\n");
        printf("anubis> "); 
        if(!getline(&input, &n, stdin)) break;
        parse_command_parallel(input,path);
        input[0] = '\0';
        if(feof(stdin)) break;
    }
    else if(argc==2)
    {
        
        FILE* fd = fopen(argv[1],"r");
        if(fd==NULL)
        {
            ERROR(1,"An error has occured\n");
            exit(1);
        }
        while(1) 
        {
        if(getline(&input, &n, fd)==-1) exit(0);
        parse_command_parallel(input,path);
        input[0] = '\0';
        if(feof(stdin)) break;
    }
    }
    else{
        ERROR(1,"An error has occured\n");
        exit(1);
    }

    return 0;
}



