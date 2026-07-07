#ifndef SYS_H
#define SYS_H

#define SYSTEM_REBOOT 1
#define SYSTEM_POWEROFF 2

int reboot(int cmd, void *arg);

#endif