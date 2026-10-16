#include "../../include/bundle.h"
#include "../../include/fs.h"
#include "../drivers/vga.h"

extern const struct bundle_entry bundle_table[];

void bundle_install(void)
{
	const struct bundle_entry *entry;
	struct fs_file file;
	int result;

	for (entry = bundle_table; entry->name != 0; entry++) {
		if (fs_open(entry->name, &file, 0) != FS_ERR_NOENT) {
			continue;
		}

		result = fs_write_all(entry->name, entry->start, (unsigned int)(entry->end - entry->start));

		if (result != FS_OK) {
			print("\nbundle: ");
			print(entry->name);
			print(": ");
			print(fs_error_text(result));
			continue;
		}

		print("\ninstalled ");
		print(entry->name);
	}
}
