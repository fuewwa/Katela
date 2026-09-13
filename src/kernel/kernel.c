#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/speaker.h"
#include "../../include/standart.h"
#include "../../include/fs.h"
#include "../../include/panic.h"
#include "../../include/scheduler.h"
#include "../../include/mm.h" // useless at now

// swiss function
void swiss() {
    clear();
    print("Swiss editor (ESC to exit)\n\n");

    while (1) {
        char c = get_key();

        if (c == 27) { // ESC
            break;
        } else if (c == '\b') {
            backspace();
        } else if (c == '\n') {
            print("\n");
        } else {
            char str[2] = {c, 0};
            print(str);
        }
    }

    clear();
}

// main function
void kernel_main() {
    init();
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

            // parse command
            char command[64];
            int i = 0;

            while (buffer[i] != ' ' && buffer[i] != '\0') {
                command[i] = buffer[i];
                i++;
            }
            command[i] = '\0';

            char *args = buffer + i;
            if (*args == ' ') args++;

            // commands
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
			            files[i + 1];
			        }
			    file_count--;
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
                continue;
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
                continue;
            }

            if (find_file(new_name) != -1) {
                print("file already exists\n");
                continue;
            }

            strcpy(files[idx].name, new_name);

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
        	    continue;
    		}

    		int idx = find_file(fname);

    		if (idx == -1) {
        	    if (file_count >= MAX_FILES) {
            	    	print("file limit reached\n");
            	        continue;
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

            } else {
                print("command not found\n");
            }

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
