#!/bin/bash

BRANCH=`git branch | grep \* | cut -d ' ' -f2`
TAG=`git describe --always`
DATE=`date +%y.%m.%d-%H.%M`

NAME=dynamic_mapping-${BRANCH}-${DATE}-${TAG}

cp -R bin/ ${NAME} 

tar -czf ${NAME}.tar.gz ${NAME}
rm -rf ${NAME}

mv ${NAME}.tar.gz /media/data/dev/backups