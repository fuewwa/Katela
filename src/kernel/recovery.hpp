#ifndef RECOVERY_HPP
#define RECOVERY_HPP

namespace recovery {

enum InstallResult {
    INSTALL_OK = 0,
    INSTALL_NO_IMAGE,
    INSTALL_BAD_IMAGE,
    INSTALL_NO_DISK,
    INSTALL_DISK_TOO_SMALL,
    INSTALL_WRITE_FAILED,
    INSTALL_VERIFY_FAILED
};

typedef void (*ProgressFn)(unsigned done, unsigned total);

void reboot();
void power_off();
bool image_present();
int install(ProgressFn progress);
const char *install_text(int result);

}

#endif
