#!/bin/bash

. $BR2_CONFIG > /dev/null 2>&1
export BR2_CONFIG

#generate network upgrade package.
echo "**************************************************"
echo "Generate the network upgrade package"

DIR_PWD=$(pwd)
echo ${DIR_PWD}

DIR_WORK=$1
BR_CFG=$2 
PRODUCT_NAME=$3
PRODUCT_VERSION=$4

DIR_NET_UPG=for-net-upgrade

if [ -z ${DIR_WORK} ] ; then
	DIR_WORK="."
fi
cd ${DIR_WORK}


UPGRADE_FILE=${IMAGES_DIR}/for-upgrade/$(basename ${BR2_EXTERNAL_HCFOTA_FILENAME} .bin).ota.bin

if [ ! -f ${UPGRADE_FILE} ] ; then
	echo "NO ${UPGRADE_FILE} !!!!"
	cd ${DIR_PWD}
	exit
else
	echo "Upgrade file:" ${UPGRADE_FILE}
fi

if [ -f vmlinux ] ; then
	OS_NAME=linux
	APP_PROJECTOR="PROJECTORAPP=y"
	APP_SCREEN="SCREENAPP=y"
else
	OS_NAME=rtos
	APP_PROJECTOR="APPS_PROJECTOR=y"
	APP_SCREEN="APPS_HCSCREEN=y"
	
	#rtos, then input parameter BR_CFG must be merged to be absolute address
	if [[ "${BR_CFG}" != /* ]]; then
		BR_CFG=${DIR_PWD}/${BR_CFG}
	fi
fi
#echo "pwd:" $(pwd) "BR_CFG" ${BR_CFG}

if [ -z ${BR_CFG} ] ; then
	APP_NAME=hcprojector
else
	APP_GET=$(grep -nr ${APP_SCREEN} ${BR_CFG} | grep "=y")
	if [ -n "${APP_GET}" ] ; then
		APP_NAME=hcscreen
	fi
	APP_GET=$(grep -nr ${APP_PROJECTOR} ${BR_CFG} | grep "=y")
	if [ -n "${APP_GET}" ] ; then
		APP_NAME=hcprojector
	fi
fi

if [ -z ${APP_NAME} ] ; then
	APP_NAME=hcapp
fi

echo "OS_NAME:" ${OS_NAME}

echo "APP_NAME:" ${APP_NAME}

if [ -d ${DIR_NET_UPG} ] ; then
	rm -rf  ${DIR_NET_UPG}
fi

mkdir -p ${DIR_NET_UPG}/${PRODUCT_VERSION}

echo "work dir: $DIR_WORK"
	
cp ${UPGRADE_FILE} ${DIR_NET_UPG}/${PRODUCT_VERSION}

#here you can modify the url of upgreaded binary
#like this: "http://172.16.12.81:80/hccast/linux/HC16A3000V104K/hcprojector/2307261826/HCFOTA.bin"
#BIN_URL=http://172.16.12.81:80/hccast/${OS_NAME}/${PRODUCT_NAME}/${APP_NAME}/${PRODUCT_VERSION}/${BR2_EXTERNAL_HCFOTA_FILENAME}
BIN_URL=${CONFIG_HTTP_UPGRADE_URL}/hccast/${OS_NAME}/${PRODUCT_NAME}/${APP_NAME}/${PRODUCT_VERSION}/${BR2_EXTERNAL_HCFOTA_FILENAME}

UPGRADE_JSON=HCFOTA.jsonp
if [ -d ${DIR_NET_UPG} ] ; then
	echo "Generating upgrade json config ..."
	cat << EOF > ${DIR_NET_UPG}/${UPGRADE_JSON}
jsonp_callback({
  "product": "${PRODUCT_NAME}",
  "version": "${PRODUCT_VERSION}",
  "force_upgrade": true,
  "url": "${BIN_URL}"
})

EOF

	echo "Generating upgrade json config ${UPGRADE_JSON} done!"
fi

cd ${DIR_PWD}
echo "revert dir:" $(pwd)
echo "**************************************************"
