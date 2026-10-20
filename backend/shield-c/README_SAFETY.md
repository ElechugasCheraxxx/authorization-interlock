# Kernel Safety & Recovery Protocol (Interlock Shield)

To avoid "bricking" your Mac or entering an infinite Kernel Panic boot loop, follow these protocols strictly.

## 1. Emergency Recovery Script
If the system fails to boot, enter **Recovery Mode** (Hold Power button on Apple Silicon), open Terminal, and run:

```bash
sh /path/to/project/backend/kernel/recovery_panic.sh
```

## 2. Recovery Script Content (`recovery_panic.sh`)
```bash
#!/bin/bash
# Emergency Kernel Extension Removal
echo "[!] Emergency Recovery Initiated..."

# 1. Identify System Volume
DISK=$(ls /Volumes | grep "Macintosh HD" | head -n 1)
TARGET="/Volumes/${DISK}/Library/Extensions/InterlockShield.kext"

if [ -d "$TARGET" ]; then
    echo "[+] Found malformed KEXT. Removing..."
    rm -rf "$TARGET"
    echo "[+] KEXT removed. Clearing Kernel Cache..."
    kmutil clear-staging
    echo "[+] Done. You can now reboot safely."
else
    echo "[-] InterlockShield.kext not found in system extensions."
fi
```

## 3. Safe Development Rules
1. **NEVER** copy the `.kext` to `/Library/Extensions` during development.
2. **USE** `sudo kextload` manually from the build directory.
3. **VM FIRST**: Test all memory-shuffling logic in a macOS VM (Tart/Viable) before running on bare metal.
4. **LIBKERN ONLY**: No `std::` namespace. No exceptions. No RTTI.

## 4. Compilation Guard
The `CMakeLists.txt` in this directory is configured with:
- `-fno-exceptions`: Prevents kernel panics from uncaught C++ exceptions.
- `-fno-rtti`: Disables runtime type information (not supported in XNU).
- `-nostdinc++`: Ensures you don't accidentally link the standard C++ library.
