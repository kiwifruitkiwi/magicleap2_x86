#!/bin/bash -e

if [ -z "${OUT}" ]; then
  echo "ERROR: This script must be run from within an Android build window."
  exit 1
fi

if [ "$1" == "" ]; then
  echo "ERROR: Must pass path to prebuilt ramdisk as first parameter"
  echo "Normally, this is \${OUT}/ramdisk-recovery.img, but may be different"
  echo "if you do not want to use a prebuilt ramdisk from another build."
  exit 1
fi

cd ${ANDROID_BUILD_TOP}
PREBUILT_RAMDISK="$1"

ML_PREBUILT_KERNEL_CMDLINE=kernel/common/ml_${TARGET_PRODUCT}_${TARGET_BUILD_VARIANT}_kernel_cmdline.txt

if [ ! -e ${ML_PREBUILT_KERNEL_CMDLINE} ]; then
  echo "ERROR: Unable to find ${ML_PREBUILT_KERNEL_CMDLINE}"
  exit 1
fi

KERNEL_CMDLINE=`cat ${ML_PREBUILT_KERNEL_CMDLINE}`
KERNEL_CMDLINE="${KERNEL_CMDLINE} buildvariant=${TARGET_BUILD_VARIANT}"

echo "Running mkbootimg..."
set -x
${ANDROID_HOST_OUT}/bin/mkbootimg --kernel ${OUT}/kernel \
                                  --ramdisk ${PREBUILT_RAMDISK} \
                                  --cmdline "${KERNEL_CMDLINE}" \
                                  --os_version 10 \
                                  --os_patch_level 2020-02-05 \
                                  --output ${OUT}/boot.img \
