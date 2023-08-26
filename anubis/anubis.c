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

const int MAX_PATH_COUNT = 100;
const int MAX_INPUT_SIZE = 100;

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

void process_cmd(char **tokens,int *n,char* path[])
{
    int pid = fork();
    if(pid<0)
    {
        
        ERROR(0,"An error has occurred\n");
    }
    if(pid==0)
    { 
        // int fd1 ;
        // if ((fd1 = creat("output.txt" , 0644)) < 0) {
        //     perror("Couldn't open the output file");
        //     exit(0);
        // }           

        // dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
        // close(fd1);

        int i = 0;
        while(tokens[(*n)-1][i]!='\n') i++;
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
                myargs[0] = command;
                while(tokens[i]!=NULL)
                {
                    myargs[i] = tokens[i];
                    i++;
                }
                myargs[i] = NULL;
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
        if(!strcmp("exit\n",input)) exit(0);
        int n;
        char* del = " ";
        char** tokens = parse(input,&n,del);
        if(!strcmp(tokens[0],"exit\n") || !strcmp(tokens[0],"cd") || !strcmp(tokens[0],"cd\n") || 
           !strcmp(tokens[0],"path")|| !strcmp(tokens[0],"path\n") || !strcmp(tokens[0],"exit")) 
           process_command_self(tokens,path);
        else {
            if(strstr(input,">")==NULL)
            process_cmd(tokens,&n,path);
            
            }
        input[0] = '\0';
        if(feof(stdin)) break; 
    }
    else if(argc==2)
    {
        
        FILE* fd = fopen(argv[1],"r");
        if(fd==NULL)
        {
            ERROR(0,"An error has occured\n");
            exit(1);
        }
        while(1) 
        {
        if(getline(&input, &n, fd)==-1) exit(0);
        // printf("%s",input);
        if(!strcmp("exit\n",input)) exit(0);
        int n;
        char* del = " ";
        char** tokens = parse(input,&n,del);
        if(!strcmp(tokens[0],"exit\n") || !strcmp(tokens[0],"cd") || !strcmp(tokens[0],"cd\n") || !strcmp(tokens[0],"path")|| !strcmp(tokens[0],"path\n")) process_command_self(tokens,path);
        else {
            process_cmd(tokens,&n,path);
            free_tokens(tokens,n);}
        input[0] = '\0';
        if(feof(stdin)) break;   
        }
    }
    else{
        ERROR(0,"An error has occured\n");
        exit(1);
    }

    return 0;
}


