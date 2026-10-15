#include <stdlib.h>

#ifdef __ANDROID__
    #include <pwd.h>
#endif

#include "../../includes/common/sys_utils.h"

char *get_user(void) {
#ifdef _WIN64
    char *user = getenv("USERNAME");
#else
    #ifndef __ANDROID__
        char *user = getenv("USER");
    #else
        uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);

        char *user = pw->pw_name;
    #endif
#endif

    return (user) ? user : "Unknown";
}