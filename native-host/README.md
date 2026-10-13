# Buna ARM build proof

The TV has an aarch64 kernel with a **32-bit ARM userland**. The locally studied
reference executable and TV libc/SDL libraries use ARM EABI version 5 with the
soft-float procedure-call standard (`e_flags = 0x05000200`). The executable
loader is `/lib/ld-linux.so.3`; the TV reports glibc 2.35.

On 13 October 2026, `abi-probe.c` was cross-compiled on Windows with Zig 0.15.2:

```powershell
$zigExe = 'research/tools/zig-0.15.2/zig-x86_64-windows-0.15.2/zig.exe'
& $zigExe cc -target arm-linux-gnueabi.2.35 -O2 -o native-host/build/buna-abi-probe native-host/abi-probe.c
```

The resulting ELF is 32-bit ARM with the same `e_flags` and loader path as the
reference. It was copied to `/tmp/buna-abi-probe` on the TV and returned exit 0
with `Buna ARM ABI probe: loaded`. This confirms **build and load compatibility
for a simple C executable**, not graphics, remote input or media integration.

The Zig archive came from the [official Zig 0.15.2 release](https://ziglang.org/download/)
and its SHA-256 matched the official release metadata:
`3a0ed1e8799a2f8ce2a6e6290a9ff22e6906f8227865911fb7ddedc3cc14cb0c`.
The compiler, archive and compiled binary stay in ignored local directories;
only the source and instructions are committed. The next native-host step is
an original Wayland/EGL window that receives remote input and renders a frame.
No existing TV apps or files were removed.
