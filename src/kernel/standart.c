#include "../../include/standart.h"
#include "../drivers/vga.h"
#include "../drivers/speaker.h"

void init() {
    init_speaker();
    clear();

    print("Welcome to ");
    print(DISTRO);
    print("!\nWrite help for cmd list! ");
}

void help() {
    print("off - turn off cpu to safety power off\n");
	print("echo - prints arguments\n");
	print("info - prints information about ts\n");
	print("clear or cls - clears all console\n");
	print("swiss - open text editor\n");
	print("create {name} - create file\n");
	print("see - list files\n");
	print("set {name} {text} - write to file\n");
	print("get {name} - read file\n");
	print("beep - plays a sound\n");
    print("rename {old} {new} - renames file\n");
	print("rm {name} - removes file\n");
    print("dt - tests disk read/write\n");
    print("panic - requests system panic");
}

char *cpuinfo(void)
{
    static char vendor[13];
    unsigned int eax, ebx, ecx, edx;

    asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    vendor[0]  = ebx;
    vendor[1]  = ebx >> 8;
    vendor[2]  = ebx >> 16;
    vendor[3]  = ebx >> 24;

    vendor[4]  = edx;
    vendor[5]  = edx >> 8;
    vendor[6]  = edx >> 16;
    vendor[7]  = edx >> 24;

    vendor[8]  = ecx;
    vendor[9]  = ecx >> 8;
    vendor[10] = ecx >> 16;
    vendor[11] = ecx >> 24;

    vendor[12] = '\0';

    return vendor;
}

int strlen(const char *str)
{
    int len = 0;

    while (str[len] != '\0')
        len++;

    return len;
}
