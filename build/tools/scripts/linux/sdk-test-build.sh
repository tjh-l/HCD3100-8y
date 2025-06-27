#!/bin/bash
#-----------------------------------------------------------------------
# FUNCTION: usage
# DESCRIPTION:  Display usage information.
#-----------------------------------------------------------------------
function usage() {
    cat << EOT

Usage : ${0} [OPTION] ...
  Create a build from given options.

Options:
  -h, --help                    Display this message
  -i, --id                      Build id
  -n, --name                    Build name
  -e, --email                   Notification email address
  -c, --commit                  Git commit id
  -s, --configs                 Build configs
  -b, --callback                Callback api url

Exit status:
  0   if OK,
  !=0 if serious problems.

Example:
  1) Use short options to create vm:
    $ sudo ${0} -a test -b test@mail.com

  2) Use long options to create vm:
    $ sudo ${0} --arg1 test -arg2 test@mail.com

Report bugs to evon.du@hichiptech.com

EOT
}
# ----------  end of function usage  -----------------------------------

#-----------------------------------------------------------------------
# FUNCTION: log
# DESCRIPTION:  Display and save log information.
#-----------------------------------------------------------------------
function log(){
  # 颜色变量
  MSG_INFO="\033[1;36m"
  MSG_END="\033[0m"
  # 显示输出
  if [ "${MSG_COLOUR_ENABLED}" == "y" ]
  then
    echo -e "${MSG_INFO}${1}${MSG_END}"
  else
    echo "${1}"
  fi
  # 文件输出
	echo -e "${1}" >> buildlogs/build.result.txt
}
# ----------  end of function log  -------------------------------------

#-----------------------------------------------------------------------
# FUNCTION: log_fail
# DESCRIPTION:  Display and save fail log information.
#-----------------------------------------------------------------------
function log_fail(){
  # 颜色变量
  MSG_INFO="\033[1;31m"
  MSG_END="\033[0m"
  # 显示输出
  if [ "${MSG_COLOUR_ENABLED}" == "y" ]
  then
    echo -e "${MSG_INFO}${1}${MSG_END}"
  else
    echo "${1}"
  fi
  # 文件输出
	echo -e "${1}" >> buildlogs/build.fail.txt
}
# ----------  end of function log_fail  --------------------------------

#-----------------------------------------------------------------------
# ACTION: options
# DESCRIPTION:  Load options information.
#-----------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    key="$1"
    shift
    val="$1"
    if [ "${val:0:1}" == "-" ]
    then
      val=""
    else
      shift
    fi
    case "$key" in
        -h|--help)
            usage
            exit
            ;;
        -i|--id)
            SCRIPT_ID="${val}"
            ;;
        -n|--name)
            SCRIPT_NAME="${val}"
            ;;
        -e|--email)
            SCRIPT_EMAIL="${val}"
            ;;
        -c|--commit)
            SCRIPT_COMMIT="${val}"
            ;;
        -s|--configs)
            SCRIPT_CONFIGS="${val}"
            ;;
        -b|--callback)
            SCRIPT_CALLBACK="${val}"
            ;;
        *)
            echo "Unknown option: $key"
            ;;
    esac
done
# ----------  end of load options  -----------------------------------

# 输出空行
echo ""

# 输出参数
log "------------------------------------------------------------------------"
log "[TAG] params"
log "* id:$SCRIPT_ID"
log "* name:$SCRIPT_NAME"
log "* email:$SCRIPT_EMAIL"
log "* commit:$SCRIPT_COMMIT"
log "* configs:$SCRIPT_CONFIGS"
log "* callback:$SCRIPT_CALLBACK"
log "------------------------------------------------------------------------"
log ""

# 环境参数
log "------------------------------------------------------------------------"
log "[TAG] environments"
log "* pwd:$(pwd)"
log "------------------------------------------------------------------------"
log ""

# 参数判断
if [ "${SCRIPT_COMMIT}" == "" ] || [ "${SCRIPT_CONFIGS}" == "" ]
then
  log "[ERROR] params error"
  exit
fi

# 初始变量
ddr_clk=""
ddr_type=""
ddr_size=""
ddr_sip=""
daily_build_result="Success"

# 创建目录
rm -rf images_dailybuild
mkdir images_dailybuild

# 创建日志
rm -rf buildlogs
mkdir buildlogs
touch buildlogs/build.result.txt
touch buildlogs/build.fail.txt

# 回调通知
if [ "${SCRIPT_CALLBACK}" != "" ]
then
  log "------------------------------------------------------------------------"
  log "[TAG] callback building"
  log "[COMMAND] curl \"${SCRIPT_CALLBACK}\" --data \"{\"build\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${SCRIPT_NAME}\",\"email\":\"${SCRIPT_EMAIL}\",\"commit\":\"${SCRIPT_COMMIT}\",\"configs\":\"${SCRIPT_CONFIGS}\",\"state\":\"building\"}\""
  curl "${SCRIPT_CALLBACK}" --data "{\"build\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${SCRIPT_NAME}\",\"email\":\"${SCRIPT_EMAIL}\",\"commit\":\"${SCRIPT_COMMIT}\",\"configs\":\"${SCRIPT_CONFIGS}\",\"state\":\"building\"}"
  echo ""
  log "------------------------------------------------------------------------"
  log ""
fi

# 指定修订
log "------------------------------------------------------------------------"
log "[TAG] git checkout"
log "[COMMAND] git checkout ${SCRIPT_COMMIT}"
git checkout ${SCRIPT_COMMIT}
log "[COMMAND] git submodule update --init --recursive --force"
git submodule update --init --recursive --force
log "------------------------------------------------------------------------"
log ""

# 建立归档目录
log "------------------------------------------------------------------------"
log "[TAG] create jenkins archive"
archive_jenkins="./archive"
log "[COMMAND] mkdir -p ${archive_jenkins}"
mkdir -p ${archive_jenkins}
rm -rf ${archive_jenkins}/*
log "------------------------------------------------------------------------"
log ""

# 执行结果保存值
exec_result=0

# 遍历配置进行编译
configs_arr=(${SCRIPT_CONFIGS//;/ });
for config in ${configs_arr[@]}
do
  # 先进行字符转义(不然空字符串会被直接忽略)
  config_escape="_${config//,/,_}";

  # 分割配置编译项
  data_arr=(${config_escape//,/ });

  # 选项变量定义(注意要移除转义时添加的首个下划线)
  item_name=${data_arr[0]:1:${#data_arr[0]}}
  item_alias=${data_arr[1]:1:${#data_arr[1]}}
  item_wifis=${data_arr[2]:1:${#data_arr[2]}}
  item_patch=${data_arr[3]:1:${#data_arr[3]}}
  item_options=${data_arr[4]:1:${#data_arr[4]}}

  # 开始执行配置编译
  log "------------------------------------------------------------------------"
  log "[TAG] building"
  log "[BUILDING] ${item_name}:"

  # 还原仓库
  if [ "${SKIP_GIT_STASH}" == "y" ]
  then
    log "[SKIP] git stash"
  else
  	#还原项目
    log "[COMMAND] git stash"
    git stash
    #清理文件
    #log "[COMMAND] git clean -dfe archive"
    #git clean -dfe archive
    #还原子模块
    grep -oP 'path = \K.+' .gitmodules | while read submodule_path; do
        if [ -d "$submodule_path" ];then
    		log "[COMMAND] <$submodule_path> git checkout ."
        	(cd "$submodule_path" && git checkout . && cd -)
        else
        	log "[NOTFOUNT] <$submodule_path>"
        fi
    done
  fi

  # 压缩补丁
  if [ "${item_patch}" != "" ]
  then
    # 下载包
    log "[DOWNLOAD] curl $item_patch >> jenkins.patch.zip"
    rm -rf jenkins.patch.zip
    curl $item_patch >> jenkins.patch.zip
    echo ""
    # 解压包
    log "[ZIP] unzip -o ./jenkins.patch.zip"
    unzip -o ./jenkins.patch.zip
    echo ""
  fi

  # 执行原本的编译脚本的开始部分
  defconfig="${item_name}_defconfig"
	ddr_clk=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep MHz)
	ddr_type=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep ddr[2,3])
	ddr_size=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i++){print $i}}' | grep -E "64M|128M|256M")
	ddr_sip=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep sip)
	soc_family=$(cat configs/${defconfig} | grep BR2_EXTERNAL_BOARD_DDRINIT_FILE | awk -F "[_:.:/]" '{for (i=1;i<=NF;i=i+1){print $i}}' | grep -m1 hc1[5,6]xx)
	rm -rf buildroot/output
	make -C buildroot BR2_EXTERNAL=$PWD $defconfig
	[ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break

  # 添加附加选项备注头
  echo "" >> buildroot/.config
  echo "#" >> buildroot/.config
  echo "# Build web options" >> buildroot/.config
  echo "#" >> buildroot/.config

  # 遍历处理编译选项
  options_arr=(${item_options//|/ });
  for option in ${options_arr[@]}
  do
    # 提取键值信息
    option_arr=(${option//=/ });
    option_key=${option_arr[0]}
    option_value=${option_arr[1]}
    # 查询是否有此内容
    log "[MODIFY] modify config option: ${option_key}"
    if cat buildroot/.config | egrep ".*${option_key}.*"
    then
      # 存在配置则替换
      sed -i "s/.*${option_key}.*/${option_key}=${option_value}/g" buildroot/.config
      cat buildroot/.config | egrep ".*${option_key}.*"
    else
      # 不存在则末尾新增
      echo "${option_key}=${option_value}" >> buildroot/.config
      cat buildroot/.config | egrep ".*${option_key}.*"
    fi
  done

  # 判断是否编译多WiFi
  if [ "${item_wifis}" == "y" ]
  then
    # 设置BR2_PACKAGE_RTL8733BU_WIFI
    log "[MODIFY] enabled BR2_PACKAGE_RTL8733BU"
    sed -i 's/.*BR2_PACKAGE_RTL8733BU.*/BR2_PACKAGE_RTL8733BU=y/g' buildroot/.config
    cat output/.config | egrep '.*BR2_PACKAGE_RTL8733BU.*'

    # 设置BR2_PACKAGE_RTL8188FU_WIFI
    log "[MODIFY] enabled BR2_PACKAGE_RTL8188FU"
    sed -i 's/.*BR2_PACKAGE_RTL8188FU.*/BR2_PACKAGE_RTL8188FU=y/g' buildroot/.config
    cat output/.config | egrep '.*BR2_PACKAGE_RTL8188FU.*'
  fi

  # 执行原本的编译脚本的后面部分
  make -C buildroot all
  [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break

  # 获取DDR信息
  cat configs/${defconfig} | egrep '.*BR2_EXTERNAL_BOARD_DDRINIT_FILE.*'
  ddr="$(cat configs/${defconfig} | sed -n 's/BR2_EXTERNAL_BOARD_DDRINIT_FILE="\${BR2_EXTERNAL_HCLINUX_PATH}\/\(.*\)"/\1/p')"
  ddr="$(echo ${ddr} | sed 's/[^[:print:]]//g')"
  log "[DDR] $defconfig DDR: $ddr"

  # 进行DDR编译
  if [ "${ddr_sip}" == "sip" ]; then
    log "Building using $ddr"
    ddr_file=$(basename $ddr)
    sed -i "/BR2_EXTERNAL_BOARD_DDRINIT_FILE/s/ddrinit.*/ddrinit\/${ddr_file}\"/" buildroot/.config
    make -C buildroot all
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    backup_dir=$(basename $ddr .abs)
    rm -rf images_dailybuild/${defconfig}/${backup_dir}
    mkdir -p images_dailybuild/${defconfig}/${backup_dir}
    cp -rf buildroot/output/images/for-* images_dailybuild/${defconfig}/${backup_dir}
  else
    log "Building using $ddr"
    ddr_file=$(basename $ddr)
    sed -i "/BR2_EXTERNAL_BOARD_DDRINIT_FILE/s/ddrinit.*/ddrinit\/${ddr_file}\"/" buildroot/.config
    make -C buildroot all
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    backup_dir=$(basename $ddr .abs)
    rm -rf images_dailybuild/${defconfig}/${backup_dir}
    mkdir -p images_dailybuild/${defconfig}/${backup_dir}
    cp -rf buildroot/output/images/for-* images_dailybuild/${defconfig}/${backup_dir}
  fi

  #if [ -f buildroot/output/images/for-debug/HCFOTA.bin ] ; then
  	#if [ "${ddr_sip}" == "sip" ]; then
  	#	for ddr in $(ls board/hichip/${soc_family}/common/ddrinit/${soc_family}_sip_${ddr_type}_${ddr_size}_*.abs); do
  	#		log "Building using $ddr"
  	#		ddr_file=$(basename $ddr)
  	#		sed -i "/BR2_EXTERNAL_BOARD_DDRINIT_FILE/s/ddrinit.*/ddrinit\/${ddr_file}\"/" buildroot/.config
  	#		make -C buildroot all
  	#		backup_dir=$(basename $ddr .abs)
  	#		rm -rf images_dailybuild/${defconfig}/${backup_dir}
  	#		mkdir -p images_dailybuild/${defconfig}/${backup_dir}
  	#		cp -rf buildroot/output/images/for-* images_dailybuild/${defconfig}/${backup_dir}
  	#	done
  	#else
  	#	for ddr in $(ls board/hichip/${soc_family}/common/ddrinit/${soc_family}_${ddr_type}_${ddr_size}_*.abs); do
  	#		log "Building using $ddr"
  	#		ddr_file=$(basename $ddr)
  	#		sed -i "/BR2_EXTERNAL_BOARD_DDRINIT_FILE/s/ddrinit.*/ddrinit\/${ddr_file}\"/" buildroot/.config
  	#		make -C buildroot all
  	#		backup_dir=$(basename $ddr .abs)
  	#		rm -rf images_dailybuild/${defconfig}/${backup_dir}
  	#		mkdir -p images_dailybuild/${defconfig}/${backup_dir}
  	#		cp -rf buildroot/output/images/for-* images_dailybuild/${defconfig}/${backup_dir}
  	#	done
  	#fi
  #else
  	#log_fail "Building $defconfig fail!!!"
  	#daily_build_result="Fail"
  #fi

  # 执行原本脚本的编译结果判断
  #if [ "$daily_build_result" == "Fail" ] ; then
  #	cat buildlogs/dailybuild.fail.txt
  #	exit 1;
  #fi

  # 建立镜像目录
  images_archive_item="${archive_jenkins}/${item_name}_${item_alias}"
  log "[COMMAND] mkdir ${images_archive_item}"
  mkdir -p ${images_archive_item}

  # 保存镜像
  log "[COMMAND] cp -r buildroot/output/images ${images_archive_item}/images"
  cp -r buildroot/output/images ${images_archive_item}/images

  # 输出遍历块分割线
  log "------------------------------------------------------------------------"
  log ""
done

# 写入仓库日志
log "------------------------------------------------------------------------"
log "[TAG] wire git log"
log "[COMMAND] git log -1 >> git.log.txt"
git log -1 >> buildlogs/git.log.txt
log "------------------------------------------------------------------------"
log ""

# 进行日志归档
log "------------------------------------------------------------------------"
log "[TAG] log archive"
log "[COMMAND] cp -r buildlogs ${archive_jenkins}/buildlogs"
cp -r buildlogs ${archive_jenkins}/buildlogs
log "------------------------------------------------------------------------"
log ""

# 回调通知
if [ "${SCRIPT_CALLBACK}" != "" ]
then
  log "------------------------------------------------------------------------"
  log "[TAG] callback completed"
  log "[COMMAND] curl \"${SCRIPT_CALLBACK}\" --data \"{\"id\":\"${SCRIPT_ID}\",\"build_id\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${SCRIPT_NAME}\",\"email\":\"${SCRIPT_EMAIL}\",\"commit\":\"${SCRIPT_COMMIT}\",\"configs\":\"${SCRIPT_CONFIGS}\",\"state\":\"completed\"}\""
  curl "${SCRIPT_CALLBACK}" --data "{\"id\":\"${SCRIPT_ID}\",\"build_id\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${SCRIPT_NAME}\",\"email\":\"${SCRIPT_EMAIL}\",\"commit\":\"${SCRIPT_COMMIT}\",\"configs\":\"${SCRIPT_CONFIGS}\",\"state\":\"completed\"}"
  log "------------------------------------------------------------------------"
  log ""
fi

# 返回结果
if [ ${exec_result} -ne 0 ]
then
  log "[RESULT] FAIL"
  echo ""
  exit 1;
else
  log "[RESULT] SUCCESS"
  echo ""
  exit 0;
fi