#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../../include/standart.h"
#include "../../include/fs.h"
#include "../../include/gdt.h"
#include "../../include/idt.h"
#include "../../include/mem.h"
#include "../../include/mm.h"

#define SWISS_START_SIZE 1024
#define SWISS_MAX_SIZE (512 * 1024)

void swiss() {
    unsigned int capacity = SWISS_START_SIZE;
    unsigned int index = 0;
    char *buffer = kmalloc(capacity);

    clear();
    print("Swiss editor (ESC to exit)\n\n");

    while (1) {
        char c = get_key();

        if (c == 27) {
            break;
        } else if (c == '\b') {
            if (index > 0) {
                index--;
                backspace();
            }
        } else {
            if (index + 1 >= capacity) {
                if (capacity >= SWISS_MAX_SIZE) {
                    continue;
                }

                char *bigger = kmalloc(capacity * 2);
                memcpy(bigger, buffer, index);
                kfree(buffer);
                buffer = bigger;
                capacity *= 2;
            }

            buffer[index++] = c;

            if (c == '\n') {
                print("\n");
            } else {
                char str[2] = {c, 0};
                print(str);
            }
        }
    }

    buffer[index] = '\0';
    clear();

    if (buffer[0] == '?' && buffer[1] == '<') {
        unsigned int i = 2;
        char name[FS_NAME_MAX + 1];
        int n = 0;

        while (buffer[i] != '>' && buffer[i] != '\n' && buffer[i] != '\0' && n < FS_NAME_MAX) {
            name[n++] = buffer[i++];
        }
        name[n] = '\0';

        if (buffer[i] == '>' && n > 0) {
            i++;
            if (buffer[i] == '\n') {
                i++;
            }

            int result = fs_write_all(name, buffer + i, index - i);

            if (result != FS_OK) {
                print("fs: ");
                print(fs_error_text(result));
                print("\n");
            } else {
                print("saved script \"");
                print(name);
                print("\"\n");
            }
        }
    }

    kfree(buffer);
}

// main function
void kernel_main() {
    init();
    int mount = fs_init();

    if (mount == FS_INIT_FORMATTED) {
        print("\n\nnew disk formatted");
    } else if (mount == FS_INIT_MIGRATED) {
        print("\n\nold disk format converted");
    } else if (mount != FS_INIT_MOUNTED) {
        print("\n\nfs: ");
        print(fs_error_text(mount));
    }

    gdt_init();
    idt_init();
    print("\n\n> ");

    char buffer[SHELL_LINE_SIZE];
    int index = 0;

    while (1) {
        char c = get_key();

        if (c == '\n') {
            buffer[index] = '\0';
            print("\n");

            if (index == 0) {
                print("> ");
                continue;
            }

            execute_command(buffer);

            index = 0;
            print("\n> ");

        } else if (c == '\b') {

            if (index > 0) {
                index--;
                backspace();
            }

        } else {

            if (index < SHELL_LINE_SIZE - 1) {
                buffer[index++] = c;

                char str[2] = {c, 0};
                print(str);
            }
        }
    }
}
