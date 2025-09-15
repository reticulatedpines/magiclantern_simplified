FROM archlinux:latest

RUN pacman --noconfirm -Syu && \
    pacman --noconfirm -S make python3 python-pip python-distutils-extra python-docutils gcc arm-none-eabi-gcc arm-none-eabi-newlib inetutils zip && \
    rm /var/cache/pacman/pkg/*

# Create the build user
RUN useradd -m build && \
    # Delete the build users password
    passwd -d build && \
    # Allow the build user passwordless sudo
    printf 'build ALL=(ALL) ALL\n' | tee -a /etc/sudoers

USER build
