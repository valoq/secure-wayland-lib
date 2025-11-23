# Wayland Secure Context Library

This library sets up a secure Wayland context for graphical applications.  
It provides a helper process to manage the context and socket, and cleans up after the application exits.

Code partially forked from https://github.com/qwertviop/run_with_wayland_security_context_v1

## Features

- Socket created under `${XDG_RUNTIME_DIR}/wayland-secure-context/` (directory auto-created)
- Pipe-based background helper process
- API for setup and cleanup
- GTK4 Hello World demo

## Usage

1. Build with `make`
2. Run the demo: `./gtk4_hello`

## API

See `wayland_security_context.h` for usage.

## License

MIT
