#!/bin/bash
# TOOLS_PATH="/home/xubin/work/tools/platform/anyka/platform/rootfs"

mkdir -p update/
mkdir -p out

make PLATFORM=PC DBG=1
exit

if [ "$#" -eq 1 ]; then
	make PLATFORM=PC DBG=1
else
	make PLATFORM=RV1108 DBG=1
fi

if [ "$?" == 0 ]; then
	echo "Debug build finished!"
else
	echo "make $# error!"
fi
