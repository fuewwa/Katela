#ifndef STANDART_H
#define STANDART_H

#define DISTRO  "Katela Kernel"
#define VERSION "1.3"

// hi :3

#define SHELL_LINE_SIZE 256

void init();
void help();
void execute_command(char *buffer);

char *cpuinfo(void);

#endif
