#include "hc_test_getfile.h"

#ifdef __linux__
    #define UDISK_PATH1 "media/hdd/"
    #define UDISK_PATH2 "media/hdd1/"
#else
    #define UDISK_PATH1 "media/sda/"
    #define UDISK_PATH2 "media/sda1/"
#endif

static const char *read_usbdiskpath(void)
{
    DIR *dirp = NULL;
    if ((dirp = opendir(UDISK_PATH1)) == NULL) {
        if ((dirp = opendir(UDISK_PATH2)) != NULL) {
            return UDISK_PATH2;
        }
    } else {
        return UDISK_PATH1;
    }

    printf("The USB flash drive cannot be read.\n");
    return NULL;
}

static int is_assign_file(const char *filename, const char *target_extension)
{
    size_t len = strlen(filename);
    size_t ext_len = strlen(target_extension);
    return len >= ext_len && strcmp(filename + len - ext_len, target_extension) == 0;
}

static int read_directory_recursive(const char *path, const char *target_extension, char** userpath)
{
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path)) == NULL) {
        printf("opendir error.\n");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        int malloclength = strlen(path) + strlen(entry->d_name) + 2;
        char *full_path = NULL;
        full_path = (char *) malloc(malloclength);
        if (full_path == NULL) {
            printf("malloc error.\n");
            closedir(dir);
            return 0;
        }
        snprintf(full_path, malloclength, "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            int result = read_directory_recursive(full_path, target_extension, userpath);
            if (result) {
                free(full_path);
                closedir(dir);
                return 1;
            }
        } else {
            if (is_assign_file(entry->d_name, target_extension)) {
                *userpath = full_path;
                closedir(dir);
                return 1;
            } else if (strstr(full_path, target_extension) && strstr(target_extension, "/")) {
                *userpath = full_path;
                closedir(dir);
                return 1;
            }
        }

        free(full_path);
    }

    closedir(dir);

    return 0;
}

void hc_test_findfile(char **userpath, const char *filetype)
{
    const char *directory_path = read_usbdiskpath();
    if (!directory_path)
        return ;
    read_directory_recursive(directory_path, filetype, userpath);
}
