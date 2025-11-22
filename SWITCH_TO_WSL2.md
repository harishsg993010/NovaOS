# Switching to WSL2 for Kernel Development

## Why Switch?

The issue you're encountering is that **Windows GCC produces PE/COFF object files** (Windows format), but we need **ELF object files** (Linux/Unix format) for a freestanding x86-64 kernel. WSL2 provides a true Linux environment that makes this seamless.

## Installation Steps

### Step 1: Install WSL2

Open **PowerShell as Administrator** and run:

```powershell
wsl --install -d Ubuntu
```

This will:
- Install WSL2
- Install Ubuntu
- Reboot (if needed)

### Step 2: Complete Ubuntu Setup

After reboot, Ubuntu will open automatically:
- Create a username
- Create a password
- Wait for installation to complete

### Step 3: Install Build Tools

In the Ubuntu terminal:

```bash
# Update package list
sudo apt update

# Install all build tools
sudo apt install -y build-essential nasm xorriso \
    grub-pc-bin grub-common mtools qemu-system-x86 gdb git
```

### Step 4: Navigate to Your Project

Your Windows files are accessible at `/mnt/c/`:

```bash
cd /mnt/c/Users/haris/Desktop/personal/OS
```

### Step 5: Build!

```bash
# Clean any Windows build artifacts
make clean

# Build the kernel (should work perfectly now!)
make all

# Create bootable ISO
make iso

# Create disk image
qemu-img create -f raw disk.img 10M

# Run the OS
make run
# Or with disk:
qemu-system-x86_64 -cdrom build/novae.iso -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk
```

## Quick Reference

```bash
# From Windows, open WSL2:
# 1. Press Win+R
# 2. Type: ubuntu
# 3. Press Enter

# In WSL2:
cd /mnt/c/Users/haris/Desktop/personal/OS
make clean && make all && make iso && make run
```

## Advantages of WSL2

✅ **Native ELF Support** - GCC produces correct format
✅ **Better Tools** - All Linux tools available
✅ **GRUB Works** - Can create bootable ISOs
✅ **Faster Builds** - Better filesystem performance
✅ **Easier Debugging** - GDB works perfectly
✅ **Community Support** - Most OS dev is done on Linux

## Alternative: Cross-Compiler in MSYS2

If you must use MSYS2, you need to install a cross-compiler:

```bash
# Warning: This is more complex and less supported
# Install cross-compilation toolchain
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils

# Then modify Makefile to use x86_64-elf-gcc
# (Not recommended - WSL2 is much easier)
```

## Troubleshooting WSL2

### "WSL2 not installed"
```powershell
# Enable WSL feature
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart

# Enable Virtual Machine Platform
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart

# Reboot, then:
wsl --install -d Ubuntu
```

### "Can't access Windows files"
Your `C:` drive is at `/mnt/c/` in WSL2.

### "Too slow"
Consider moving project to WSL2 filesystem:
```bash
cp -r /mnt/c/Users/haris/Desktop/personal/OS ~/OS
cd ~/OS
```

## Next Steps

After switching to WSL2:

1. **Verify build works**: `make clean && make all`
2. **Create ISO**: `make iso`
3. **Test in QEMU**: `make run`
4. **See all Phase 5 features working!**

## Time Estimate

- WSL2 installation: 5-10 minutes
- Build tools installation: 5 minutes
- First successful build: 1 minute
- **Total: ~15-20 minutes**

Much faster than troubleshooting MSYS2 PE/COFF issues! 😊

## Need Help?

If you encounter issues:
- Check WSL2 status: `wsl --status`
- Check Ubuntu version: `lsb_release -a`
- Update WSL: `wsl --update`
- Restart WSL: `wsl --shutdown` then reopen Ubuntu

---

**Recommendation**: Switch to WSL2 now. It's the standard for OS development and will make everything easier going forward.
