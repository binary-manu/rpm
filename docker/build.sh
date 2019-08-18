#!/bin/sh

IMAGE="ranish-borlandc-builder"

if [ -n "$FORCE_REBUILD" ] || ! docker image inspect "$IMAGE" > /dev/null 2>&1; then
    # Build the image
    [ -n "$http_proxy"  ] && PROXY_ARGS="$PROXY_ARGS --build-arg http_proxy=$http_proxy"
    [ -n "$https_proxy" ] && PROXY_ARGS="$PROXY_ARGS --build-arg https_proxy=$https_proxy"
    [ -n "$no_proxy"    ] && PROXY_ARGS="$PROXY_ARGS --build-arg no_proxy=$no_proxy"
    docker build $PROXY_ARGS -f Dockerfile --tag "$IMAGE" . || exit 1
fi

docker run --rm -it --user $(id -u):$(id -g) -v $(pwd)/../:/build "$IMAGE" "$@"
