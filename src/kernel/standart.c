#include "../../include/standart.h"
#include "../drivers/vga.h"
#include "../drivers/speaker.h"
#include "../drivers/ata.h"
#include "../../include/fs.h"
#include "../../include/panic.h"
#include "../../include/usermode.h"
#include "../../include/syscall.h"
#include "../../include/kernel.h"

static unsigned char ring3_stack[4096];
static const char *g_exec_script;

static void urun(const char *line) {
    unsigned int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_RUN), "b"(line));
}

static void uexit(void) {
    asm volatile("int $0x80" : : "a"(SYS_EXIT));
}

static void script_interpreter_entry(void) {
    const char *script = g_exec_script;
    char line[FILE_DATA_SIZE];
    int i = 0;
    int j = 0;

    while (script[i] != '\0') {
        char c = script[i];

        if (c == ':' && j > 0 && line[j - 1] == ' ') {
            line[j - 1] = '\0';
            if (line[0] != '\0') {
                urun(line);
            }
            j = 0;
        } else if (c == '\n') {
            if (j > 0 && line[j - 1] != ' ' && j < (int)sizeof(line) - 1) {
                line[j++] = ' ';
            }
        } else if (c == ' ' && j == 0) {
            // skip leading whitespace of a new statement
        } else if (j < (int)sizeof(line) - 1) {
            line[j++] = c;
        }

        i++;
    }

    if (j > 0) {
        line[j] = '\0';
        urun(line);
    }

    uexit();
}

void execute_command(char *buffer) {
    char command[64];
    int i = 0;

    while (buffer[i] != ' ' && buffer[i] != '\0') {
        command[i] = buffer[i];
        i++;
    }
    command[i] = '\0';

    char *args = buffer + i;
    if (*args == ' ') args++;

    if (strcmp(command, "info") == 0) {

        print("Distro: \"");
        print(DISTRO);
        print("\"\n");

        print("Version: \"");
        print(VERSION);
        print("\"\n");

    } else if (strcmp(command, "echo") == 0) {

        print(args);
        print("\n");

    } else if (strcmp(command, "swiss") == 0) {

        swiss();

    } else if (strcmp(command, "panic") == 0) {

        panic("user requested panic");

    } else if (strcmp(command, "off") == 0) {

        print("You can safely turn off your PC now.\n");

        asm volatile("cli");
        while (1) asm volatile("hlt");

    } else if (strcmp(command, "beep") == 0) {

        speaker_beep();
        speaker_off();

    } else if (strcmp(command, "dt") == 0) {

        unsigned char out[512];
        unsigned char in[512];
        int i;

        for (i = 0; i < 512; i++) {
            out[i] = (unsigned char)(i & 0xFF);
        }

        if (ata_write_sector(5, out) != 0) {
            print("disk: write failed\n");
        } else if (ata_read_sector(5, in) != 0) {
            print("disk: read failed\n");
        } else {
            int ok = 1;

            for (i = 0; i < 512; i++) {
                if (in[i] != out[i]) {
                    ok = 0;
                    break;
                }
            }

            print(ok ? "disk: ok\n" : "disk: mismatch\n");
        }

    } else if (strcmp(command, "exec") == 0) {

        if (args[0] == '\0') {
            print("filename required\n");
            return;
        }

        int idx = find_file(args);

        if (idx == -1) {
            print("file not found\n");
            return;
        }

        g_exec_script = files[idx].data;

        enter_usermode(script_interpreter_entry, ring3_stack + sizeof(ring3_stack));

    } else if (strcmp(command, "help") == 0) {

        help();

    } else if (strcmp(command, "rm") == 0) {

        if (args[0] == '\0') {
            print("standart: filename required\n");
        } else {

            int idx = find_file(args);
            if (idx == -1) {
                print("standart: file not found");
            } else {
                for (int i = idx; i < file_count - 1; i++) {
                    files[i] = files[i + 1];
                }
                file_count--;
                fs_save();
            }
        }

    } else if (strcmp(command, "clear") == 0 || strcmp(command, "cls") == 0) {

        clear();

    } else if (strcmp(command, "create") == 0) {

        if (args[0] == '\0') {
            print("name required\n");
        } else if (file_count >= MAX_FILES) {
            print("file limit reached\n");
        } else {
            strcpy(files[file_count].name, args);
            file_count++;
            fs_save();
        }

    } else if (strcmp(command, "rename") == 0) {

        char old_name[32];
        char new_name[32];

        int i = 0;

        while (args[i] != ' ' && args[i] != '\0' && i < 31) {
            old_name[i] = args[i];
            i++;
        }
        old_name[i] = '\0';

        if (args[i] == '\0') {
            print("new filename required\n");
            return;
        }

        args += i + 1;

        i = 0;

        while (args[i] != ' ' && args[i] != '\0' && i < 31) {
            new_name[i] = args[i];
            i++;
        }
        new_name[i] = '\0';

        int idx = find_file(old_name);

        if (idx == -1) {
            print("file not found\n");
            return;
        }

        if (find_file(new_name) != -1) {
            print("file already exists\n");
            return;
        }

        strcpy(files[idx].name, new_name);
        fs_save();

    } else if (strcmp(command, "see") == 0) {

        if (file_count == 0) {
            print("files cannot be found\n");
        } else {
            for (int i = 0; i < file_count; i++) {
                print(files[i].name);
                print("\n");
            }
        }

    } else if (strcmp(command, "get") == 0) {

        int idx = find_file(args);

        if (idx == -1) {
            print("file not found\n");
        } else {
            print(files[idx].data);
            print("\n");
        }

    } else if (strcmp(command, "set") == 0) {

        char fname[32];
        char* data = args;

        int i = 0;
        while (data[i] != ' ' && data[i] != '\0') {
            fname[i] = data[i];
            i++;
        }
        fname[i] = '\0';

        if (data[i] == ' ') {
            data += i + 1;
        } else {
            print("missing value\n");
            return;
        }

        int idx = find_file(fname);

        if (idx == -1) {
            if (file_count >= MAX_FILES) {
                print("file limit reached\n");
                return;
            }

            idx = file_count++;
            strcpy(files[idx].name, fname);
        }

        int j = 0;
        while (data[j] && j < FILE_DATA_SIZE - 1) {
            files[idx].data[j] = data[j];
            j++;
        }
        files[idx].data[j] = '\0';
        fs_save();

    } else {
        print("command not found\n");
    }
}

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
    print("swiss - open text editor (?<name> as first line saves a script)\n");
    print("create {name} - create file\n");
    print("see - list files\n");
    print("set {name} {text} - write to file\n");
    print("get {name} - read file\n");
    print("beep - plays a sound\n");
    print("rename {old} {new} - renames file\n");
    print("rm {name} - removes file\n");
    print("dt - tests disk read/write\n");
    print("exec {name} - runs a script file in ring 3\n");
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
