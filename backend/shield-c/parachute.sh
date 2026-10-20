#!/bin/bash
# Interlock Assessment - Parachute Protocol (Phase 0)
# Emergency rescue script to be run from macOS Recovery Mode Terminal.

echo "----------------------------------------------------"
echo "INTERLOCK SHIELD: EMERGENCY PARACHUTE ACTIVATED"
echo "----------------------------------------------------"

# 1. Identificar el volumen principal (saltando volúmenes de sistema/recuperación)
DISK_NAME=$(ls /Volumes | grep -v "Recovery" | grep -v "External" | grep -v "Data" | head -n 1)
TARGET_VOL="/Volumes/${DISK_NAME}"

if [ ! -d "$TARGET_VOL" ]; then
    echo "[!] Error: No se pudo localizar el volumen principal en ${TARGET_VOL}"
    exit 1
fi

echo "[*] Target Volume identified: ${TARGET_VOL}"

# 2. Rutas críticas de la KEXT
KEXT_PATH="${TARGET_VOL}/Library/Extensions/InterlockShield.kext"
STAGING_PATH="${TARGET_VOL}/Library/KernelCollections"

# 3. Eliminación quirúrgica
if [ -d "$KEXT_PATH" ]; then
    echo "[+] Removing Malformed KEXT..."
    rm -rf "$KEXT_PATH"

    echo "[+] Clearing Kernel Staging and Collections..."
    # kmutil es esencial en macOS Big Sur y superiores
    kmutil clear-staging --volume-root "${TARGET_VOL}" 2>/dev/null

    echo "[SUCCESS] Interlock Shield has been neutralized."
    echo "[i] You can now reboot normally."
else
    echo "[-] InterlockShield.kext not found. System might be clean or KEXT was loaded in-memory only."
fi
