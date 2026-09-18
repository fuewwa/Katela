#ifndef STANDART_H
#define STANDART_H

#define DISTRO  "Katela Kernel"
#define VERSION "1.3"

// hi :3

void init();
void help();
void execute_command(char *buffer);

char *cpuinfo(void);
int strlen(const char *str);

#endif
