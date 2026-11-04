#ifndef RECOVERY_H
#define RECOVERY_H

#define INSTALL_OK 0
#define INSTALL_NO_IMAGE 1
#define INSTALL_BAD_IMAGE 2
#define INSTALL_NO_DISK 3
#define INSTALL_DISK_TOO_SMALL 4
#define INSTALL_WRITE_FAILED 5
#define INSTALL_VERIFY_FAILED 6

int recovery_requested(void);
void recovery_main(void);

void recovery_reboot(void);
void recovery_power_off(void);

int recovery_image_present(void);
int recovery_install(void (*progress)(unsigned int done, unsigned int total));
int recovery_erase_fs(void);
const char *recovery_install_text(int result);

#endif
