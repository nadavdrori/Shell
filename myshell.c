

#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

int find_index(char **arglist, int size, char* target)
{
    for (int i = 0; i < size; i++) {
        if (strcmp(arglist[i], target) == 0) {
            return i;
        }
    }
    return -1;
}

int default_exec(char **arglist){
    int pid = fork();
    if (pid == -1) {
        // Fork failed
        fprintf(stderr, "fork failed");
        return 0;
    }
    else if (pid == 0){
        signal(SIGINT,SIG_DFL);
        if (execvp(arglist[0], arglist)==-1){
            fprintf(stderr, "execution failed");
            exit(1);
        }
    }
    else{
        int status;
        waitpid(pid, &status, 0);
    }
    return 1;
}

int background_exec(char **arglist){
    int pid = fork();
    if (pid == -1) {
        // Fork failed
        fprintf(stderr, "fork failed");
        return 0;
    }
    else if (pid == 0){
        // ignor signals
        if(execvp(arglist[0], arglist)==-1){
            fprintf(stderr, "execution failed");
            exit(1);
        }
    }
    else{
        return 1;
    }
}

int input_redr_com(char **arglist, int count){
    int pid = fork();
    if (pid == -1) {
        // Fork failed
        fprintf(stderr, "fork failed");
        return 0;
    }
    else if (pid == 0){
        int fd = open(arglist[count-1], O_CREAT|O_TRUNC|O_WRONLY);
        if (fd == -1) {
            fprintf(stderr, "error opening file");
            exit(1);
        }
        if (dup2(fd, 0) == -1) {
            fprintf(stderr, "error redirecting stdin");
            exit(1);
        }
        signal(SIGINT,SIG_DFL);
        arglist[count-2] = NULL;
        if (execvp(arglist[0], arglist)==-1){
            fprintf(stderr, "execution failed");
            exit(1);
        }
    }
    else{
        int status;
        waitpid(pid,&status,0);
    }
    return 1;
}

int output_redr_com(char **arglist, int count){
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork failed");
        return 0;
    }
    else if (pid == 0){
        int file_i = arglist[count-1];
        int fd = open(arglist[file_i], O_CREAT|O_TRUNC|O_WRONLY);
        if (fd == -1) {
            fprintf(stderr, "error opening file");
            exit(1);
        }
        if (dup2(fd, 1) == -1) {
            fprintf(stderr, "error redirecting stdin");
            exit(1);
        }
        arglist[count-2] = NULL;
        signal(SIGINT, SIG_DFL);
        if (execvp(arglist[0], arglist)==-1){
            fprintf(stderr, "execution failed");
            exit(1);
        }
    }
    else{
        int status;
        waitpid(pid, &status, 0);
    }
    return 1;
}

int pipe_exe(char **arglist, int pipe_i){
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "pipe failed");
        return 0;
    }
    int pid1 = fork();
    if (pid1 == -1) {
        fprintf(stderr, "fork1 failed");
        return 0;
    }
    else if(pid1 == 0){
        close(pipefd[0]);
        if (dup2(pipefd[1], 1)==-1){
            fprintf(stderr, "dup2 com1 failed");
            exit(1);
        }
        signal(SIGINT,SIG_DFL);
        close(pipefd[1]);
        if (execvp(arglist[0], arglist)==-1){
            fprintf(stderr, "execution failed com1");
            exit(1);
        }
    }
    else{
        int pid2 = fork();
        if (pid2 == -1) {
            fprintf(stderr, "fork2 failed");
            return 0;
        }
        else if(pid1 == 0){
            close(pipefd[1]);
            if (dup2(pipefd[0], 0)==-1){
                fprintf(stderr, "dup2 com2 failed");
                exit(1);
            }
            signal(SIGINT,SIG_DFL);
            close(pipefd[0]);
            if(execvp(arglist[0], arglist[pipe_i+1]) == -1){
                fprintf(stderr, "execution failed com2");
                exit(1);
            }
        }
        else{
            int status1, status2;
            close(pipefd[0]);
            close(pipefd[1]);

            waitpid(pid1, &status1, 0);
            waitpid(pid2, &status2, 0);
            return 1;
        }
    }
    return 1;
}

int prepare(void){
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    return 0;
}

int finalize(void){
    return 0;
}

int process_arglist(int count, char **arglist){

    int pipe_com = 0, background_com=0, i_redr_com = 0, o_redr_com = 0;

    for (int i=0; i<range(count); i++) {
        if (arglist[i][0] == "|"){
            pipe_com = 1;
        }
        else if (arglist[i][0]=="<"){
            i_redr_com = 1;
        }
        else if (strcmp(arglist[i], ">>")==0){
            o_redr_com = 1;
        }
        else if (arglist[count][0] == "&") {
            background_com = 1;
        }
    }
    if (pipe_com == 1){
        int pipe_index = find_index(arglist, count+1, "|");
        arglist[pipe_index] = NULL;
        if (pipe_exe(arglist, pipe_index) == 0);
        {return 0;}
    }
    else if (i_redr_com == 1) {
        if (input_redr_com(arglist, count)==0);
        {return 0;}
    }
    else if(o_redr_com == 1){
        if (output_redr_com(arglist, count)==0);
        {return 0;}
    }
    else if (background_com == 1){
        arglist[count-1] = NULL;
        if (background_exec(arglist)==0);
        {return 0;}
    }
    else{ 
        if (default_exec(arglist)==0);
        {return 0;}
    }
    return 1;
    
}

