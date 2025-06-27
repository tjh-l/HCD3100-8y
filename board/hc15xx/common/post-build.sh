#!/bin/bash

. $BR2_CONFIG > /dev/null 2>&1
export BR2_CONFIG

current_dir=$(dirname $0)
PYPATH=/usr/bin:/usr/local/bin:$PATH
FDTINFO=${TOPDIR}/build/tools/fdt/getfdt
BINMODIFY=${TOPDIR}/build/scripts/binmodify.py
GENBOOTMEDIA=${TOPDIR}/build/scripts/genbootmedia
GENSFBIN=${TOPDIR}/build/scripts/gensfbin.py
HCPROGINI=${TOPDIR}/build/tools/fdt/gen_hcprogini
GENFLASHBIN=${TOPDIR}/build/tools/fdt/gen_flashbin
FIXUP_PART_FNAME=${TOPDIR}/build/tools/fdt/fixup_part_filename
GET_PART_FNAME=${TOPDIR}/build/tools/fdt/get_part_filename
GENPERSISTENTMEM=${TOPDIR}/build/tools/fdt/genpersistentmem
DDRCONFIGMODIFY=${TOPDIR}/build/tools/fdt/ddrconfig_modify
DTB2DTS=${TOPDIR}/build/tools/dtb2dts.py
DTS2DTB=${TOPDIR}/build/tools/dts2dtb.py
MKYAFFS2IMAGE=${TOPDIR}/build/tools/mkyaffs2image
HCFOTAGEN=${TOPDIR}/build/tools/fdt/HCFota_Generator
DTB=${IMAGES_DIR}/dtb.bin
if [ -f ${HOST_DIR}/bin/hcprecomp2 ] ; then
	HCPRECOMP2=${HOST_DIR}/bin/hcprecomp2
else
	HCPRECOMP2=${TOPDIR}/build/tools/hcprecomp2
fi
if [ -f ${HOST_DIR}/bin/mkimage ] ; then
	MKIMAGE=${HOST_DIR}/bin/mkimage
else
	MKIMAGE=${TOPDIR}/build/tools/mkimage
fi
make -C $(dirname ${HCPROGINI})

mkdir -p ${IMAGES_DIR}/for-factory
mkdir -p ${IMAGES_DIR}/for-upgrade
mkdir -p ${IMAGES_DIR}/for-debug
mkdir -p ${IMAGES_DIR}/dts

if [ "${DDR_SIZE}" == "16M" ]; then
	UPDATER_LOAD_ADDR=0x80400000
	UPDATER_LOAD_ADDR_NONCACHE=0xA0400000
else
	UPDATER_LOAD_ADDR=0x81000000
	UPDATER_LOAD_ADDR_NONCACHE=0xA1000000
fi

message()
{
	echo -e "\033[47;30m>>> $1\033[0m"
}

if [ "${BR2_EXTERNAL_PRODUCT_VERSION}" = "y" ]; then
	firmware_version=$(expr ${BR2_EXTERNAL_PRODUCT_VERSION_VERSION} \* 65536 + 0${BR2_EXTERNAL_PRODUCT_VERSION_PATCHLEVEL} \* 256 + 0${BR2_EXTERNAL_PRODUCT_VERSION_SUBLEVEL})
else
	firmware_version=$(date +%y%m%d%H%M)
fi

app=${CONFIG_APPS_NAME}
oldapp=$(${GET_PART_FNAME} -i ${DTB} -l "firmware")
hcprogrammer_support_usb0=0
hcprogrammer_support_usb1=0
hcprogrammer_usb_irq_detect_timeout=0
hcprogrammer_usb_sync_detect_timeout=0
if [ "${BR2_EXTERNAL_HCPROGRAMMER_SUPPORT}" = "y" ] ; then
	if [ "${BR2_EXTERNAL_HCPROGRAMMER_SUPPORT_USB0}" = "y" ] ; then
		hcprogrammer_support_usb0=1
	else
		hcprogrammer_support_usb0=0
	fi
	if [ "${BR2_EXTERNAL_HCPROGRAMMER_SUPPORT_USB1}" = "y" ] ; then
		hcprogrammer_support_usb1=1
	else
		hcprogrammer_support_usb1=0
	fi
	if [ "${BR2_EXTERNAL_HCPROGRAMMER_USB_IRQ_DETECT_TIMEOUT}" != "" ]; then
		hcprogrammer_usb_irq_detect_timeout=${BR2_EXTERNAL_HCPROGRAMMER_USB_IRQ_DETECT_TIMEOUT}
	fi
	if [ "${BR2_EXTERNAL_HCPROGRAMMER_USB_SYNC_DETECT_TIMEOUT}" != "" ]; then
		hcprogrammer_usb_sync_detect_timeout=${BR2_EXTERNAL_HCPROGRAMMER_USB_SYNC_DETECT_TIMEOUT}
	fi
fi

if [ -f ${DTB} ] ; then
	message "Update firmware filename to ${app}.uImage ....."
	${FIXUP_PART_FNAME} -i ${DTB} -o ${DTB} -l "firmware" -n ${app}.uImage
	message "Update firmware filename to ${app}.uImage done!"
fi

if [ -f ${IMAGES_DIR}/${app}.out ] ; then
	app_ep_noncache=$(readelf -h ${IMAGES_DIR}/${app}.out | grep Entry | awk '{print $NF}' | sed 's/0x8/0xa/')
	app_ep=$(readelf -h ${IMAGES_DIR}/${app}.out | grep Entry | awk '{print $NF}')
	app_load_noncache=$(nm -n ${IMAGES_DIR}/${app}.out | awk '/T _start/ {print "0x"$1}' | sed 's/0x8/0xa/')
	app_load=$(nm -n ${IMAGES_DIR}/${app}.out | awk '/T _start/ {print "0x"$1}')
fi

if [ "${CONFIG_OF_SEPARATE}" = "y" ] && [ -f ${IMAGES_DIR}/dtb.bin ]; then
	if [ -f ${IMAGES_DIR}/${app}.out ] ; then
	message "Generating download-${app}.ini ....."
	cat << EOF > ${IMAGES_DIR}/download-${app}.ini
[Project]
RunMode=0
InitMode=0
RunAddr=${app_ep_noncache}
FileNum=2
[File0]
File=${app}.out
Type=5
Addr=${app_load_noncache}
[File1]
File=dtb.bin
Type=1
Addr=0xa5ff0000
[AutoRun]
AutoRun0=wm 0xb8800004 0x85ff0000
EOF
	message "Generating download-${app}.ini done!"
	fi
fi

if [ -f ${IMAGES_DIR}/boardtest.out ] && [ -f ${IMAGES_DIR}/boardtest.bin ]; then
	boardtest_ep=$(readelf -h ${IMAGES_DIR}/boardtest.out | grep Entry | awk '{print $NF}')
	boardtest_load=$(nm -n ${IMAGES_DIR}/boardtest.out | awk '/T _start/ {print "0x"$1}')
	${MKIMAGE} -A mips -O u-boot -T standalone -C none -n boardtest -e ${boardtest_ep} -a ${boardtest_load} \
		-d ${IMAGES_DIR}/boardtest.bin ${IMAGES_DIR}/boardtest.uImage
fi

if [ -f ${IMAGES_DIR}/hcboot.out ] && [ -f ${IMAGES_DIR}/hcboot.bin ] && [ -f "${BR2_EXTERNAL_BOARD_DDRINIT_FILE}" ]; then
	message "Generating bootloader.bin ....."
	fddrinit=$(basename ${BR2_EXTERNAL_BOARD_DDRINIT_FILE})
	hcboot_sz=$(wc -c ${IMAGES_DIR}/hcboot.bin | awk '{print $1}')
	hcboot_ep=$(readelf -h ${IMAGES_DIR}/hcboot.out | grep Entry | awk '{print $NF}')
	if [ -f ${IMAGES_DIR}/hcboot.is_dtb_separate ]; then
		hcboot_is_dtb_separate=1
	else
		hcboot_is_dtb_separate=0
	fi
	if [ "${BR2_EXTERNAL_HCPROGRAMMER_SUPPORT_WINUSB}" = "y" ] ; then
		hcprogrammer_support_winusb=1
	else
		hcprogrammer_support_winusb=0
	fi
	${DDRCONFIGMODIFY} --input ${BR2_EXTERNAL_BOARD_DDRINIT_FILE} --output ${IMAGES_DIR}/${fddrinit} \
		--size ${hcboot_sz} \
		--entry ${hcboot_ep} \
		--from 0xafc03000 \
		--to ${hcboot_ep} \
		--dtb ${DTB} \
		--portA ${hcprogrammer_support_usb0} \
		--portB ${hcprogrammer_support_usb1} \
		--irq ${hcprogrammer_usb_irq_detect_timeout} \
		--sync ${hcprogrammer_usb_sync_detect_timeout} \
		--version ${firmware_version} \
		--product ${BR2_EXTERNAL_PRODUCT_NAME} \
		--loaddtb ${hcboot_is_dtb_separate} \
		--winusb ${hcprogrammer_support_winusb}

	cat ${IMAGES_DIR}/${fddrinit} ${IMAGES_DIR}/hcboot.bin > ${IMAGES_DIR}/bootloader.bin
	message "Generating bootloader.bin done!"
fi

if [ -f ${IMAGES_DIR}/${app}.bin ] ; then
	message "Generating ${app}.uImage ....."
	if [ "${BR2_EXTERNAL_FW_COMPRESS_LZO1X}" = "y" ] ; then

		${HCPRECOMP2} ${IMAGES_DIR}/${app}.bin  ${IMAGES_DIR}/${app}.bin.lzo
		${MKIMAGE} -A mips -O u-boot -T standalone -C lzo -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin.lzo ${IMAGES_DIR}/${app}.uImage

	elif [ "${BR2_EXTERNAL_FW_COMPRESS_GZIP}" = "y" ] ; then

		gzip -kf9 ${IMAGES_DIR}/${app}.bin > ${IMAGES_DIR}/${app}.bin.gz
		${MKIMAGE} -A mips -O u-boot -T standalone -C gzip -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin.gz ${IMAGES_DIR}/${app}.uImage

	elif [ "${BR2_EXTERNAL_FW_COMPRESS_LZMA}" = "y" ] ; then

		lzma -zkf -c ${IMAGES_DIR}/${app}.bin > ${IMAGES_DIR}/${app}.bin.lzma
		${MKIMAGE} -A mips -O u-boot -T standalone -C lzma -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin.lzma ${IMAGES_DIR}/${app}.uImage

	elif [ "${BR2_EXTERNAL_FW_COMPRESS_NONE}" = "y" ] ; then
		${MKIMAGE} -A mips -O u-boot -T standalone -C none -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin ${IMAGES_DIR}/${app}.uImage

	fi
	message "Generating ${app}.uImage done!"
fi

if [ "${BR2_PACKAGE_PREBUILTS_ECR6600U}" = "y" ] ; then
	cp -vf ${PREBUILT_DIR}/usr/lib/firmware/ECR6600U_transport.bin ${IMAGES_DIR}/fs-partition1-root/
fi

if [ -f "${BR2_EXTERNAL_BOOTMEDIA_FILE}" ] ; then
	message "Generating logo.hc ....."
	${GENBOOTMEDIA} -i ${BR2_EXTERNAL_BOOTMEDIA_FILE} -o ${IMAGES_DIR}/fs-partition1-root/logo.hc
	message "Generating logo.hc done!"
fi

if [ -d ${IMAGES_DIR}/fs-partition1-root -a "`ls -A ${IMAGES_DIR}/fs-partition1-root`" != "" ] ; then
	message "Generating romfs.img ....."
	genromfs -f ${IMAGES_DIR}/romfs.img -d ${IMAGES_DIR}/fs-partition1-root/ -V "romfs"
	message "Generating romfs.img done!"
fi

if [ -d ${IMAGES_DIR}/fs-partition2-root -a "`ls -A ${IMAGES_DIR}/fs-partition2-root`" != "" ] ; then
	message "Generating romfs2.img ....."
	genromfs -f ${IMAGES_DIR}/romfs2.img -d ${IMAGES_DIR}/fs-partition2-root/ -V "romfs"
	message "Generating romfs2.img done!"
fi

if [ -d ${IMAGES_DIR}/fs-partition3-root -a "`ls -A ${IMAGES_DIR}/fs-partition3-root`" != "" ] ; then
	message "Generating romfs3.img ....."
	genromfs -f ${IMAGES_DIR}/romfs3.img -d ${IMAGES_DIR}/fs-partition3-root/ -V "romfs"
	message "Generating romfs3.img done!"
fi

message "Generating persistentmem.bin ....."
persistentmem_fs=$(${FDTINFO} ${DTB} /hcrtos/persistentmem string fs-type)
if [ "${persistentmem_fs}" == "yaffs2" ] || [ "${persistentmem_fs}" == "littlefs" ] ; then
	${GENPERSISTENTMEM} -v ${firmware_version} -p ${BR2_EXTERNAL_PRODUCT_NAME} -d ${DTB} -o ${IMAGES_DIR}/fs-partition2-root/persistentmem-0.bin
else
	${GENPERSISTENTMEM} -v ${firmware_version} -p ${BR2_EXTERNAL_PRODUCT_NAME} -d ${DTB} -o ${IMAGES_DIR}/persistentmem.bin
fi
if [ "${persistentmem_fs}" == "yaffs2" ] ; then
	yaffs2img=$(${GET_PART_FNAME} -i ${DTB} -l "yaffs2")
	${MKYAFFS2IMAGE} ${IMAGES_DIR}/fs-partition2-root ${IMAGES_DIR}/${yaffs2img} ${CONFIG_MTD_SPINAND_PAGESIZE} ${CONFIG_MTD_SPINAND_ERASESIZE}
elif [ "${persistentmem_fs}" == "littlefs" ] ; then
	echo "TO BE SUPPORTED TO GENERATE LITTLEFS"
fi
message "Generating persistentmem.bin done"

message "Generating flash binary ....."
${GENFLASHBIN} --wkdir ${IMAGES_DIR} --dtb ${DTB} --outdir ${IMAGES_DIR}/for-factory
[ $? != 0 ] && exit 1;
message "Generating flash binary done!"

message "Generating ${BR2_EXTERNAL_HCFOTA_FILENAME} .....!"
rm -vf ${IMAGES_DIR}/for-{upgrade,debug}/$(basename ${BR2_EXTERNAL_HCFOTA_FILENAME} .bin)*
cp -vf ${TOPDIR}/build/tools/hc15xx_jtag_updater.bin ${IMAGES_DIR}
${HCFOTAGEN} --dtb ${DTB} -o ${IMAGES_DIR}/for-upgrade/${BR2_EXTERNAL_HCFOTA_FILENAME} -u \
	-r "${BR2_EXTERNAL_BOARD_DDRINIT_FILE}" -p "${IMAGES_DIR}/hc15xx_jtag_updater.bin" \
	--version ${firmware_version} \
	--product ${BR2_EXTERNAL_PRODUCT_NAME}
[ $? != 0 ] && exit 1;
${HCFOTAGEN} --dtb ${DTB} -o ${IMAGES_DIR}/for-debug/${BR2_EXTERNAL_HCFOTA_FILENAME} \
	-r "${BR2_EXTERNAL_BOARD_DDRINIT_FILE}" -p "${IMAGES_DIR}/hc15xx_jtag_updater.bin" \
	--version ${firmware_version} \
	--product ${BR2_EXTERNAL_PRODUCT_NAME}
[ $? != 0 ] && exit 1;
message "Generating ${BR2_EXTERNAL_HCFOTA_FILENAME} done!"

message "Generating sfburn.ini ....."
updater_ep_noncache=$(readelf -h ${TOPDIR}/build/tools/hc15xx_jtag_updater.out | grep Entry | awk '{print $NF}' | sed 's/0x8/0xa/')
updater_load_noncache=$(nm -n ${TOPDIR}/build/tools/hc15xx_jtag_updater.out | awk '/T _start/ {print "0x"$1}' | sed 's/0x8/0xa/')

md5=$(md5sum ${IMAGES_DIR}/for-debug/${BR2_EXTERNAL_HCFOTA_FILENAME} | awk '{print $1}' | cut -c1-5)
cp -f ${IMAGES_DIR}/for-debug/${BR2_EXTERNAL_HCFOTA_FILENAME} ${IMAGES_DIR}/for-debug/${BR2_EXTERNAL_HCFOTA_FILENAME}.${md5}
fota_size=$(wc -c ${IMAGES_DIR}/for-debug/${BR2_EXTERNAL_HCFOTA_FILENAME} | awk '{print $1}' | xargs -i printf "0x%08x" {})
cat << EOF > ${IMAGES_DIR}/for-debug/sfburn.ini
[Project]
RunMode=0
InitMode=0
RunAddr=${updater_ep_noncache}
FileNum=2
[File0]
File=${BR2_EXTERNAL_HCFOTA_FILENAME}.${md5}
Type=1
Addr=${UPDATER_LOAD_ADDR_NONCACHE}
[File1]
File=hc15xx_jtag_updater.out
Type=5
Addr=${updater_load_noncache}
[AutoRun]
AutoRun0=wm 0x800001F0 $(printf 0x%08x $fota_size)
AutoRun1=wm 0x800001F4 ${UPDATER_LOAD_ADDR}
AutoRun1=wm 0x800001F8 0
AutoRun3=wm 0xb8818504 0x0
EOF
message "Generating sfburn.ini done!"

if [ "${BR2_PACKAGE_APPS_HCUSBCAST}" = "y" ];then
	message "Generating IUM_*******_*******.bin .....!"
	rm -f ${IMAGES_DIR}/IUM*.bin
	${IMAGES_DIR}/pcm2wav ${IMAGES_DIR}/for-debug/HCFOTA_${BR2_EXTERNAL_PRODUCT_NAME}_${firmware_version}.bin ${IMAGES_DIR}/IUM_${BR2_EXTERNAL_PRODUCT_NAME}_${firmware_version}.bin
	message "Generating IUM_${BR2_EXTERNAL_PRODUCT_NAME}_${firmware_version} done!"
fi

cp -vf ${TOPDIR}/build/tools/hc15xx_jtag_updater.out ${IMAGES_DIR}/for-debug
cp -vf ${TOPDIR}/build/tools/hc15xx_jtag_updater.bin ${IMAGES_DIR}
cp -vf ${TOPDIR}/build/tools/HCProgrammer.exe ${IMAGES_DIR}/for-debug
cp -vf ${IMAGES_DIR}/${app}.uImage ${IMAGES_DIR}/${oldapp}
cp -f $(dirname $0)/dts/lcd/* ${IMAGES_DIR}/dts/
for f in ${CONFIG_CUSTOM_DTS_PATH}; do
	dts_basename=$(basename ${CONFIG_DEFAULT_DEVICE_TREE} .dtb)
	f_basename=$(basename ${f} .dts)
	if [ "${dts_basename}" == "${f_basename}" ]; then
		[[ -f ${f} ]] && cp -vf ${f} ${IMAGES_DIR}/dts/board.dts
		[[ -f ${TOPDIR}/${f} ]] && cp -vf ${TOPDIR}/${f} ${IMAGES_DIR}/dts/board.dts
	else
		[[ -f ${f} ]] && cp -vf ${f} ${IMAGES_DIR}/dts/
		[[ -f ${TOPDIR}/${f} ]] && cp -vf ${TOPDIR}/${f} ${IMAGES_DIR}/dts/
	fi
done

chmod +x ${TOPDIR}/board/hc1xxx/common/gen_upgrade_pkt.sh
source ${TOPDIR}/board/hc1xxx/common/gen_upgrade_pkt.sh ${IMAGES_DIR} ${BR2_CONFIG} ${BR2_EXTERNAL_PRODUCT_NAME} ${firmware_version}
