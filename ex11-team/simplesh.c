/* simplesh.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_ARGS 50
#define MAX_INPUT_LENGTH 256

int getargs(char *cmd, char **argv);

void handle_signal(int signo);
void command_exit();
void command_ls(const char *dir);
void command_pwd();
void command_cd(const char *path);
void command_mkdir(const char *path);
void command_rmdir(const char *path);
void command_ln(const char *target, const char *linkpath);
void command_cp(const char *src, const char *dest);
void command_rm(const char *path);
void command_mv(const char *oldpath, const char *newpath);
void command_cat(const char *filename);

int main() {
    char buf[MAX_INPUT_LENGTH];
    char *argv[MAX_ARGS];
    pid_t pid;
    int background = 0;

    signal(SIGINT, handle_signal);
	signal(SIGTSTP, handle_signal);

    while (1) {
        printf("shell> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            perror("input error");
            exit(EXIT_FAILURE);
        }

        buf[strcspn(buf, "\n")] = '\0';

        int narg = getargs(buf, argv);
        
        if (narg == 0) continue;

 //       if (narg > 0 && strcspn(argv[narg], |) == 0)

        if (strcmp(buf, "exit") == 0) {
            command_exit();
        }
        else if (strcmp(buf, "ls") == 0) {
            command_ls(narg > 1 ? argv[1] : ".");
            continue;
        }
        else if (strcmp(buf, "pwd") == 0){
            command_pwd();
            continue;
        }
        else if (strcmp(argv[0], "cd") == 0) {
            if (narg < 2) {
                fprintf(stderr, "cd: Argument required\n");
            } else {
                command_cd(argv[1]);
            }
            continue;
        }
        else if (strcmp(argv[0], "mkdir") == 0){
            if (narg < 2) {
                fprintf(stderr, "mkdir: Directory name required\n");
            } else {
                command_mkdir(argv[1]);
            }
            continue;
        }
        else if (strcmp(argv[0], "rmdir") == 0){
            if (narg < 2) {
                fprintf(stderr, "rmdir: Directory name required\n");
            } else {
                command_rmdir(argv[1]);
            }
            continue;
        }
        else if (strcmp(argv[0], "ln") == 0){
            if (narg < 3) {
                fprintf(stderr, "ln: Target File and Path required\n");
            } else {
                command_ln(argv[1], argv[2]);
            }
            continue;
        }
        else if (strcmp(argv[0], "cp") == 0){
            if (narg < 3) {
                fprintf(stderr, "cp: Target File and Source File Path required\n");
            } else {
                command_cp(argv[1], argv[2]);
            }
            continue;
        }
        else if (strcmp(argv[0], "rm") == 0){
            if (narg < 2) {
                fprintf(stderr, "rm: File Path required\n");
            } else {
                command_rm(argv[1]);
            }
            continue;
        }
        else if (strcmp(argv[0], "mv") == 0){
            if (narg < 3) {
                fprintf(stderr, "mv: Target File and path required\n");
            } else {
                command_mv(argv[1], argv[2]);
            }
            continue;
        }
        else if (strcmp(argv[0], "cat") == 0){
            if (narg < 2) {
                fprintf(stderr, "cat: Filename required\n");
            } else {
                command_cat(argv[1]);
            }
            continue;
        }

        if (narg > 0 && strcmp(argv[narg - 1], "&") == 0) {
            background = 1;
            argv[--narg] = NULL; // 인수에서 '&' 제거
        }
        else {
            background = 0;
        }

        pid = fork();

        if (pid == 0) {
            if(background){
                freopen("/dev/null", "r", stdin);
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                setsid();
            }
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } 
        else if (pid > 0) {
            if (!background){
                waitpid(pid, NULL, 0);
            }
        } 
        else {
            perror("fork failed");
        }
    }
    return 0;
}

int getargs(char *cmd, char **argv) {
    int narg = 0;
    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t')
            *cmd++ = '\0';
        else {
            argv[narg++] = cmd++;
            while (*cmd != '\0' && *cmd != ' ' && *cmd != '\t')
                cmd++;
        }
    }
    argv[narg] = NULL;
    return narg;
}

void handle_signal(int signo){
    if(signo == SIGINT){
        printf("\nCtrl-C (SIGINT)\n");
        kill(getpid(), SIGTERM);
    }
    else if(signo == SIGTSTP){
        printf("\nCtrl-Z (SIGQUIT)\n");
        kill(getpid(), SIGSTOP);
    }
}

void command_exit(){
    printf("shell program close.\n");
    exit(0);
}

void command_ls(const char *dir){
	DIR *d = opendir(dir);
    struct dirent *entry;

	if (!d) {
		perror("open dir failed");
		return;
	}


	while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        printf("%s", entry->d_name);
		printf("    ");
        }
	}
    printf("\n");
	closedir(d);
}

void command_pwd(){
    char pwd[64];
    if (getcwd(pwd, sizeof(pwd)) != NULL){
        printf("%s\n", pwd);
    }
    else{
        perror("pwd error");
    }
}

void command_cd(const char *path) {
    if (chdir(path) != 0) {
        perror("chdir");
    }
}

void command_mkdir(const char *path){
    if(mkdir(path, 0777) != 0){
        perror("mkdir failed");
    }
}

void command_rmdir(const char *path){
    if(rmdir(path) != 0){
        perror("rmdir failed");
    }
}

void command_ln(const char *target, const char *linkpath) {
    if (link(target, linkpath) != 0) {
        perror("link");
    }
}

void command_cp(const char *src, const char *dest) {
    FILE *srcFile, *destFile;
    char buffer[1024];
    size_t bytesRead;

    srcFile = fopen(src, "rb");
    if (srcFile == NULL) {
        perror("fopen");
        return;
    }
    destFile = fopen(dest, "wb");
    if (destFile == NULL) {
        perror("fopen");
        fclose(srcFile);
        return;
    }
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destFile);
    }
    fclose(srcFile);
    fclose(destFile);
}

void command_rm(const char *path) {
    if (unlink(path) != 0) {
        perror("unlink");
    }
}

void command_mv(const char *oldpath, const char *newpath) {
    if (rename(oldpath, newpath) != 0) {
        perror("rename");
    }
}

void command_cat(const char *filename) {
    FILE *file;
    char buffer[1024];

    file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }
    fclose(file);
}