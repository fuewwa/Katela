#ifndef BUNDLE_H
#define BUNDLE_H

struct bundle_entry {
	const char *name;
	const char *start;
	const char *end;
};

void bundle_install(void);

#endif
