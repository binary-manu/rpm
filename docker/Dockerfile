FROM opensuse/leap:15.2

RUN zypper ref && zypper in -y wine dosbox && zypper cc -a

ENV SDL_VIDEODRIVER=dummy \
    SDL_AUDIODRIVER=dummy \
    SOURCEDIR=/build/src \
    TOOLSDIR=/build/tools \
    WINEPREFIX=/tmp/home/.wine \
    REALDOSBOX=/usr/bin/dosbox

ENV WINEPATH=$TOOLSDIR/BC5/BIN \
    BUILDLOG=$SOURCEDIR/BUILD.LOG

COPY entry.sh /entry.sh
COPY dosbox /usr/local/bin/dosbox

ENTRYPOINT ["/entry.sh"]
