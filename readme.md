# Wayland Screen Locker
A lightweight, secure screen locker for Wayland compositors with visual authentication feedback and customizable icons.

## Features
+ Wayland Native: Built using the ext-session-lock-v1 protocol for secure screen locking
+ Visual Feedback: Dynamic icon states that change based on authentication progress
+ Custom Fonts: Support for Nerd Fonts with Unicode icons
+ Cairo Graphics: High-quality text and icon rendering with anti-aliasing
+ PAM Authentication: Secure user authentication using the system's PAM stack

## Building
*This has only been used on my system that runs arch, so if you need something else change is welcome.*
### Arch Linux
```
sudo pacman -S --needed wayland pam libxkbcommon cairo meson
meson setup build
ninja -C build
```
