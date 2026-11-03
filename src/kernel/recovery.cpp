#include "recovery.hpp"
#include "../../include/recovery.h"
#include "../../include/io.h"

extern "C" unsigned multiboot_magic;
extern "C" unsigned multiboot_info;

namespace {

const unsigned MULTIBOOT_MAGIC = 0x2BADB002;
const unsigned INFO_FLAG_CMDLINE = 1u << 2;

const int WIDTH = 80;
const int HEIGHT = 25;

const unsigned char ATTR_NORMAL = 0x07;
const unsigned char ATTR_SELECTED = 0x70;
const unsigned char ATTR_TITLE = 0x1F;
const unsigned char ATTR_WARNING = 0x0E;
const unsigned char ATTR_OK = 0x0A;
const unsigned char ATTR_ERROR = 0x0C;

enum Key {
    KEY_NONE,
    KEY_UP,
    KEY_DOWN,
    KEY_ENTER,
    KEY_ESCAPE,
    KEY_YES,
    KEY_OTHER
};

enum Action {
    ACTION_REBOOT,
    ACTION_FACTORY,
    ACTION_POWER_OFF,
    ACTION_COUNT
};

const char *const ITEMS[ACTION_COUNT] = {
    "Reboot system now",
    "Reboot bootloader to factory conf",
    "Power off"
};

const int MENU_ROW = 3;
int progress_row = 0;

struct Info {
    unsigned flags;
    unsigned mem_lower;
    unsigned mem_upper;
    unsigned boot_device;
    unsigned cmdline;
};

volatile unsigned short *const video = reinterpret_cast<volatile unsigned short *>(0xB8000);

void put_cell(int x, int y, char c, unsigned char attr) {
    video[y * WIDTH + x] = static_cast<unsigned short>((attr << 8) | static_cast<unsigned char>(c));
}

void fill_row(int y, unsigned char attr) {
    for (int x = 0; x < WIDTH; x++) {
        put_cell(x, y, ' ', attr);
    }
}

void put_text(int x, int y, const char *text, unsigned char attr) {
    for (int i = 0; text[i] != '\0' && x + i < WIDTH; i++) {
        put_cell(x + i, y, text[i], attr);
    }
}

void clear_screen() {
    for (int y = 0; y < HEIGHT; y++) {
        fill_row(y, ATTR_NORMAL);
    }
}

void hide_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void number_text(unsigned value, char *out) {
    char digits[11];
    int count = 0;
    do {
        digits[count++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value != 0);
    for (int i = 0; i < count; i++) {
        out[i] = digits[count - 1 - i];
    }
    out[count] = '\0';
}

Key read_key() {
    while (true) {
        unsigned char status = inb(0x64);
        if (!(status & 1)) {
            return KEY_NONE;
        }
        unsigned char code = inb(0x60);
        if (status & 0x20) {
            continue;
        }
        if (code == 0xE0 || (code & 0x80)) {
            continue;
        }
        switch (code) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x1C: return KEY_ENTER;
            case 0x01: return KEY_ESCAPE;
            case 0x15: return KEY_YES;
            default: return KEY_OTHER;
        }
    }
}

Key wait_key() {
    while (true) {
        Key key = read_key();
        if (key != KEY_NONE) {
            return key;
        }
        asm volatile("pause");
    }
}

void draw_menu(int selected) {
    clear_screen();
    fill_row(0, ATTR_TITLE);
    put_text(1, 0, "Kernel recovery mode", ATTR_TITLE);

    for (int i = 0; i < ACTION_COUNT; i++) {
        unsigned char attr = (i == selected) ? ATTR_SELECTED : ATTR_NORMAL;
        fill_row(MENU_ROW + i, ATTR_NORMAL);
        for (int x = 2; x < WIDTH - 2; x++) {
            put_cell(x, MENU_ROW + i, ' ', attr);
        }
        put_text(3, MENU_ROW + i, ITEMS[i], attr);
    }

    put_text(2, HEIGHT - 2, "Use the Up and Down keys to select an entry, Enter to run it.", ATTR_NORMAL);
}

void draw_progress(unsigned done, unsigned total) {
    unsigned percent = total == 0 ? 0 : done * 100 / total;
    const int bar_x = 3;
    const int bar_width = 50;
    int filled = static_cast<int>(percent * bar_width / 100);

    for (int i = 0; i < bar_width; i++) {
        put_cell(bar_x + i, progress_row, i < filled ? '#' : '.', ATTR_NORMAL);
    }

    char text[16];
    number_text(percent, text);
    int length = 0;
    while (text[length] != '\0') {
        length++;
    }
    text[length++] = '%';
    text[length++] = ' ';
    text[length] = '\0';
    put_text(bar_x + bar_width + 2, progress_row, text, ATTR_NORMAL);
}

void factory_screen() {
    draw_menu(ACTION_FACTORY);
    int row = MENU_ROW + ACTION_COUNT + 1;

    if (!recovery::image_present()) {
        put_text(3, row, recovery::install_text(recovery::INSTALL_NO_IMAGE), ATTR_ERROR);
        put_text(3, row + 1, "Press any key to return.", ATTR_NORMAL);
        wait_key();
        return;
    }

    put_text(3, row, "This writes the bootloader and the kernel to the first disk.", ATTR_WARNING);
    put_text(3, row + 1, "Press Y to continue, any other key to cancel.", ATTR_NORMAL);

    if (wait_key() != KEY_YES) {
        return;
    }

    put_text(3, row + 3, "Installing, do not turn the power off...", ATTR_NORMAL);
    progress_row = row + 4;
    int result = recovery::install(draw_progress);

    put_text(3, row + 6, recovery::install_text(result), result == recovery::INSTALL_OK ? ATTR_OK : ATTR_ERROR);
    put_text(3, row + 7, "Press any key to return.", ATTR_NORMAL);
    wait_key();
}

bool has_word(const char *text, const char *word) {
    int i = 0;
    while (text[i] != '\0') {
        while (text[i] == ' ') {
            i++;
        }
        int start = i;
        while (text[i] != '\0' && text[i] != ' ') {
            i++;
        }
        int length = i - start;
        int j = 0;
        while (j < length && word[j] != '\0' && text[start + j] == word[j]) {
            j++;
        }
        if (j == length && word[j] == '\0') {
            return true;
        }
    }
    return false;
}

}

extern "C" int recovery_requested(void) {
    if (multiboot_magic != MULTIBOOT_MAGIC || multiboot_info == 0) {
        return 0;
    }
    const Info *info = reinterpret_cast<const Info *>(multiboot_info);
    if (!(info->flags & INFO_FLAG_CMDLINE) || info->cmdline == 0) {
        return 0;
    }
    return has_word(reinterpret_cast<const char *>(info->cmdline), "recovery") ? 1 : 0;
}

extern "C" void recovery_main(void) {
    asm volatile("cli");
    hide_cursor();

    int selected = 0;
    draw_menu(selected);

    while (true) {
        Key key = wait_key();

        if (key == KEY_UP) {
            selected = (selected + ACTION_COUNT - 1) % ACTION_COUNT;
            draw_menu(selected);
        } else if (key == KEY_DOWN) {
            selected = (selected + 1) % ACTION_COUNT;
            draw_menu(selected);
        } else if (key == KEY_ENTER) {
            if (selected == ACTION_REBOOT) {
                recovery::reboot();
            } else if (selected == ACTION_POWER_OFF) {
                recovery::power_off();
            } else {
                factory_screen();
                draw_menu(selected);
            }
        }
    }
}
