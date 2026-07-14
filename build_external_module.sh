#!/bin/bash
set -e

export RDIR="$(pwd)"
export ARCH=arm64
export PLATFORM_VERSION=12
export ANDROID_MAJOR_VERSION=s

export BUILD_CROSS_COMPILE="${RDIR}/toolchain/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-"
export BUILD_CC="${RDIR}/toolchain/clang/host/linux-x86/clang-r353983c/bin/clang"

export MODULE_DIR="${RDIR}/external/khack"

make -w \
  -C "${RDIR}" \
  O="${RDIR}/out" \
  M="${MODULE_DIR}" \
  -j"$(nproc)" \
  ARCH=arm64 \
  PLATFORM_VERSION=12 \
  ANDROID_MAJOR_VERSION=s \
  CROSS_COMPILE="${BUILD_CROSS_COMPILE}" \
  CC="${BUILD_CC}" \
  modules

mkdir -p "${RDIR}/build/modules"
cp "${MODULE_DIR}"/*.ko "${RDIR}/build/modules/"
