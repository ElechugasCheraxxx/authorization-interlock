#!/bin/bash
# Interlock Assessment IDE - KEXT Manager (Backup & Load)
# Task 0.3 of KERNEL_ROADMAP.xml

KEXT_NAME="InterlockShield.kext"
BUILD_PATH="./build/Debug/$KEXT_NAME"
BACKUP_DIR="$HOME/interlock_kext_backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "[*] Interlock KEXT Manager starting..."

# 1. Create backup directory if it doesn't exist
mkdir -p "$BACKUP_DIR"

# 2. Function to backup
do_backup() {
    if [ -d "$BUILD_PATH" ]; then
        echo "[+] Backing up current build to $BACKUP_DIR/backup_$TIMESTAMP"
        cp -R "$BUILD_PATH" "$BACKUP_DIR/backup_$TIMESTAMP.kext"
    fi
}

# 3. Function to load
load_kext() {
    echo "[*] Attempting to load $KEXT_NAME..."
    sudo chown -R root:wheel "$BUILD_PATH"
    sudo chmod -R 755 "$BUILD_PATH"

    # Pre-flight check: is SIP off?
    csrutil status | grep -q "disabled"
    if [ $? -ne 0 ]; then
        echo "[!] WARNING: SIP is ENABLED. Loading unsigned KEXTs will fail."
    fi

    sudo kextload "$BUILD_PATH"
    if [ $? -eq 0 ]; then
        echo "[SUCCESS] KEXT loaded."
    else
        echo "[ERROR] Failed to load KEXT. Check 'dmesg' or 'log show --predicate \"processID == 0\" --last 1m'"
    fi
}

# 4. Function to unload
unload_kext() {
    echo "[*] Unloading $KEXT_NAME..."
    sudo kextunload -b com.interlock.Shield
}

case "$1" in
    load)
        do_backup
        load_kext
        ;;
    unload)
        unload_kext
        ;;
    backup)
        do_backup
        ;;
    *)
        echo "Usage: $0 {load|unload|backup}"
        exit 1
esac
