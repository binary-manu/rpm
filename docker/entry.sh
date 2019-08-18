#!/bin/sh

# Ensure that our user has a home directory
export HOME=$(dirname $WINEPREFIX)
mkdir -p $HOME

wine cmd /c echo

cd "$WINEPREFIX/dosdevices"
[ ! -e 't:' ] && ln -s $TOOLSDIR  't:'
[ ! -e 's:' ] && ln -s $SOURCEDIR 's:'

cd "$SOURCEDIR"
wine make "$@"
