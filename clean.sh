#!/bin/sh

DIR="$( cd "$(dirname "$0")" ; pwd -P )"
cd "${DIR}"/buildroot
make distclean
