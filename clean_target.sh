#!/bin/bash

TARGET=buildroot/output/target

if [ -d "$TARGET" ]
then
rm -Rf buildroot/output/target
fi

find buildroot/output/ -name ".stamp_target_installed" |xargs rm -rf

