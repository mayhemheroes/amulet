#!/bin/sh
set -xe
export NDK_HOME=~/AppData/Local/Android/sdk/ndk/29.0.14206865
export NDK_HOST=windows-x86_64
echo TARGET=android_x86_64.release > settings
echo LUAVM=lua52 >> settings
make
