#!/bin/bash

TARGET=buildroot/output/target

if [ -f "$TARGET" ]
then
rm -rf buildroot/output/target
fi

find buildroot/output/ -name ".stamp_target_installed" |xargs rm -rf

