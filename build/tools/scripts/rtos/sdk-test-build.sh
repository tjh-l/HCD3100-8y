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
	echo -e "${1}" >> ./buildlogs/build.result.txt
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
	echo -e "${1}" >> ./buildlogs/build.fail.txt
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
        -d|--debug-archive)
            SCRIPT_DEBUG_ARCHIVE="${val}"
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
log "* debug-archive:$SCRIPT_DEBUG_ARCHIVE"
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
  log ""
  exit
fi

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
  log "[COMMAND] curl \"${SCRIPT_CALLBACK}\" --data \"{\"id\":\"${SCRIPT_ID}\",\"build_id\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${SCRIPT_NAME}\",\"email\":\"${SCRIPT_EMAIL}\",\"commit\":\"${SCRIPT_COMMIT}\",\"configs\":\"${SCRIPT_CONFIGS}\",\"state\":\"building\"}\""
  curl "${SCRIPT_CALLBACK}" --data "{\"id\":\"${SCRIPT_ID}\",\"build_id\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${SCRIPT_NAME}\",\"email\":\"${SCRIPT_EMAIL}\",\"commit\":\"${SCRIPT_COMMIT}\",\"configs\":\"${SCRIPT_CONFIGS}\",\"state\":\"building\"}"
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

# 建立镜像归档目录
log "------------------------------------------------------------------------"
log "[TAG] create jenkins archive"
archive_jenkins="./archive"
log "[COMMAND] mkdir -p ${archive_jenkins}"
mkdir -p ${archive_jenkins}
rm -rf archive/*
log "------------------------------------------------------------------------"
log ""

# 建立调试归档目录
log "------------------------------------------------------------------------"
log "[TAG] create debug archive"
archive_debug=""
if [ "${SCRIPT_DEBUG_ARCHIVE}" != "" ]
then
  archive_debug="${SCRIPT_DEBUG_ARCHIVE}/${BUILD_ID}"
  log "[COMMAND] mkdir -p ${archive_debug}"
  mkdir -p ${archive_debug}
fi
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

  # 执行子编译脚本
  log "------------------------------------------------------------------------"
  log "[TAG] building"
  log "[BUILDING] ${item_name}:"

  # 清理上一次编译
  if [ "${SKIP_OUTPUT_CLEAR}" == "y" ]
  then
    log "[SKIP] clear output"
  else
    log "[TAG] clear output"
    log "[COMMAND] rm -rf output"
    rm -rf output
    log "[COMMAND] rm -rf bl_output"
    rm -rf bl_output
  fi

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

  # 执行BootLoad编译
  log "[COMMAND] make O=bl_output ${item_name}_bl_defconfig"
  make O=bl_output ${item_name}_bl_defconfig
  [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
  log "[COMMAND] make O=bl_output all"
  make O=bl_output all
  [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break

  # 执行Config编译
  log "[COMMAND] make ${item_name}_defconfig"
  make ${item_name}_defconfig
  [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break

  # 修改output/.config权限
  chmod 777 output/.config

  # 遍历处理编译选项
  options_arr=(${item_options//|/ });
  echo "" >> output/.config
  echo "#" >> output/.config
  echo "# Build web options" >> output/.config
  echo "#" >> output/.config
  for option in ${options_arr[@]}
  do
    # 提取键值信息
    option_arr=(${option//=/ });
    option_key=${option_arr[0]}
    option_value=${option_arr[1]}
    # 查询是否有此内容
    log "[MODIFY] modify config option: ${option_key}"
    if cat output/.config | egrep ".*${option_key}.*"
    then
      # 存在配置则替换
      sed -i "s/.*${option_key}.*/${option_key}=${option_value}/g" output/.config
      cat output/.config | egrep ".*${option_key}.*"
    else
      # 不存在则末尾新增
      echo "${option_key}=${option_value}" >> output/.config
      cat output/.config | egrep ".*${option_key}.*"
    fi
  done

  # 建立项目镜像归档目录
  archive_jenkins_item="${archive_jenkins}/${item_name}_${item_alias}"
  log "[COMMAND] mkdir -p ${archive_jenkins_item}"
  mkdir -p ${archive_jenkins_item}

  # 建立项目调试归档目录
  archive_debug_item=""
  if [ "${archive_debug}" != "" ]
  then
    archive_debug_item="${archive_debug}/${item_name}_${item_alias}"
    log "[COMMAND] mkdir -p ${archive_debug_item}"
    mkdir -p ${archive_debug_item}
  fi

  # 判断是否编译多WiFi
  if [ "${item_wifis}" == "y" ]
  then
    # 备份配置
    cp output/.config output/.config.backup

    # 编译RTL8188FU_WiFi
    log "[MODIFY] enabled RTL8188FU"
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8188FU.*/BR2_PACKAGE_PREBUILTS_RTL8188FU=y/g' output/.config
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8188EU.*/# BR2_PACKAGE_PREBUILTS_RTL8188EU is not set/g' output/.config
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8733BU.*/# BR2_PACKAGE_PREBUILTS_RTL8733BU is not set/g' output/.config
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8188FU.*'
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8188EU.*'
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8733BU.*'
    log "[COMMAND] make O=output syncconfig"
    make O=output syncconfig
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    log "[COMMAND] make all"
    make all
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    # 保存镜像
    log "[COMMAND] cp -r output/images ${archive_jenkins_item}/images_RTL8188FU"
    cp -r output/images ${archive_jenkins_item}/images_RTL8188FU
    # 保存调试
    if [ "${archive_debug_item}" != "" ]
    then
      log "[COMMAND] zip -r ${archive_debug_item}/output_RTL8188FU.zip output"
      zip -rP hichip1234 ${archive_debug_item}/output_RTL8188FU.zip output
    fi

    # 编译RTL8188EU_WiFi
    log "[MODIFY] enabled RTL8188EU"
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8188FU.*/# BR2_PACKAGE_PREBUILTS_RTL8188FU is not set/g' output/.config
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8188EU.*/BR2_PACKAGE_PREBUILTS_RTL8188EU=y/g' output/.config
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8733BU.*/# BR2_PACKAGE_PREBUILTS_RTL8733BU is not set/g' output/.config
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8188FU.*'
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8188EU.*'
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8733BU.*'
    log "[COMMAND] make O=output syncconfig"
    make O=output syncconfig
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    log "[COMMAND] make all"
    make all
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    # 保存镜像
    log "[COMMAND] cp -r output/images ${archive_jenkins_item}/images_RTL8188EU"
    cp -r output/images ${archive_jenkins_item}/images_RTL8188EU
    # 保存调试
    if [ "${archive_debug_item}" != "" ]
    then
      log "[COMMAND] zip -r ${archive_debug_item}/output_RTL8188EU.zip output"
      zip -rP hichip1234 ${archive_debug_item}/output_RTL8188EU.zip output
    fi

    # 编译RTL8733BU_WiFi
    log "[MODIFY] enabled RTL8733BU"
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8188FU.*/# BR2_PACKAGE_PREBUILTS_RTL8188FU is not set/g' output/.config
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8188EU.*/# BR2_PACKAGE_PREBUILTS_RTL8188EU is not set/g' output/.config
    sed -i 's/.*BR2_PACKAGE_PREBUILTS_RTL8733BU.*/BR2_PACKAGE_PREBUILTS_RTL8733BU=y/g' output/.config
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8188FU.*'
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8188EU.*'
    cat output/.config | egrep '.*BR2_PACKAGE_PREBUILTS_RTL8733BU.*'
    log "[COMMAND] make O=output syncconfig"
    make O=output syncconfig
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    log "[COMMAND] make all"
    make all
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    # 保存镜像
    log "[COMMAND] cp -r output/images ${archive_jenkins_item}/images_RTL8733BU"
    cp -r output/images ${archive_jenkins_item}/images_RTL8733BU
    # 保存调试
    if [ "${archive_debug_item}" != "" ]
    then
      log "[COMMAND] zip -r ${archive_debug_item}/output_RTL8733BU.zip output"
      zip -rP hichip1234 ${archive_debug_item}/output_RTL8733BU.zip output
    fi

    # 恢复配置
    mv output/.config.backup output/.config
  else
    # 执行编译
    log "[COMMAND] make O=output syncconfig"
    make O=output syncconfig
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    log "[COMMAND] make all"
    make all
    [ $? -ne 0 ] && log "[ERROR] exec fail" && exec_result=1 && log "------------------------------------------------------------------------" && log "" && break
    # 保存镜像
    log "[COMMAND] cp -r output/images ${archive_jenkins_item}/images"
    cp -r output/images ${archive_jenkins_item}/images
    # 保存调试
    if [ "${archive_debug_item}" != "" ]
    then
      log "[COMMAND] zip -r ${archive_debug_item}/output.zip output"
      zip -rP hichip1234 ${archive_debug_item}/output.zip output
    fi
  fi;

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