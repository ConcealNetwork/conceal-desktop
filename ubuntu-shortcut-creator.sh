#!/bin/bash
# Copyright (c) 2018-2025 Conceal Network & Conceal Devs
#
# This script is to be used on Linux Ubuntu system after completing the build as per README.md  
# or after downloading and extracting the release zip from github.
# it will create shortcut with Icon.
#
# If you have already installed the application manually, you can run this script to update the shortcut.
# sudo is required since the .desktop file will be written in ~/.local/share/applications/
# Also make sure this script is executable, otherwise run 'chmod 755 ubuntu-shortcut-creator.sh'
#
# run with: sudo ./ubuntu-shortcut-creator.sh

# Define colors for terminal output
if [[ "$TERM" =~ xterm* ]] || [[ "$TERM" =~ screen* ]] || [[ "$TERM" =~ tmux* ]]; then
    # Colors for xterm-compatible terminals
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    ORANGE='\033[0;33m'
    BOLD='\033[1m'
    NC='\033[0m' # No Color
else
    # No colors for other terminals
    RED=''
    GREEN=''
    YELLOW=''
    ORANGE=''
    BOLD=''
    NC=''
fi

NEED_QT_DEPENDENCY=false

# Detect distribution and package manager
if command -v apt >/dev/null 2>&1; then
    DISTRO="debian"
    PKG_MANAGER="apt"
    PKG_CHECK="dpkg -l"
    PKG_NAME="libqt5charts5"
elif command -v pacman >/dev/null 2>&1; then
    DISTRO="arch"
    PKG_MANAGER="pacman"
    PKG_CHECK="pacman -Q"
    PKG_NAME="qt5-charts"
else
    DISTRO="unknown"
    PKG_MANAGER="unknown"
    PKG_CHECK=""
    PKG_NAME=""
fi

# Check if Qt dependencies are actually needed
if [ "$DISTRO" != "unknown" ]; then
    if ! $PKG_CHECK | grep -q "$PKG_NAME"; then
        NEED_QT_DEPENDENCY=true
    fi
fi

# Function to install Qt dependencies
install_qt_dependencies() {
if [ "$NEED_QT_DEPENDENCY" = true ]; then
    echo -e "${YELLOW}${BOLD}Qt Charts is more likely needed for this release version.${NC}"
    read -p "$(echo -e "${BOLD}Confirm install? (Y/n)${NC} ")" -n 1 -r
    echo
    if [[ $REPLY =~ ^[Nn]$ ]]; then
        echo -e "${YELLOW}Qt dependency installation skipped by user${NC}"
    else
        echo -e "${YELLOW}${BOLD}Checking for Qt dependencies...${NC}"
        if [ "$DISTRO" = "debian" ]; then
            echo -e "${ORANGE}Installing Qt5 Charts package (includes all necessary Qt5 libraries)...${NC}"
            apt-get update -qq
            apt install -y -qq libqt5charts5
            echo -e "${GREEN}Qt5 Charts package installed successfully${NC}"
        elif [ "$DISTRO" = "arch" ]; then
            echo -e "${ORANGE}Installing Qt5 Charts package (includes all necessary Qt5 libraries)...${NC}"
            pacman -S -q qt5-charts
            echo -e "${GREEN}Qt5 Charts package installed successfully${NC}"
        else
            echo -e "${YELLOW}${BOLD}Warning: Could not install Qt dependencies automatically.${NC}"
            echo -e "Please install Qt5 Charts package for your distribution."
            echo -e "The application may not work without it."
        fi
    fi
fi
}

# Check if script is run as sudo
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}${BOLD}Error: This script needs to be run as sudo.${NC}"
    echo -e "Please run: ${BOLD}sudo $0${NC}"
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

# Check if desktop file already exists
# incase the user installed manually the desktop file, we need to install the dependencies
if [ -f "$DESKTOP_FILE" ]; then
    echo -e "${YELLOW}${BOLD}Desktop file already exists.${NC}"
    read -p "$(echo -e "${BOLD}Do you want to overwrite it? (y/n)${NC} ")" -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}Operation cancelled${NC}"
        install_qt_dependencies
        sleep 1
        exit 1
    fi
    echo -e "${ORANGE}Overwriting existing desktop file...${NC}"
fi

BINARY_PATH_BUILD="${SCRIPT_DIR}/bin/conceal-desktop"
BINARY_PATH_RELEASE="${SCRIPT_DIR}/conceal-desktop"

# Check if binary exists
if [ -f "$BINARY_PATH_BUILD" ]; then
    BINARY_PATH="$BINARY_PATH_BUILD"
    ICON_PATH="${SCRIPT_DIR}/src/images/conceal.png"
    NEED_QT_DEPENDENCY=false
    echo -e "${ORANGE}Detected: Manual build case${NC}"
elif [ -f "$BINARY_PATH_RELEASE" ]; then
    BINARY_PATH="$BINARY_PATH_RELEASE"
    ICON_PATH="${SCRIPT_DIR}/icon/conceal.png"
    NEED_QT_DEPENDENCY=true
    echo -e "${ORANGE}Detected: Release zip case${NC}"
else
    echo -e "${RED}${BOLD}Error: Binary not found at:${NC}"
    echo -e "  ${RED}$BINARY_PATH_BUILD${NC} (manual build)"
    echo -e "  ${RED}$BINARY_PATH_RELEASE${NC} (release zip)"
    exit 1
fi

# Check if icon exists
if [ ! -f "$ICON_PATH" ]; then
    echo -e "${RED}${BOLD}Error: Icon not found at $ICON_PATH${NC}"
    exit 1
fi

echo -e "${ORANGE}Using binary: ${BOLD}$BINARY_PATH${NC}"
echo -e "${ORANGE}Using icon: ${BOLD}$ICON_PATH${NC}"

# Create desktop file
echo -e "${ORANGE}Creating desktop shortcut...${NC}"
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

# Install Qt dependencies if confirmed
install_qt_dependencies

echo -e "${GREEN}${BOLD}Desktop shortcut created successfully at $DESKTOP_FILE${NC}"
echo -e "${GREEN}You should now be able to find Conceal Desktop in your Applications menu${NC}"