# Desktop Environment Installer

A simple GTK-based graphical application for installing desktop environments on various Linux distributions.

## Features
- Detects current Linux distribution automatically
- Supports multiple package managers:
  - apt (Debian/Ubuntu)
  - dnf (Fedora)
  - pacman (Arch/Manjaro)
  - zypper (openSUSE)
  - apk (Alpine)
  - emerge (Gentoo)
- Supported desktop environments:
  - Xfce
  - KDE Plasma
  - GNOME
  - Cinnamon
  - LXDE
- Updates package repositories before installation
- User-friendly GUI interface
- Error handling and feedback

## Prerequisites
- GTK 3 development libraries (`libgtk-3-dev` on Debian-based systems)
- A supported Linux distribution
- Root privileges (via pkexec) for package installation
- C compiler (gcc recommended)

## Installation

1. Clone the repository:
```bash
git clone https://github.com/allanfherrera/deskinstall/upload/main
cd desktop-environment-installer
gcc -o de-installer main.c `pkg-config --cflags --libs gtk+-3.0`

2. Usage
Run the application:
```bash
./deskinstall
