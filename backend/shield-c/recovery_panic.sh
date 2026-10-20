#!/bin/bash
# Interlock Assessment IDE - Emergency Kernel Recovery
# Usage: boot into Recovery Mode -> Terminal -> sh this_script.sh

echo "[!] Interlock Shield: Emergency Recovery Initiated..."

# 1. Mount the main volume
DISK_NAME=$(ls /Volumes | grep -v "Recovery" | grep -v "External" | head -n 1)
TARGET_VOL="/Volumes/${DISK_NAME}"

echo "[*] Target Volume: ${TARGET_VOL}"

# 2. Path to the KEXT
KEXT_PATH="${TARGET_VOL}/Library/Extensions/InterlockShield.kext"

if [ -d "$KEXT_PATH" ]; then
    echo "[+] Malformed KEXT detected. Deleting..."
    rm -rf "$KEXT_PATH"

    echo "[+] Clearing Kernel Staging Cache..."
    # kmutil is the modern way to manage kernel collections in Big Sur+
    kmutil clear-staging --volume-root "${TARGET_VOL}"

    echo "[SUCCESS] KEXT removed. You can now reboot safely."
else
    echo "[-] InterlockShield.kext was not found in the system extensions."
    echo "Check if it was loaded manually via kextload (which doesn't persist after reboot)."
fi
