#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

#define ARRAY_SIZE 1000

int background = 0;

void parse_command(char *src, char *s[], char *in, char *out){
	int index = 0;
	char *tok;
	for (tok = strtok(src, " "); tok && index < ARRAY_SIZE;
			tok = strtok(NULL, " ")){
		if (strcmp(tok, "<") == 0){
			strcpy(in, tok = strtok(NULL, " \n"));
		}
		else if (strcmp(tok, ">") == 0)
			strcpy(out, tok = strtok(NULL, " \n"));
		else
			s[index++] = tok;
	}
	s[index] = NULL;
}

char *remove_space_before_after(char *src){
	int len_string = strlen(src);
	for (int j = 0; j < len_string && src[j] == ' '; j++)
		src++;
	for (int j = len_string-1; j >= 0 && src[j] == ' '; j--)
		src[j] = '\0';
	return src;
}


void check_background_indicator(char *prompt){
	int prompt_last_index = strlen(prompt) - 1;
	if (prompt[prompt_last_index] == '&'){
		background = 1;
		prompt[prompt_last_index] = '\0';
	}
	else background = 0;
}

int get_prompt(char *dst){
	printf("%c", 37);
	fgets(dst, 255, stdin);
	dst[strlen(dst) - 1] = '\0';
	remove_space_before_after(dst);
	check_background_indicator(dst);
	return (strlen(dst)) ? 1: 0;
}

void open_filedescriptor(char *name, int flags, int mode, int x){
	int fd = open(name, flags, mode);
	if (fd < 0) perror(name), exit(1);
	if (dup2(fd, x) < 0) perror("Dup2 failed");
	close(fd);
}

void initiate_background(){
	pid_t sid = setsid();
	if (sid < 0) perror("Setsid() failed");
	int fd = open("/dev/NULL", O_RDWR);
	dup2(fd, 0);
	close(fd);
}

void io_redirection(char **cmd, char *in, char *out){
	int status;
	switch(fork()){
	case 0:
		if (strlen(in) > 0)
			open_filedescriptor(in, O_RDONLY, 0664, 0);
		if (strlen(out) > 0)
			open_filedescriptor(out, O_CREAT | O_WRONLY | O_TRUNC, 0664, 1);
		execvp(cmd[0], cmd);
		perror(cmd[0]), exit(1);
	case -1:
		perror("Fork");
	default:
		while (wait(&status) > 0);
		break;
	}
}

void close_pipe(int p[][2], int size){
	for (int i = 0; i < size; i++)
		close(p[i][0]), close(p[i][1]);
}


void pipe_start(char **cmd, int count){
	int p[count-1][2];
	for(int i = 0; cmd[i]; i++){
		char in[55] = "", out[55] = "", *command[ARRAY_SIZE];
		parse_command(cmd[i], command, in, out);
		pipe(p[i]);
		switch(fork()){
		case 0:
			if (i == 0){
				if (strlen(in) > 0)
					open_filedescriptor(in, O_RDONLY, 0664, 0);
				if (dup2(p[i][1], 1) == -1) perror("Dup2 failed");
			}
			else if (i == count - 1){
				if (strlen(out) > 0)
					open_filedescriptor(out, O_CREAT | O_WRONLY | O_TRUNC, 0664, 1);
				if (dup2(p[i-1][0], 0) == -1) perror("Dup2 failed");
			}
			else{
				if (dup2(p[i-1][0], 0) == -1) perror("Dup2 failed");
				if (dup2(p[i][1], 1) == -1) perror("Dup2 failed");
			}
			close_pipe(p, i+1);
			execvp(command[0], command);
			perror("Exec Failed");
		case -1:
			perror("Fork");
		}
	}
	close_pipe(p, count);
	while(wait(NULL) != -1);
}

int split_pipe(char **dst, char *prompt){
	int count = 0;
	for (char *tok = strtok(prompt, "|"); tok; tok = strtok(NULL, "|"))
		dst[count++] = remove_space_before_after(tok);
	dst[count] = NULL;
	return count;
}

void start_shell(){
	while(1){
		char prompt[255];
		if (get_prompt(prompt)){
			if (background){
				pid_t pid = fork();
				if (pid == -1) perror("Fork failed");
				else if (pid != 0)
					continue;
				else
					initiate_background();
			}
			char *pipe_split[55];
			int pipe_count = split_pipe(pipe_split, prompt);
			if (pipe_count == 1){
				char *command[ARRAY_SIZE], in[255] = "", out[255] = "";
				parse_command(pipe_split[0], command, in, out);
				io_redirection(command, in, out);
			}
			else
				pipe_start(pipe_split, pipe_count);
		}
		if (background) exit(0);
	}
}

int main(int argc, char **argv, char **envp){
	start_shell();
	return 0;
}
