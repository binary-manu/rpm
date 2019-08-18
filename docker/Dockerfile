FROM archlinux/base

RUN  \
    printf '\n[multilib]\nInclude = /etc/pacman.d/mirrorlist\n' \
        >> /etc/pacman.conf && \
    pacman-key --init && pacman-key --populate archlinux && \
    pacman -Syu --noconfirm dosbox wine && \
    rm -f /var/lib/pacman/sync/* /var/cache/pacman/pkg/*

ENV SDL_VIDEODRIVER=dummy \
    SDL_AUDIODRIVER=dummy \
    SOURCEDIR=/build/src \
    TOOLSDIR=/build/tools \
    WINEPREFIX=/tmp/home/.wine \
    REALDOSBOX=/usr/bin/dosbox

ENV WINEPATH=$TOOLSDIR/BC5/BIN

COPY entry.sh /entry.sh
COPY dosbox /usr/local/bin/dosbox

ENTRYPOINT ["/entry.sh"]
