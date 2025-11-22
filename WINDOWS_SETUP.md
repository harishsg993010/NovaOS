# Windows Development Setup for NovaeOS

This guide will help you set up the development environment for NovaeOS on Windows.

## Prerequisites

You need to install the required build tools on Windows. There are two recommended approaches:

### Option A: WSL2 (Windows Subsystem for Linux) - RECOMMENDED

This is the easiest and most compatible approach.

#### 1. Install WSL2

Open PowerShell as Administrator and run:

```powershell
wsl --install -d Ubuntu
```

Restart your computer if prompted.

#### 2. Install Build Tools in WSL2

Open Ubuntu (from Start menu) and run:

```bash
sudo apt update
sudo apt install -y build-essential nasm xorriso grub-pc-bin \
    grub-common mtools git gdb qemu-system-x86
```

#### 3. Navigate to Project Directory

In WSL2, Windows drives are mounted under `/mnt/`:

```bash
cd /mnt/c/Users/haris/Desktop/personal/OS
```

#### 4. Build and Run

```bash
make all
make iso
make run
```

### Option B: MSYS2

If you prefer a native Windows toolchain:

#### 1. Install MSYS2

Download and install from: https://www.msys2.org/

#### 2. Install Build Tools

Open MSYS2 MINGW64 terminal and run:

```bash
pacman -Syu
pacman -S base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-nasm \
    mingw-w64-x86_64-gdb make git
```

#### 3. Install GRUB (Required for creating bootable ISO)

This is tricky on Windows. You have two options:

**Option B1: Use WSL2 just for grub-mkrescue**
- Install WSL2 as described in Option A
- Install only GRUB tools: `sudo apt install grub-pc-bin grub-common xorriso`
- Use MSYS2 for compilation, WSL2 for ISO creation

**Option B2: Use pre-built GRUB**
- Download grub-mkrescue for Windows from: https://github.com/grub4dos/grub4dos/releases
- This is more complex and may have compatibility issues

#### 4. Build in MSYS2

```bash
cd /c/Users/haris/Desktop/personal/OS
make all
# For ISO creation, switch to WSL2 or use pre-built GRUB
```

### Option C: Docker (Alternative)

If you have Docker Desktop for Windows:

#### 1. Create Dockerfile

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential nasm xorriso grub-pc-bin \
    grub-common mtools git gdb qemu-system-x86

WORKDIR /workspace
```

#### 2. Build Docker Image

```powershell
docker build -t novae-build .
```

#### 3. Build in Docker

```powershell
docker run -v C:\Users\haris\Desktop\personal\OS:/workspace novae-build make all
docker run -v C:\Users\haris\Desktop\personal\OS:/workspace novae-build make iso
```

#### 4. Run QEMU (on Windows)

You'll need QEMU installed on Windows natively to run the OS.

## Installing QEMU on Windows

### Method 1: Official Binary

1. Download from: https://qemu.weilnetz.de/w64/
2. Install to default location (usually `C:\Program Files\qemu`)
3. Add to PATH:
   - Open System Properties → Environment Variables
   - Edit Path variable
   - Add `C:\Program Files\qemu`

### Method 2: Via WSL2

QEMU is already installed in WSL2 if you followed Option A above.

### Method 3: Via Chocolatey

```powershell
choco install qemu
```

## Verifying Installation

### WSL2 Verification

```bash
# In WSL2 terminal
nasm --version      # Should show NASM version
gcc --version       # Should show GCC version
ld --version        # Should show GNU ld version
grub-mkrescue --version
qemu-system-x86_64 --version
```

### Windows Verification

```powershell
# In PowerShell or Command Prompt
qemu-system-x86_64.exe --version
```

## Common Issues and Solutions

### Issue: "make: command not found"

**Solution**: You're not in WSL2 or MSYS2. Use one of those terminals, not Git Bash or Windows CMD.

### Issue: "grub-mkrescue: command not found"

**Solution**:
- WSL2: `sudo apt install grub-pc-bin grub-common xorriso`
- MSYS2: This is difficult. Use WSL2 for this step.

### Issue: "nasm: command not found"

**Solution**:
- WSL2: `sudo apt install nasm`
- MSYS2: `pacman -S mingw-w64-x86_64-nasm`

### Issue: Cannot access Windows files from WSL2

**Solution**: Windows drives are mounted at `/mnt/c/`, `/mnt/d/`, etc.

```bash
cd /mnt/c/Users/haris/Desktop/personal/OS
```

### Issue: QEMU window doesn't appear

**Solution**:
- Ensure QEMU is installed
- Try adding `-display sdl` or `-display gtk` to QEMU command
- On WSL2, you may need an X server like VcXsrv or enable WSLg (Windows 11)

### Issue: Build errors with "unknown target"

**Solution**: Make sure you're using a 64-bit capable toolchain. Check `gcc -v` output.

## Recommended Setup (Best Experience)

For the best development experience, we recommend:

1. **Use WSL2** for all build operations
2. **Install Visual Studio Code** on Windows
3. **Install "Remote - WSL" extension** in VS Code
4. **Open project from WSL2** in VS Code: `code /mnt/c/Users/haris/Desktop/personal/OS`

This gives you:
- Full Linux toolchain
- Windows GUI for VS Code
- Integrated terminal
- Git integration
- Debugging support

## Quick Start (After Setup)

Once everything is installed:

```bash
# In WSL2 terminal:
cd /mnt/c/Users/haris/Desktop/personal/OS

# Build everything
make all

# Create bootable ISO
make iso

# Run in QEMU
make run

# For debugging
make debug
# In another terminal:
gdb build/kernel.elf -ex 'target remote localhost:1234'
```

## Current Environment Detection

Your current setup appears to be:
- **Shell**: Git Bash on Windows (limited functionality)
- **Recommendation**: Install WSL2 for full functionality

## Next Steps

1. Install WSL2 (Option A above)
2. Install build tools in WSL2
3. Navigate to project directory
4. Run `make info` to verify everything is working
5. Build and run: `make all && make iso && make run`

---

For more information, see:
- Main README: [README.md](README.md)
- Architecture: [ARCHITECTURE.md](ARCHITECTURE.md)
- Roadmap: [ROADMAP.md](ROADMAP.md)
