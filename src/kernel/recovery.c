#include "../../include/recovery.h"
#include "../../include/io.h"

#define MULTIBOOT_MAGIC 0x2BADB002
#define INFO_FLAG_CMDLINE (1u << 2)

#define WIDTH 80
#define HEIGHT 25
#define MENU_ROW 3

#define ATTR_NORMAL 0x07
#define ATTR_SELECTED 0x70
#define ATTR_TITLE 0x1F
#define ATTR_WARNING 0x0E
#define ATTR_OK 0x0A
#define ATTR_ERROR 0x0C

#define KEY_NONE 0
#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_ENTER 3
#define KEY_YES 4
#define KEY_OTHER 5

#define ACTION_REBOOT 0
#define ACTION_FACTORY 1
#define ACTION_POWER_OFF 2
#define ACTION_COUNT 3

extern unsigned int multiboot_magic;
extern unsigned int multiboot_info;

struct info {
	unsigned int flags;
	unsigned int mem_lower;
	unsigned int mem_upper;
	unsigned int boot_device;
	unsigned int cmdline;
};

static const char *const items[ACTION_COUNT] = {
	"Reboot system now",
	"Reboot bootloader to factory conf",
	"Power off"
};

static volatile unsigned short *const video = (volatile unsigned short *)0xB8000;
static int progress_row;

static void put_cell(int x, int y, char c, unsigned char attr) {
	video[y * WIDTH + x] = (unsigned short)((attr << 8) | (unsigned char)c);
}

static void fill_row(int y, unsigned char attr) {
	for (int x = 0; x < WIDTH; x++) {
		put_cell(x, y, ' ', attr);
	}
}

static void put_text(int x, int y, const char *text, unsigned char attr) {
	for (int i = 0; text[i] != '\0' && x + i < WIDTH; i++) {
		put_cell(x + i, y, text[i], attr);
	}
}

static void clear_screen(void) {
	for (int y = 0; y < HEIGHT; y++) {
		fill_row(y, ATTR_NORMAL);
	}
}

static void hide_cursor(void) {
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

static int number_text(unsigned int value, char *out) {
	char digits[11];
	int count = 0;

	do {
		digits[count++] = (char)('0' + value % 10);
		value /= 10;
	} while (value != 0);

	for (int i = 0; i < count; i++) {
		out[i] = digits[count - 1 - i];
	}
	out[count] = '\0';
	return count;
}

static int read_key(void) {
	while (1) {
		unsigned char status = inb(0x64);

		if (!(status & 1)) {
			return KEY_NONE;
		}

		unsigned char code = inb(0x60);

		if ((status & 0x20) || code == 0xE0 || (code & 0x80)) {
			continue;
		}

		switch (code) {
			case 0x48: return KEY_UP;
			case 0x50: return KEY_DOWN;
			case 0x1C: return KEY_ENTER;
			case 0x15: return KEY_YES;
			default: return KEY_OTHER;
		}
	}
}

static int wait_key(void) {
	while (1) {
		int key = read_key();

		if (key != KEY_NONE) {
			return key;
		}
		asm volatile("pause");
	}
}

static void draw_menu(int selected) {
	clear_screen();
	fill_row(0, ATTR_TITLE);
	put_text(1, 0, "Kernel recovery mode", ATTR_TITLE);

	for (int i = 0; i < ACTION_COUNT; i++) {
		unsigned char attr = (i == selected) ? ATTR_SELECTED : ATTR_NORMAL;

		for (int x = 2; x < WIDTH - 2; x++) {
			put_cell(x, MENU_ROW + i, ' ', attr);
		}
		put_text(3, MENU_ROW + i, items[i], attr);
	}

	put_text(2, HEIGHT - 2, "Use the Up and Down keys to select an entry, Enter to run it.", ATTR_NORMAL);
}

static void draw_progress(unsigned int done, unsigned int total) {
	unsigned int percent = total == 0 ? 0 : done * 100 / total;
	int bar_x = 3;
	int bar_width = 50;
	int filled = (int)(percent * bar_width / 100);
	char text[16];

	for (int i = 0; i < bar_width; i++) {
		put_cell(bar_x + i, progress_row, i < filled ? '#' : '.', ATTR_NORMAL);
	}

	int length = number_text(percent, text);

	text[length++] = '%';
	text[length++] = ' ';
	text[length] = '\0';
	put_text(bar_x + bar_width + 2, progress_row, text, ATTR_NORMAL);
}

static void factory_screen(void) {
	int row = MENU_ROW + ACTION_COUNT + 1;

	draw_menu(ACTION_FACTORY);

	if (!recovery_image_present()) {
		put_text(3, row, recovery_install_text(INSTALL_NO_IMAGE), ATTR_ERROR);
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

	int result = recovery_install(draw_progress);

	put_text(3, row + 6, recovery_install_text(result), result == INSTALL_OK ? ATTR_OK : ATTR_ERROR);
	put_text(3, row + 7, "Press any key to return.", ATTR_NORMAL);
	wait_key();
}

static int has_word(const char *text, const char *word) {
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
			return 1;
		}
	}
	return 0;
}

int recovery_requested(void) {
	if (multiboot_magic != MULTIBOOT_MAGIC || multiboot_info == 0) {
		return 0;
	}

	const struct info *info = (const struct info *)multiboot_info;

	if (!(info->flags & INFO_FLAG_CMDLINE) || info->cmdline == 0) {
		return 0;
	}
	return has_word((const char *)info->cmdline, "recovery");
}

void recovery_main(void) {
	int selected = 0;

	asm volatile("cli");
	hide_cursor();
	draw_menu(selected);

	while (1) {
		int key = wait_key();

		if (key == KEY_UP) {
			selected = (selected + ACTION_COUNT - 1) % ACTION_COUNT;
			draw_menu(selected);
		} else if (key == KEY_DOWN) {
			selected = (selected + 1) % ACTION_COUNT;
			draw_menu(selected);
		} else if (key == KEY_ENTER) {
			if (selected == ACTION_REBOOT) {
				recovery_reboot();
			} else if (selected == ACTION_POWER_OFF) {
				recovery_power_off();
			} else {
				factory_screen();
				draw_menu(selected);
			}
		}
	}
}
