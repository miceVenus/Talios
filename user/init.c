#include "fcntl.h"
#include "stdio.h"
#include "unistd.h"

int main(){
        // Can`t Be Called
    // ColorPrintfk(BLUE, BLACK, "In User Level\n");

    char string[] = "/SJKLDJLK/shdadhajskh/SAD/LLLKSNNMM.txt";
    char string2[] = "fuck you fuck you fuck you\n";
    char buf[512];
    long errono = 0;

    int fd = open(string, 0);

    if(fd == 0) {putstring("error in open");}

    fd = errono;

    if(read(fd, buf, 17))
        putstring("error in read");

    if(write(fd, string2, 30))
        putstring("error in write");

    if(lseek(fd, 0, SEEK_SET))
        putstring("error in lseek");

    if(read(fd, buf, 60))
        putstring("error in read");

    putstring(buf);

    close(fd);

    if(fork() == 0){
        putstring("here is child process");
    }else{
        putstring("here is parent process");
        malloc(100);
    }
    
    while(1){

    };
}