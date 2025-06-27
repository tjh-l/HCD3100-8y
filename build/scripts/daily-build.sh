#!/bin/bash

rm -rf images_dailybuild
mkdir images_dailybuild
touch images_dailybuild/dailybuild.result.txt
touch images_dailybuild/dailybuild.fail.txt

ddr_clk=""
ddr_type=""
ddr_size=""
ddr_sip=""
daily_build_result="Success"

function log()
{
	echo -e "$1"
	echo -e "$1" >> images_dailybuild/dailybuild.result.txt
}

function log_fail()
{
	echo -e "$1"
	echo -e "$1" >> images_dailybuild/dailybuild.fail.txt
}

for bootdefconfig in $(ls configs/ | grep "_bl_defconfig$"); do
	defconfig=$(echo $bootdefconfig | sed 's/_bl_/_/')
	if [ ! -f configs/$defconfig ] ; then
		continue
	fi

	ddr_clk=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep MHz)
	ddr_type=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep ddr[2,3])
	ddr_size=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i++){print $i}}' | grep -E "64M|128M|256M")
	ddr_sip=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep sip)
	soc_family=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.:/]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep -m1 hc1[5,6]xx)
	rm -rf output*
	log "Building using $bootdefconfig ..."
	make O=output-bl $bootdefconfig
	make O=output-bl all
	log "Building using $defconfig ..."
	make $defconfig
	make all
	if [ -f output/images/for-debug/HCFOTA.bin ] ; then
		log "Building $defconfig success!!!"
		if [ "${ddr_sip}" == "sip" ]; then
			for ddr in $(ls board/${soc_family}/common/ddrinit/${soc_family}_sip_${ddr_type}_${ddr_size}_*.abs); do
				log "Building using $ddr"
				ddr_file=$(basename $ddr)
				sed -i "/BR2_EXTERNAL_BOARD_DDRINIT_FILE/s/ddrinit.*/ddrinit\/${ddr_file}\"/" output/.config
				make all
				backup_dir=$(basename $ddr .abs)
				rm -rf images_dailybuild/${defconfig}/${backup_dir}
				mkdir -p images_dailybuild/${defconfig}/${backup_dir}
				cp -rf output/images/for-* images_dailybuild/${defconfig}/${backup_dir}
			done
		else
			for ddr in $(ls board/${soc_family}/common/ddrinit/${soc_family}_${ddr_type}_${ddr_size}_*.abs); do
				log "Building using $ddr"
				ddr_file=$(basename $ddr)
				sed -i "/BR2_EXTERNAL_BOARD_DDRINIT_FILE/s/ddrinit.*/ddrinit\/${ddr_file}\"/" output/.config
				make all
				backup_dir=$(basename $ddr .abs)
				rm -rf images_dailybuild/${defconfig}/${backup_dir}
				mkdir -p images_dailybuild/${defconfig}/${backup_dir}
				cp -rf output/images/for-* images_dailybuild/${defconfig}/${backup_dir}
			done
		fi
	else
		log_fail "Building $defconfig fail!!!"
		daily_build_result="Fail"
	fi
done

if [ "$daily_build_result" == "Fail" ] ; then
	cat images_dailybuild/dailybuild.fail.txt
	exit 1;
fi
