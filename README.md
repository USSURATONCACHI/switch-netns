# switch-netns2

Simple & secure C utility to change network namespaces without being root.
A wrapper around `setns()` syscall, with permission checks and command line parsing.
Like `ip netns exec`, it also mounts files from `/etc/netns/<name>/` over `/etc/` (e.g. a custom `resolv.conf`), visible only to the launched command.

Usage:
```sh
$ whoami
ussur
$ switch-netns2 my_netns -- whoami
ussur
$ switch-netns2 my_netns -- echo 'Hello from other network namespace!'
Hello from other network namespace!
```

The namespace is `/run/netns/<name>` (as created by `ip netns add <name>`).
Files in `/etc/netns/<name>/` must be owned by root and not writable by group/others, otherwise they are refused.

The previous version (`switch-netns`, with `--by-file`, `--by-pid` and `--by-fd`) is tagged `v1.0`.

## Build and install

### Via AUR:
```sh
yay -S switch-netns2
```

### Via PKGBUILD:
```sh
$ makepkg -si
```

### Manually:

Installation:
```sh
$ cc -O3 -o switch-netns2 main.c -lcap
$ sudo install -Dm755 switch-netns2 /usr/bin/switch-netns2
$ sudo setcap cap_sys_admin=ep /usr/bin/switch-netns2
```

Uninstallation:
```sh
$ sudo rm /usr/bin/switch-netns2
```

### Dependencies

- `libcap`,
- a C compiler.
