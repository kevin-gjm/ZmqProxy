#!/bin/sh

if [ `uname -o ` = "GNU/Linux"   ] ; then
    echo "linux"
    remote_dir=linux/$CI_PROJECT_NAME/$CI_COMMIT_TAG
    python /home/build/upload.py  ./build/$CI_PROJECT_NAME  $remote_dir
else
    echo "win"
    remote_dir=win/$CI_PROJECT_NAME/$CI_COMMIT_TAG
    python D:/run/upload.py  ./build/$CI_PROJECT_NAME.exe  $remote_dir
fi

