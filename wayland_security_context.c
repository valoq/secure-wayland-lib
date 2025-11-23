#define _GNU_SOURCE
#include "wayland_security_context.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "security-context-client-protocol.h"

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                                   const char *interface, uint32_t version)
{
    if (strcmp(wp_security_context_manager_v1_interface.name, interface) == 0) {
        *((struct wp_security_context_manager_v1 **)data) = wl_registry_bind(
            registry, name, &wp_security_context_manager_v1_interface, 1);
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    // No-op
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

static int create_wl_socket(char *socket_path)
{
    char *socket_dir = getenv("XDG_RUNTIME_DIR");
    if (!socket_dir) return -1;

    // Compose the secure context directory path
    char secure_dir[256];
    snprintf(secure_dir, sizeof(secure_dir), "%s/wayland-secure-context", socket_dir);

    // Create the directory if it does not exist
    struct stat st = {0};
    if (stat(secure_dir, &st) == -1) {
        if (mkdir(secure_dir, 0700) == -1) {
            perror("Failed to create wayland-secure-context directory");
            return -1;
        }
    }
    sleep(5);
    // Now create the socket in the secure directory
    snprintf(socket_path, 256, "%s/wayland-XXXXXX", secure_dir);
    int ret = mkstemp(socket_path);
    if (ret == -1) return -1;
    close(ret);
    unlink(socket_path);

    int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) return -1;
    int flags = fcntl(listen_fd, F_GETFD);
    fcntl(listen_fd, F_SETFD, flags | FD_CLOEXEC);

    struct sockaddr_un sockaddr = { .sun_family = AF_UNIX };
    snprintf(sockaddr.sun_path, sizeof(sockaddr.sun_path), "%s", socket_path);

    if (bind(listen_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != 0) {
        close(listen_fd); return -1;
    }
    if (listen(listen_fd, 0) != 0) {
        close(listen_fd); return -1;
    }
    return listen_fd;
}

int setup_wayland_security_context_helper(HelperInfo *info)
{
    int pipefd[2];
    int info_pipe[2];
    if (pipe(pipefd) < 0) return -1;
    if (pipe(info_pipe) < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        close(info_pipe[0]);
        close(info_pipe[1]);
        return -1;
    }

    if (pid == 0) {
        // Child: run the blocking helper
        close(pipefd[1]); // Close unused write end
        close(info_pipe[0]); // Child writes socket_path to info_pipe[1]

        struct wl_display *display;
        struct wl_registry *registry;
        struct wp_security_context_manager_v1 *security_context_manager = NULL;
        struct wp_security_context_v1 *security_context;
        int listen_fd = -1, ret;
        char socket_path[256];

        display = wl_display_connect(NULL);
        if (!display) exit(EXIT_FAILURE);

        registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, &security_context_manager);
        ret = wl_display_roundtrip(display);
        wl_registry_destroy(registry);
        if (ret == -1 || !security_context_manager) exit(EXIT_FAILURE);

        listen_fd = create_wl_socket(socket_path);
        if (listen_fd == -1) exit(EXIT_FAILURE);

        int fds[2];
        if (pipe2(fds, O_CLOEXEC) < 0) exit(EXIT_FAILURE);

        security_context = wp_security_context_manager_v1_create_listener(security_context_manager,
                                                                         listen_fd, fds[1]);
        wp_security_context_v1_commit(security_context);
        wp_security_context_v1_destroy(security_context);
        wl_display_roundtrip(display);

        setenv("WAYLAND_DISPLAY", basename(socket_path), 1);
        unsetenv("WAYLAND_SOCKET");

        close(fds[0]);
        close(listen_fd);

        // Send socket_path to parent
        write(info_pipe[1], socket_path, sizeof(socket_path));
        close(info_pipe[1]);

        // Block until parent sends shutdown signal
        char buf;
        read(pipefd[0], &buf, 1);

        // Cleanup
        if (security_context_manager) wp_security_context_manager_v1_destroy(security_context_manager);
        wl_display_disconnect(display);

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    }

    // Parent
    close(pipefd[0]); // Close unused read end
    close(info_pipe[1]); // Parent reads socket_path from info_pipe[0]

    info->helper_pid = pid;
    info->pipe_fd = pipefd[1];

    // Read socket path from child
    ssize_t n = read(info_pipe[0], info->socket_path, sizeof(info->socket_path));
    close(info_pipe[0]);
    if (n <= 0) {
        info->socket_path[0] = '\0';
        return -1;
    }
    info->socket_path[sizeof(info->socket_path)-1] = '\0'; // ensure null termination

    return 0;
}

int cleanup_wayland_security_context_helper(const HelperInfo *info)
{
    // Signal helper to terminate
    char x = 'X';
    write(info->pipe_fd, &x, 1);
    close(info->pipe_fd);

    // Wait for helper to exit
    int status = 0;
    waitpid(info->helper_pid, &status, 0);

    // Remove the socket file
    if (info->socket_path[0]) {
        unlink(info->socket_path);
    }

    return (status == 0) ? 0 : -1;
}
