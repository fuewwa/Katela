#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../../include/standart.h"
#include "../../include/fs.h"
#include "../../include/gdt.h"
#include "../../include/idt.h"

// swiss function
void swiss() {
    static char buffer[FILE_DATA_SIZE + 40];
    int index = 0;

    clear();
    print("Swiss editor (ESC to exit)\n\n");

    while (1) {
        char c = get_key();

        if (c == 27) { // ESC
            break;
        } else if (c == '\b') {
            if (index > 0) {
                index--;
                backspace();
            }
        } else if (c == '\n') {
            if (index < (int)sizeof(buffer) - 1) {
                buffer[index++] = '\n';
            }
            print("\n");
        } else {
            if (index < (int)sizeof(buffer) - 1) {
                buffer[index++] = c;
                char str[2] = {c, 0};
                print(str);
            }
        }
    }

    buffer[index] = '\0';
    clear();

    if (buffer[0] == '?' && buffer[1] == '<') {
        int i = 2;
        char name[32];
        int n = 0;

        while (buffer[i] != '>' && buffer[i] != '\n' && buffer[i] != '\0' && n < 31) {
            name[n++] = buffer[i++];
        }
        name[n] = '\0';

        if (buffer[i] == '>' && n > 0) {
            i++;
            if (buffer[i] == '\n') {
                i++;
            }

            int idx = find_file(name);

            if (idx == -1) {
                if (file_count >= MAX_FILES) {
                    print("file limit reached\n");
                    return;
                }

                idx = file_count++;
                strcpy(files[idx].name, name);
            }

            int k = 0;
            while (buffer[i + k] != '\0' && k < FILE_DATA_SIZE - 1) {
                files[idx].data[k] = buffer[i + k];
                k++;
            }
            files[idx].data[k] = '\0';
            fs_save();

            print("saved script \"");
            print(name);
            print("\"\n");
        }
    }
}

// main function
void kernel_main() {
    init();
    fs_load();
    gdt_init();
    idt_init();
    print("\n\n> ");

    char buffer[64];
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

            if (index < 63) {
                buffer[index++] = c;

                char str[2] = {c, 0};
                print(str);
            }
        }
    }
}
