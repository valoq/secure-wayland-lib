#ifndef WAYLAND_SECURITY_CONTEXT_H
#define WAYLAND_SECURITY_CONTEXT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    pid_t helper_pid;
    int pipe_fd;
    char socket_path[256];
} HelperInfo;

// Starts the Wayland security context helper as a child process.
// Sets up communication using a pipe.
// Returns 0 on success, -1 on error.
// Populates HelperInfo struct.
int setup_wayland_security_context_helper(HelperInfo *info);

// Cleans up the helper process, waits for exit and removes the socket file.
int cleanup_wayland_security_context_helper(const HelperInfo *info);

#ifdef __cplusplus
}
#endif

#endif // WAYLAND_SECURITY_CONTEXT_H