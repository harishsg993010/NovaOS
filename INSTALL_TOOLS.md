# Installing Build Tools for NovaeOS (MSYS2)

## Quick Installation

Open your MSYS2 terminal and run these commands:

### Step 1: Update MSYS2
```bash
pacman -Syu
```

**Note**: If MSYS2 closes after this, reopen it and run:
```bash
pacman -Su
```

### Step 2: Install Build Tools
```bash
pacman -S --needed base-devel mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-nasm mingw-w64-x86_64-gdb make git
```

Press Enter to accept all defaults when prompted.

### Step 3: Install QEMU
```bash
pacman -S mingw-w64-x86_64-qemu
```

### Step 4: Verify Installation
```bash
gcc --version
nasm -v
make --version
qemu-system-x86_64 --version
```

## Alternative: Install Manually

If the above doesn't work, install each package separately:

```bash
# Core build tools
pacman -S base-devel

# GCC compiler
pacman -S mingw-w64-x86_64-gcc

# NASM assembler
pacman -S mingw-w64-x86_64-nasm

# Make
pacman -S make

# GDB debugger
pacman -S mingw-w64-x86_64-gdb

# QEMU emulator
pacman -S mingw-w64-x86_64-qemu

# Git (if not already installed)
pacman -S git
```

## Note About GRUB

GRUB (grub-mkrescue) is not easily available on MSYS2. You have two options:

### Option 1: Use WSL2 for ISO Creation (Recommended)
1. Install WSL2: `wsl --install -d Ubuntu` (in PowerShell as Admin)
2. In WSL2, install GRUB: `sudo apt install grub-pc-bin grub-common xorriso`
3. Build the kernel in MSYS2
4. Create ISO in WSL2

### Option 2: Use Pre-built ISO Tools
Download GRUB utilities for Windows from third-party sources (not recommended).

### Option 3: Skip ISO Creation
Run the kernel directly with QEMU using the ELF file:
```bash
qemu-system-x86_64 -kernel build/kernel.elf
```

## Quick Test

After installation, test that everything works:

```bash
cd /c/Users/haris/Desktop/personal/OS
make clean
make all
```

If successful, you should see:
- Assembly files being compiled
- C files being compiled
- Kernel being linked
- "Build complete!" message

## Troubleshooting

### "pacman: command not found"
Make sure you're running MSYS2 MINGW64 (not MSYS2 MSYS).

### "gcc: No such file or directory"
The toolchain might be installed in a different path. Try:
```bash
export PATH="/mingw64/bin:$PATH"
```

### Slow package installation
This is normal for first-time setup. Be patient.

### Missing dependencies
If build fails with missing dependencies:
```bash
pacman -S mingw-w64-x86_64-headers
```

## Next Steps

After installation:
1. Navigate to project: `cd /c/Users/haris/Desktop/personal/OS`
2. Build kernel: `make clean && make all`
3. Create disk image: `qemu-img create -f raw disk.img 10M`
4. Run kernel: See BUILD_AND_TEST.md
