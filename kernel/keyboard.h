#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "lib.h"

#define KB_DATA_PORT    0x60
#define KB_CMD_PORT     0x64
#define KB_STATUS_PORT  0x64
#define KB_CONF_WRITE   0x60
#define KB_CONF_READ    0x20

#define KB_INIT_CONF    0x47
#define KB_BUF_SIZE     100

#define KB_STATUS_IBF   0x2
#define KB_STATUS_OBF   0x1

#define NR_SCAN_CODES   0x80
#define MAP_COLS        0x2
#define PB_CODE_SIZE    6

#define PAUSE_BREAK      1
#define PRINT_SCREEN     2
#define OTHER_KEY        4

#define FLAG_BREAK      0x80


#define WAIT_KB_WRITE() while(IN8b(KB_STATUS_PORT) & KB_STATUS_IBF);
#define WAIT_KB_READ() while(IN8b(KB_STATUS_PORT) & KB_STATUS_OBF);

#define KEY_CMD_RESET_BUFFER 1


typedef struct KeyboardInBuf{
    unsigned char * PHead;
    unsigned char * PTail;

    int count;
    unsigned char buf[KB_BUF_SIZE];

}KeyboardInBuf;

void KeyboardInit();
void AnalyzeKeyCode();

#endif