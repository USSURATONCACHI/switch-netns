#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/capability.h>
#include "misc.h"
#include "panic.h"
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "assert.h"

#define PRODUCT_NAME "switch-netns2"

// Print
typedef struct {
    int namespace_fd;
    const char* netns_name;
    int argc;
    char* const* argv;
} LaunchParams;
static LaunchParams parse_launch_params(const char* program_name, int argc, char** argv);

// Mounting `/etc/netns/<name>/*` over `/etc/`.
static void bind_etc_files(const char* netns_name);
static int open_netns_etc_dir(const char* netns_name, const char* dir_path);
static void check_trusted(int fd, const char* path);

static void drop_capabilities(void);

// Just general utility functions.
static void check_capability(const char* program_name);
static char* get_executable_path(const char* program_name);
static void show_usage(const char* program_name);
static const char* get_setns_errno_description(int errno_number);
static bool is_valid_netns_name(const char* name);
static void show_setcap_fix_suggestion(const char* program_name);


int main(int argc, char** argv) {
    // `argv[0]` may be missing (`argc == 0`) if we were executed with an empty `argv`.
    const char* program_name = argc > 0 ? argv[0] : PRODUCT_NAME;

    if (argc <= 1) { // No CLI arguments
        show_usage(program_name);
        exit(1);
    }

    // Make user know if we miss hard-needed capability/capabilities.
    // `cap_sys_admin` - to do `setns()`, `unshare()` and `mount()` calls.
    check_capability(program_name);

    // Parse cmdline.
    LaunchParams params = parse_launch_params(program_name, argc, argv);

    // Switch network namespace.
    if (setns(params.namespace_fd, CLONE_NEWNET) != 0) {
        const char* setns_error_description = get_setns_errno_description(errno);
        perror("failed to set network namespace");

        if (setns_error_description)
            fprintf(stderr, "%s\n", setns_error_description);

        exit(3);
    }
    // File is opened with O_CLOEXEC, so it will be closed automatically.

    // Mount `/etc/netns/<name>/*` over `/etc/`, if there are any.
    bind_etc_files(params.netns_name);

    // The user-provided command must not get any of our privileges.
    drop_capabilities();

    // Execute user-provided command.
    execvp(params.argv[0], params.argv);

    // Handle error
    perror("execvpe failed");
    fprintf(stderr, "could not execute user program '%s'\n", params.argv[0]);
    exit(5);
}

static void show_usage(const char* program_name) {
    fprintf(stderr, PRODUCT_NAME " - run a command in a network namespace from `/run/netns/` without sudo, with `/etc/netns/<name>/*` mounted over `/etc/`.\n");
    fprintf(stderr, "Usage: %s my_ns -- <command> <args...>\n", program_name);
}

static LaunchParams parse_launch_params(const char* program_name, int argc,
                                        char** argv) {
    // Expected: `<program> <netns_name> -- <command> [args...]`
    if (argc < 3 || strcmp(argv[2], "--") != 0) {
        fprintf(stderr, "Expected `--` right after the network namespace name.\n\n");
        show_usage(program_name);
        exit(6);
    } else if (argc == 3) {
        fprintf(stderr, "No command provided.\n\n");
        show_usage(program_name);
        exit(2);
    }

    const char* netns_name = argv[1];
    if (!is_valid_netns_name(netns_name)) {
        fprintf(stderr, "Invalid network namespace name '%s'.\n", netns_name);
        fprintf(stderr, "It must be non-empty, shorter than %d characters, contain no `/`, and not be `.` or `..`.\n\n", NAME_MAX);
        show_usage(program_name);
        exit(7);
    }

    LaunchParams result = (LaunchParams){
        .namespace_fd = -1,
        .netns_name = netns_name,
        .argc = argc - 3,
        .argv = &argv[3],
    };

    size_t needed_len = strlen("/run/netns/%s") + strlen(netns_name) + 1;
    char* filepath = (char*)calloc(needed_len, sizeof(char));
    assert_alloc(filepath);
    snprintf(filepath, needed_len - 1, "/run/netns/%s", netns_name);

    // We do not want to pass that file descriptor to any child:
    //      The launched command has no business with it.
    result.namespace_fd = open(filepath, O_RDONLY | O_CLOEXEC);
    if (result.namespace_fd < 0) {
        perror("open failed; could not open namespace file");
        fprintf(stderr, "Could not open namespace file '%s'.\n", filepath);
        fprintf(stderr, "Check that the network namespace exists: `ls /run/netns/`.\n\n");
        free(filepath);
        exit(8);
    }
    free(filepath);

    return result;
}

static void check_capability(const char* program_name) {
    const cap_value_t required_cap = CAP_SYS_ADMIN;

    cap_t capabilities = cap_get_proc();
    if (!capabilities) {
        perror("cap_get_proc failed; Could not check capabilities of a current process.");
        return; // We do not need to exit. We will just fail on the operation that requires the capability.
    }

    cap_flag_value_t flag;
    if (cap_get_flag(capabilities, required_cap, CAP_EFFECTIVE, &flag) != 0) {
        perror("cap_get_flag failed");
        fprintf(stderr, "Could not check `%s` capability of a runnning process. The operation may fail.", cap_to_name(required_cap));
    } else if (flag != CAP_SET) {
        fprintf(stderr, "Missing capability `%s`. The operation will fail.\n\n", cap_to_name(required_cap));
        show_setcap_fix_suggestion(program_name);
    }
    cap_free(capabilities);
}

// Same convention as `ip netns exec`: every entry of `/etc/netns/<netns_name>/` is
// bind-mounted over `/etc/<entry>`, visible only to the launched command.
//
// We do this with `cap_sys_admin` on behalf of any user, so only root-controlled files
// may be mounted. Otherwise a user could mount their own file over e.g. `/etc/sudoers`.
static void bind_etc_files(const char* netns_name) {
    char dir_path[PATH_MAX];
    snprintf(dir_path, sizeof(dir_path), "/etc/netns/%s", netns_name);

    // Nothing to mount: keep the current mount namespace.
    int dir_fd = open_netns_etc_dir(netns_name, dir_path);
    if (dir_fd < 0) return;
    close(dir_fd);

    // Private copy of the mount table: our mounts will only be visible to the launched command.
    if (unshare(CLONE_NEWNS) != 0) {
        perror("unshare failed; could not create a mount namespace");
        exit(9);
    }
    // `/` is usually a shared mount, which would propagate our mounts back to the host.
    // As a slave, it still receives new mounts from the host, but ours never leak out.
    if (mount("", "/", NULL, MS_SLAVE | MS_REC, NULL) != 0) {
        perror("mount failed; could not make `/` a slave mount");
        exit(9);
    }

    // Reopen in the new mount namespace: files reached through a file descriptor from
    // the old one cannot be bind-mounted here (EINVAL).
    dir_fd = open_netns_etc_dir(netns_name, dir_path);
    if (dir_fd < 0) {
        fprintf(stderr, "Directory '%s' disappeared while preparing the mount namespace.\n", dir_path);
        exit(9);
    }

    DIR* dir = fdopendir(dir_fd);
    if (dir == NULL) {
        perror("fdopendir failed");
        fprintf(stderr, "Could not read directory '%s'.\n", dir_path);
        exit(9);
    }

    for (;;) {
        errno = 0;
        struct dirent* entry = readdir(dir);
        if (entry == NULL) {
            if (errno != 0) {
                perror("readdir failed");
                fprintf(stderr, "Could not read directory '%s'.\n", dir_path);
                exit(9);
            }
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char src_path[PATH_MAX + NAME_MAX + 2];  // For messages only.
        char dst_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", dir_path, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "/etc/%s", entry->d_name);

        // Symlinks are followed: their target is what gets checked and mounted.
        // Mounting through the same file descriptor guarantees that the checked file is the mounted one.
        int src_fd = openat(dirfd(dir), entry->d_name, O_PATH | O_CLOEXEC);
        if (src_fd < 0) {
            perror("openat failed");
            fprintf(stderr, "Could not open '%s'.\n", src_path);
            exit(9);
        }
        check_trusted(src_fd, src_path);

        char src_fd_path[64];
        snprintf(src_fd_path, sizeof(src_fd_path), "/proc/self/fd/%d", src_fd);
        if (mount(src_fd_path, dst_path, NULL, MS_BIND, NULL) != 0) {
            int mount_errno = errno;
            perror("mount failed");
            fprintf(stderr, "Could not bind-mount '%s' over '%s'.\n", src_path, dst_path);
            if (mount_errno == ENOENT)
                fprintf(stderr, "'%s' must already exist: only existing files in `/etc/` can be replaced.\n", dst_path);
            exit(9);
        }
        close(src_fd);
    }
    closedir(dir);
}

// Opens `/etc/netns/<netns_name>` and makes sure it is controlled by root.
// Returns -1 if it does not exist.
static int open_netns_etc_dir(const char* netns_name, const char* dir_path) {
    int netns_etc_fd = open("/etc/netns", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (netns_etc_fd < 0) {
        if (errno == ENOENT) return -1;
        perror("open failed; could not open directory `/etc/netns`");
        exit(9);
    }
    check_trusted(netns_etc_fd, "/etc/netns");

    int dir_fd = openat(netns_etc_fd, netns_name, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    int open_errno = errno;
    close(netns_etc_fd);
    if (dir_fd < 0) {
        if (open_errno == ENOENT) return -1;
        errno = open_errno;
        perror("openat failed");
        fprintf(stderr, "Could not open directory '%s'.\n", dir_path);
        exit(9);
    }
    check_trusted(dir_fd, dir_path);
    return dir_fd;
}

// Makes sure that only root controls the file: owned by root and not writable by group/others.
static void check_trusted(int fd, const char* path) {
    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat failed");
        fprintf(stderr, "Could not check owner and permissions of '%s'.\n", path);
        exit(9);
    }
    if (st.st_uid != 0 || (st.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
        fprintf(stderr, "Refusing to use '%s': it must be owned by root and must not be writable by group or others.\n", path);
        fprintf(stderr, "`" PRODUCT_NAME "` mounts these files over `/etc/` for any user, so whoever can change them could gain root privileges.\n");
        exit(9);
    }
}

// `execvp()` would drop our file capabilities anyway for regular executables,
// but drop everything explicitly: the launched command must never get `cap_sys_admin`.
static void drop_capabilities(void) {
    cap_t no_capabilities = cap_init();
    assert_alloc(no_capabilities);

    if (cap_set_proc(no_capabilities) != 0) {
        perror("cap_set_proc failed; could not drop capabilities before running the command");
        exit(10);
    }
    cap_free(no_capabilities);
}


static char* get_executable_path(const char* program_name) {
    char exe_link_path[1024];
    snprintf(exe_link_path, sizeof(exe_link_path), "/proc/%llu/exe",
             (long long unsigned int)getpid());

    char exe_path[8192];
    memset(exe_path, 0, sizeof(exe_path));  // zero out.
    if (readlink(exe_link_path, exe_path, sizeof(exe_path) - 1) < 0) {
        perror("could not get current executable path; falling back to program name");
        char* string = strdup(program_name);
        if (string == NULL && program_name != NULL)
            panic("Failed to allocate memory");

        return string;
    } else {
        char* string = strdup(exe_path);
        if (string == NULL) panic("Failed to allocate memory");

        return string;
    }
}

// See `$ man 2 setns`, part `ERRORS` for more info.
static const char* get_setns_errno_description(int errno_number) {
    switch (errno_number) {
        case EBADF:
            return "[EBADF] Provided file descriptor is invalid.";
        case EINVAL:
            return "[EINVAL] One of the following problems occured:\n"
                   "\t- Namespace file (in `/run/netns/`) refers to a non-network namespace;\n"
                   "\t- There is a problem with reassociating the thread with the specified namespace.";
        case ENOMEM:
            return "[ENOMEM] Cannot allocate sufficient memory to change the specified namespace.";
        case EPERM:
            return "[EPERM] Current process does not have the required capability (`cap_sys_admin`) for this operation.";
    }

    return NULL;
}

// Same rules as `ip netns`. Most importantly, no `/`: the name must not escape `/run/netns/` or `/etc/netns/`.
static bool is_valid_netns_name(const char* name) {
    size_t len = strlen(name);
    return len > 0 && len < NAME_MAX && strchr(name, '/') == NULL &&
           strcmp(name, ".") != 0 && strcmp(name, "..") != 0;
}

static void show_setcap_fix_suggestion(const char* program_name) {
    char* exe_path = get_executable_path(program_name);

    fprintf(stderr, "Note that `" PRODUCT_NAME "` by default comes with capabilities pre-set. ");
    fprintf(stderr, "Lack of privilige(s) means either invalid/broken/custom installation, messing around with executable, or malicious intent. ");
    fprintf(stderr, "If you are the system administrator, you can fix the problem via:\n`$ sudo setcap cap_sys_admin=ep %s`.\n",exe_path);
    fprintf(stderr, "- `cap_sys_admin` - is required to change a namespace and to mount `/etc/netns/<name>/*` files.\n\n");

    fprintf(stderr, "Alternatively, just run the program as root (via `sudo`) to gain privileges directly.\n\n");

    free(exe_path);
}
