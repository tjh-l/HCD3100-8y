#!/bin/bash

. $BR2_CONFIG > /dev/null 2>&1
export BR2_CONFIG

cp -vf ${IMAGES_DIR}/hcrtosapp.uImage ${IMAGES_DIR}/app.uImage
