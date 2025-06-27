#!/bin/bash

. $BR2_CONFIG > /dev/null 2>&1
export BR2_CONFIG

current_dir=$(dirname $0)
cp -vf ${current_dir}/popup.bmp.gz ${IMAGES_DIR}/fs-partition2-root/
