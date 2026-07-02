__asm__ ("jmp main \n\t");

#include "fcntl.h"
#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"

char *current_dir = NULL;

typedef struct buildin_cmd{
    char *name;
    int (*func)(int argc, char **argv);
}buildin_cmd;

// #define COMMAND(name) int name##_command(int argc, char **argv);


// COMMAND(cd);

int cd_command(int argc, char **argv){

}

int ls_command(int argc, char **argv){

}

int pwd_command(int argc, char **argv){
    if(current_dir)
        printf("%s\n", current_dir);
}

int cat_command(int argc, char **argv){

}

int touch_command(int argc, char **argv){

}

int rm_command(int argc, char **argv){

}

int mkdir_command(int argc, char **argv){

}

int rmdir_command(int argc, char **argv){

}

int exec_command(int argc, char **argv){

}

int reboot_command(int argc, char **argv){

}


buildin_cmd shell_internal_cmd[] = {
    {"cd", cd_command},
    {"ls", ls_command},
    {"pwd", pwd_command},
    {"cat", cat_command},
    {"touch", touch_command},
    {"rm", rm_command},
    {"mkdir", mkdir_command},
    {"rmdir", rmdir_command},
    {"exec", exec_command},
    {"reboot", reboot_command},
};

int find_cmd(char *cmd){
    int i = 0;
    for(i = 0; i < (sizeof(shell_internal_cmd) / sizeof(buildin_cmd)); i++){
        if(strcmp(cmd, shell_internal_cmd[i].name) == 0) return i;
    }
    return -1;
}

int read_line(int fd, char *buf){

    int key = 0;
    int count = 0;

    while(1){
        key = AnalyzeKeyCode(fd);
        if(key == '\n') return count;

        if(key){
            buf[count++] = (char)key;
            printf("%c",key);
        }
    }

}

int parse_command(char *buf, int *argc, char ***argv){
    int i, j = 0;

    while(buf[j] == ' ') j++;

    for(i = j; i < 256; i++){
        if(!buf[i]) break;

        if(buf[i] != ' ' && (buf[i+1] == ' ' || buf[i+1] == '\0')){
            (*argc)++;
        }
    }

    printf("parse_command argc:%d\n", *argc);

    if(!(*argc)){
        return -1;
    }

    *argv = (char **)malloc(sizeof(char **) * (*argc));

    for(i = 0; i < *argc && j < 256; i++){
        *(*argv + i) = &buf[j];
        while(buf[j] != ' ' && buf[j] != '\0') j++;
        buf[j++] = '\0';
        while(buf[j] == ' ') j++;
        printf("%s\n", (*argv)[i]);
    }

    return find_cmd(**argv);
}

void run_command(int index, int argc, char** argv){
    printf("run_command %s\n", shell_internal_cmd[index].name);
    shell_internal_cmd[index].func(argc, argv);
}

int main(){

    char path[] = "/KEYBOARD.DEV";
    unsigned char buf[256] = {0};
    int index = -1;

    int fd = open(path, 0);

    while(1){
        int argc = 0;
        char **argv = NULL;
        printf("[SHELL]#:");
        memset(buf, 0, 256);

        read_line(fd, buf);

        printf("\n");

        index = parse_command(buf, &argc, &argv);

        if(index < 0)
            printf("Input Error, No Command Found! \n");
        else
            run_command(index, argc, argv);
    };

    close(fd);

    while(1){

    }

    return 0;
}