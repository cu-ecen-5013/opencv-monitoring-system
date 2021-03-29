#!/bin/sh
# Shared definitions for buildroot scripts

# The defconfig from the buildroot directory we use for qemu builds
#QEMU_DEFCONFIG=configs/qemu_aarch64_virt_defconfig
PI4_DEFCONFIG=configs/raspberrypi4_defconfig
# The place we store customizations to the qemu configuration
MODIFIED_PI4_DEFCONFIG=base_external/configs/aesd_pi4_defconfig
# The defconfig from the buildroot directory we use for the project
AESD_DEFAULT_DEFCONFIG=${PI4_DEFCONFIG}
AESD_MODIFIED_DEFCONFIG=${MODIFIED_PI4_DEFCONFIG}
AESD_MODIFIED_DEFCONFIG_REL_BUILDROOT=../${AESD_MODIFIED_DEFCONFIG}
