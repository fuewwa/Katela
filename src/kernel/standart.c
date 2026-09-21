#include "../../include/standart.h"
#include "../drivers/vga.h"
#include "../drivers/speaker.h"
#include "../drivers/ata.h"
#include "../../include/fs.h"
#include "../../include/panic.h"
#include "../../include/usermode.h"
#include "../../include/syscall.h"
#include "../../include/kernel.h"
#include "../../include/mem.h"
#include "../../include/mm.h"

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
    char line[SHELL_LINE_SIZE];
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

static void print_uint(unsigned int value, int width) {
    char digits[12];
    int count = 0;
    int i;

    if (value == 0) {
        digits[count++] = '0';
    }

    while (value > 0) {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    }

    for (i = count; i < width; i++) {
        print(" ");
    }

    for (i = count - 1; i >= 0; i--) {
        char text[2] = { digits[i], '\0' };
        print(text);
    }
}

static void print_fs_error(int code) {
    print("fs: ");
    print(fs_error_text(code));
    print("\n");
}

void execute_command(char *buffer) {
    char command[64];
    int i = 0;

    while (buffer[i] != ' ' && buffer[i] != '\0' && i < (int)sizeof(command) - 1) {
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

        unsigned int script_size;
        int error;
        char *script = fs_read_all(args, &script_size, &error);

        if (script == 0) {
            print_fs_error(error);
            return;
        }

        g_exec_script = script;

        enter_usermode(script_interpreter_entry, ring3_stack + sizeof(ring3_stack));

        g_exec_script = 0;
        kfree(script);

    } else if (strcmp(command, "help") == 0) {

        help();

    } else if (strcmp(command, "rm") == 0) {

        if (args[0] == '\0') {
            print("standart: filename required\n");
        } else {
            int result = fs_remove(args);

            if (result != FS_OK) {
                print_fs_error(result);
            }
        }

    } else if (strcmp(command, "clear") == 0 || strcmp(command, "cls") == 0) {

        clear();

    } else if (strcmp(command, "create") == 0) {

        if (args[0] == '\0') {
            print("name required\n");
        } else {
            int result = fs_create(args);

            if (result != FS_OK) {
                print_fs_error(result);
            }
        }

    } else if (strcmp(command, "rename") == 0) {

        char old_name[FS_NAME_MAX + 1];
        char new_name[FS_NAME_MAX + 1];

        int i = 0;

        while (args[i] != ' ' && args[i] != '\0' && i < FS_NAME_MAX) {
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

        while (args[i] != ' ' && args[i] != '\0' && i < FS_NAME_MAX) {
            new_name[i] = args[i];
            i++;
        }
        new_name[i] = '\0';

        int result = fs_rename(old_name, new_name);

        if (result != FS_OK) {
            print_fs_error(result);
        }

    } else if (strcmp(command, "see") == 0) {

        struct fs_iter it;
        struct fs_info info;
        int result;
        int shown = 0;

        fs_iter_begin(&it);

        while ((result = fs_iter_next(&it, &info)) > 0) {
            print_uint(info.size, 10);
            print("  ");
            print(info.name);
            print("\n");
            shown++;
        }

        if (result < 0) {
            print_fs_error(result);
        } else if (shown == 0) {
            print("files cannot be found\n");
        }

    } else if (strcmp(command, "get") == 0) {

        struct fs_file file;
        int result = fs_open(args, &file, 0);

        if (result != FS_OK) {
            print_fs_error(result);
        } else {
            char chunk[513];
            unsigned int offset = 0;
            int got;

            while ((got = fs_read(&file, offset, chunk, 512)) > 0) {
                chunk[got] = '\0';
                print(chunk);
                offset += (unsigned int)got;
            }

            if (got < 0) {
                print_fs_error(got);
            }

            print("\n");
        }

    } else if (strcmp(command, "set") == 0) {

        char fname[FS_NAME_MAX + 1];
        char* data = args;

        int i = 0;
        while (data[i] != ' ' && data[i] != '\0' && i < FS_NAME_MAX) {
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

        int result = fs_write_all(fname, data, (unsigned int)strlen(data));

        if (result != FS_OK) {
            print_fs_error(result);
        }

    } else if (strcmp(command, "df") == 0) {

        struct fs_stats stats;
        int result = fs_usage(&stats);

        if (result != FS_OK) {
            print_fs_error(result);
        } else {
            unsigned int unit = stats.cluster_bytes / 512;
            unsigned int total_kb = stats.total_clusters * unit / 2;
            unsigned int free_kb = stats.free_clusters * unit / 2;

            print("total: ");
            print_uint(total_kb, 0);
            print(" KB\nused:  ");
            print_uint(((stats.total_clusters - stats.free_clusters) * unit + 1) / 2, 0);
            print(" KB\nfree:  ");
            print_uint(free_kb, 0);
            print(" KB\ncluster: ");
            print_uint(stats.cluster_bytes, 0);
            print(" bytes\n");
        }

    } else if (strcmp(command, "format") == 0) {

        if (strcmp(args, "yes") != 0) {
            print("this erases all files, run: format yes\n");
        } else {
            int result = fs_format();

            if (result != FS_OK) {
                print_fs_error(result);
            } else {
                print("disk formatted\n");
            }
        }

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
    print("see - list files with sizes\n");
    print("set {name} {text} - write to file\n");
    print("get {name} - read file\n");
    print("beep - plays a sound\n");
    print("rename {old} {new} - renames file\n");
    print("rm {name} - removes file\n");
    print("dt - tests disk read/write\n");
    print("df - shows disk usage\n");
    print("format yes - erases and formats the disk\n");
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
