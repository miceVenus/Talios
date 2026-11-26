__asm__ (".pushsection .text.entry,\"ax\"\n jmp main\n .popsection");

#include "fcntl.h"
#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "dirent.h"
#include "sys.h"

static char current_dir[4096];

typedef struct buildin_cmd{
    char *name;
    int (*func)(int argc, char **argv);
}buildin_cmd;

// #define COMMAND(name) int name##_command(int argc, char **argv);


// COMMAND(cd);

int cd_command(int argc, char **argv){
    int i = 0;

    char *path = NULL;

    unsigned long len = strlen(current_dir);

    if(argc < 2) return -1;

    if(strcmp(argv[1], ".") == 0) return 1;

    if(strcmp(argv[1], "..") == 0){
        if(strcmp("/", current_dir) == 0) return 1;

        for(i = len - 1; i > 0; i--){
            if(current_dir[i] == '/') break;
        }

        current_dir[i ? i : 1] = '\0';
        return 1;
    }

    i = len + strlen(argv[1]);
    if((unsigned long)i + 2 > sizeof(current_dir)) return -1;
    path = (char *)malloc(i + 2);
    if(!path) return -1;
    memset(path, 0, i+2);

    strcpy(current_dir, path);

    if(current_dir[len - 1] != '/'){
        path[len] = '/';
        path[len+1] = '\0';
    }

    strncat(path, argv[1], strlen(argv[1]) + 1);

    if(!chdir(path)){
        strcpy(path, current_dir);
    }else{
        printf("error in command cd bad path %s \n", path);
    }

    free(path);

    return 1;
}

int ls_command(int argc, char **argv){

    DIR * t_dir = NULL;
    dirent * buf= NULL;

    t_dir = opendir(current_dir);
    if(!t_dir) return -1;

    while(1){
        buf = readdir(t_dir);
        if(!buf) break;
        printf("%s\n", buf->d_name);
    }

    closedir(t_dir);

    return 1;
}

int pwd_command(int argc, char **argv){
    printf("%s\n", current_dir);
    return 1;
}

int cat_command(int argc, char **argv){
    if(argc < 2) return -1;

    int cd_len = strlen(current_dir);

    int len = cd_len + strlen(argv[1]);

    char *path = (char *)malloc(len + 2);
    if(!path) return -1;

    memset(path, 0, len + 2);

    strcpy(current_dir, path);
    if(current_dir[cd_len - 1] != '/'){
        path[cd_len] = '/';
        path[cd_len+1] = '\0';
    }
    strncat(path, argv[1], strlen(argv[1]) + 1);

    int fd = open(path, O_RONLY);
    free(path);
    if(fd < 0) return -1;
    long length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    printf("length : %D\n", length);
    char buf[513];
    while((len = read(fd, buf, sizeof(buf) - 1)) > 0){
        buf[len] = 0;
        putstring(buf);
    }
    putstring("\n");
    close(fd);
    return len < 0 ? -1 : 1;
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
    reboot(SYSTEM_REBOOT, NULL);
    return 1;
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

int read_line(int fd, char *buf, int capacity){

    int key = 0;
    int count = 0;

    while(1){
        key = AnalyzeKeyCode(fd);
        if(key == '\n'){ buf[count] = 0; return count; }

        if(key && count < capacity - 1){
            buf[count++] = (char)key;
            printf("%c",key);
        }
    }

}

int parse_command(char *buf, int *argc, char ***argv){
    *argc = 0;
    for(char *p = buf; *p; p++){
        if(*p != ' ' && (p == buf || p[-1] == ' ')) (*argc)++;
    }
    if(!*argc) return -1;
    *argv = malloc(sizeof(**argv) * *argc);
    if(!*argv) return -1;
    int i = 0;
    for(char *p = buf; *p; p++){
        if(*p == ' ') *p = 0;
        else if(p == buf || !p[-1]) (*argv)[i++] = p;
    }

    return find_cmd(**argv);
}

void run_command(int index, int argc, char** argv){
    shell_internal_cmd[index].func(argc, argv);
}

int main(){

    char path[] = "/KEYBOARD.DEV";
    char buf[256];
    int index = -1;

    int fd = open(path, 0);

    current_dir[0] = '/';
    current_dir[1] = 0;

    while(1){
        int argc = 0;
        char **argv = NULL;
        printf("[SHELL]#:");
        read_line(fd, buf, sizeof(buf));

        printf("\n");

        index = parse_command(buf, &argc, &argv);

        if(index < 0)
            printf("Input Error, No Command Found! \n");
        else
            run_command(index, argc, argv);
        free(argv);
    };

    close(fd);

    while(1){

    }

    return 0;
}
