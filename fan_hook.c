// Preloaded into the DJI glasses app by wtfos-modloader.
// Blocks the app's fan PWM writes (direct and via test_pwm) so fan_control owns the fan.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *redirect(const char *path) {
    return (path && strstr(path, "pwm2/duty_cycle")) ? "/dev/null" : path;
}

static mode_t get_mode(int flags, va_list ap) {
    return (flags & O_CREAT) ? (mode_t)va_arg(ap, int) : 0;
}

int open(const char *path, int flags, ...) {
    static int (*real)(const char *, int, ...);
    if (!real) real = dlsym(RTLD_NEXT, "open");
    va_list ap;
    va_start(ap, flags);
    mode_t mode = get_mode(flags, ap);
    va_end(ap);
    return real(redirect(path), flags, mode);
}

int openat(int dirfd, const char *path, int flags, ...) {
    static int (*real)(int, const char *, int, ...);
    if (!real) real = dlsym(RTLD_NEXT, "openat");
    va_list ap;
    va_start(ap, flags);
    mode_t mode = get_mode(flags, ap);
    va_end(ap);
    return real(dirfd, redirect(path), flags, mode);
}

// Fortified variants bionic uses when compiled with _FORTIFY_SOURCE
int __open_2(const char *path, int flags) {
    static int (*real)(const char *, int);
    if (!real) real = dlsym(RTLD_NEXT, "__open_2");
    return real(redirect(path), flags);
}

int __openat_2(int dirfd, const char *path, int flags) {
    static int (*real)(int, const char *, int);
    if (!real) real = dlsym(RTLD_NEXT, "__openat_2");
    return real(dirfd, redirect(path), flags);
}

FILE *fopen(const char *path, const char *mode) {
    static FILE *(*real)(const char *, const char *);
    if (!real) real = dlsym(RTLD_NEXT, "fopen");
    return real(redirect(path), mode);
}

// The app also sets the fan by spawning "/system/bin/test_pwm 2 ...", which runs
// outside this hook. Skip any test_pwm launch targeting channel 2.
static int is_fan_cmd(const char *cmd) {
    const char *p = cmd ? strstr(cmd, "test_pwm") : NULL;
    if (!p) return 0;
    p += strlen("test_pwm");
    while (*p == ' ') p++;
    return p[0] == '2' && (p[1] == ' ' || p[1] == '\0');
}

static int is_fan_argv(const char *path, char *const argv[]) {
    return path && strstr(path, "test_pwm") && argv && argv[1] && strcmp(argv[1], "2") == 0;
}

int system(const char *cmd) {
    static int (*real)(const char *);
    if (!real) real = dlsym(RTLD_NEXT, "system");
    return is_fan_cmd(cmd) ? 0 : real(cmd);
}

FILE *popen(const char *cmd, const char *type) {
    static FILE *(*real)(const char *, const char *);
    if (!real) real = dlsym(RTLD_NEXT, "popen");
    return real(is_fan_cmd(cmd) ? "true" : cmd, type);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
    static int (*real)(const char *, char *const[], char *const[]);
    if (!real) real = dlsym(RTLD_NEXT, "execve");
    if (is_fan_argv(path, argv)) _exit(0);
    return real(path, argv, envp);
}

int execv(const char *path, char *const argv[]) {
    static int (*real)(const char *, char *const[]);
    if (!real) real = dlsym(RTLD_NEXT, "execv");
    if (is_fan_argv(path, argv)) _exit(0);
    return real(path, argv);
}

int execvp(const char *file, char *const argv[]) {
    static int (*real)(const char *, char *const[]);
    if (!real) real = dlsym(RTLD_NEXT, "execvp");
    if (is_fan_argv(file, argv)) _exit(0);
    return real(file, argv);
}
