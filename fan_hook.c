// Preloaded into the DJI glasses app by wtfos-modloader.
// Redirects the app's fan PWM duty_cycle writes to /dev/null so fan_control owns the fan.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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
