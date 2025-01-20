#!/bin/bash
# Copyright (c) 2018-2025 Conceal Network & Conceal Devs
#
# This script is to be used at your convenience after having built on Linux system according to README.md 
#
# Check if zenity is available
HAS_ZENITY=false
if command -v zenity >/dev/null 2>&1; then
    HAS_ZENITY=true
fi

# Check if script is run as root
if [ "$EUID" -ne 0 ]; then
    if [ "$HAS_ZENITY" = true ]; then
        zenity --error \
            --title="Root Privileges Required" \
            --text="This script needs to be run as root.\n\nPlease run:\nsudo $0"
    else
        echo "Error: This script needs to be run as root."
        echo "Please run: sudo $0"
    fi
    exit 1
fi

# Get the actual user who ran the script with sudo
REAL_USER=${SUDO_USER:-$USER}
REAL_HOME=$(getent passwd "$REAL_USER" | cut -d: -f6)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Create user applications directory if it doesn't exist
mkdir -p "${REAL_HOME}/.local/share/applications"

# Define paths
DESKTOP_FILE="${REAL_HOME}/.local/share/applications/conceal-desktop.desktop"
BINARY_PATH="${SCRIPT_DIR}/bin/conceal-desktop"
ICON_PATH="${SCRIPT_DIR}/src/images/conceal.png"

# Check if binary exists
if [ ! -f "$BINARY_PATH" ]; then
    if [ "$HAS_ZENITY" = true ]; then
        zenity --error \
            --title="Error" \
            --text="Binary not found at $BINARY_PATH\n\nPlease run this script from the root directory of the project after building"
    else
        echo "Error: Binary not found at $BINARY_PATH"
        echo "Please run this script from the root directory of the project after building"
    fi
    exit 1
fi

# Check if icon exists
if [ ! -f "$ICON_PATH" ]; then
    if [ "$HAS_ZENITY" = true ]; then
        zenity --error \
            --title="Error" \
            --text="Icon not found at $ICON_PATH"
    else
        echo "Error: Icon not found at $ICON_PATH"
    fi
    exit 1
fi

# Check if desktop file already exists
if [ -f "$DESKTOP_FILE" ]; then
    if [ "$HAS_ZENITY" = true ]; then
        if ! zenity --question \
            --title="File Exists" \
            --text="Desktop file already exists. Do you want to overwrite it?" \
            --ok-label="Yes" \
            --cancel-label="No"; then
            exit 1
        fi
    else
        read -p "Desktop file already exists. Do you want to overwrite it? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Operation cancelled"
            exit 1
        fi
    fi
fi

# Create desktop file
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Encoding=UTF-8
Name=Conceal Desktop
Comment=Conceal Desktop
Exec=$BINARY_PATH
Terminal=false
Type=Application
Icon=$ICON_PATH
MimeType=x-scheme-handler/concealwallet;
Categories=Office;Finance;Cryptocurrency;
EOF

# Set proper ownership
chown ${REAL_USER}:${REAL_USER} "$DESKTOP_FILE"

echo "Desktop shortcut created successfully at $DESKTOP_FILE"
echo "You should now be able to find Conceal Desktop in your Applications menu" 