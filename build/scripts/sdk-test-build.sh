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
  # 文件输出
	echo -e "$1" >> buildlogs/build.result.txt
  fi
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
  # 文件输出
	echo -e "$1" >> buildlogs/build.fail.txt
  fi
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
        -n|--name)
            name="${val}"
            ;;
        -e|--email)
            email="${val}"
            ;;
        -c|--commit)
            commit="${val}"
            ;;
        -s|--configs)
            configs="${val}"
            ;;
        -b|--callback)
            callback="${val}"
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
log "[TAG] params"
log "* name:$name"
log "* email:$email"
log "* commit:$commit"
log "* configs:$configs"
log "* callback:$callback"
log ""

# 环境参数
log "[TAG] environments"
log "* pwd:$(pwd)"
log ""

# 参数判断
if [ "${commit}" == "" ] || [ "${configs}" == "" ]
then
  log "[ERROR] params error"
  exit
fi

# 创建日志
rm -rf buildlogs
mkdir buildlogs
touch buildlogs/build.result.txt
touch buildlogs/build.fail.txt

# 回调通知
if [ "${callback}" != "" ]
then
  log "[CALLBACK] building"
  log "[COMMAND] curl \"${callback}\" --data \"{\"build\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${name}\",\"email\":\"${email}\",\"commit\":\"${commit}\",\"configs\":\"${configs}\",\"state\":\"building\"}\""
  curl "${callback}" --data "{\"build\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${name}\",\"email\":\"${email}\",\"commit\":\"${commit}\",\"configs\":\"${configs}\",\"state\":\"building\"}"
  echo ""
fi

# 指定修订
log "[COMMAND] git checkout ${commit}"
git checkout $commit
log "[COMMAND] git submodule update --init --recursive --force"
git submodule update --init --recursive --force
echo ""

# 建立归档目录
mkdir archive
rm -rf archive/*

# 遍历配置进行编译
configs_arr=(${configs//;/ });
for config in ${configs_arr[@]}
do
  # 分割配置编译项
  data_arr=(${config//,/ });

  # 选项变量定义
  item_name=${data_arr[0]}
  item_alias=${data_arr[1]}
  item_wifis=${data_arr[2]}
  item_patch=${data_arr[3]}
  item_options=${data_arr[4]}

  # 执行子编译脚本
  log "[BUILDING] ${item_name}:"
  log "------------------------------------------------------------------------"

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
    log "[COMMAND] git stash"
    git stash
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
  log "[COMMAND] make O=bl_output all"
  make O=bl_output all

  # 执行Config编译
  log "[COMMAND] make ${item_name}_defconfig"
  make ${item_name}_defconfig

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

  # 建立镜像目录
  item_archive="archive/${item_name}_${item_alias}"
  log "[COMMAND] mkdir ${item_archive}"
  mkdir ${item_archive}
  log "[COMMAND] mkdir ${item_archive}/debug"
  mkdir ${item_archive}/debug

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
    log "[COMMAND] make all"
    make all
    # 保存镜像
    log "[COMMAND] cp -r output/images ${item_archive}/images_RTL8188FU"
    cp -r output/images ${item_archive}/images_RTL8188FU
    # 保存调试
    log "[COMMAND] zip -r ${item_archive}/debug/output_RTL8188FU.zip output"
    zip -rP hichip1234 ${item_archive}/debug/output_RTL8188FU.zip output

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
    log "[COMMAND] make all"
    make all
    # 保存镜像
    log "[COMMAND] cp -r output/images ${item_archive}/images_RTL8188EU"
    cp -r output/images ${item_archive}/images_RTL8188EU
    # 保存调试
    log "[COMMAND] zip -r ${item_archive}/debug/output_RTL8188EU.zip output"
    zip -rP hichip1234 ${item_archive}/debug/output_RTL8188EU.zip output

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
    log "[COMMAND] make all"
    make all
    # 保存镜像
    log "[COMMAND] cp -r output/images ${item_archive}/images_RTL8733BU"
    cp -r output/images ${item_archive}/images_RTL8733BU
    # 保存调试
    log "[COMMAND] zip -r ${item_archive}/debug/output_RTL8733BU.zip output"
    zip -rP hichip1234 ${item_archive}/debug/output_RTL8733BU.zip output

    # 恢复配置
    mv output/.config.backup output/.config
  else
    # 执行编译
    log "[COMMAND] make O=output syncconfig"
    make O=output syncconfig
    log "[COMMAND] make all"
    make all
    # 保存镜像
    log "[COMMAND] cp -r output/images ${item_archive}/images"
    cp -r output/images ${item_archive}/images
    # 保存调试
    log "[COMMAND] zip -r ${item_archive}/debug/output.zip output"
    zip -rP hichip1234 ${item_archive}/debug/output.zip output
  fi;

  log "------------------------------------------------------------------------"
  echo ""
done

# 写入仓库日志
log "[COMMAND] git log -1 >> git.log.txt"
git log -1 >> buildlogs/git.log.txt

# 进行日志归档
log "[COMMAND] cp -r buildlogs archive/buildlogs"
cp -r buildlogs archive/buildlogs
echo ""

# 回调通知
if [ "${callback}" != "" ]
then
  log "[CALLBACK] completed"
  log "[COMMAND] curl \"${callback}\" --data \"{\"build\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${name}\",\"email\":\"${email}\",\"commit\":\"${commit}\",\"configs\":\"${configs}\",\"state\":\"completed\"}\""
  curl "${callback}" --data "{\"build\":\"${BUILD_ID}\",\"build_url\":\"${BUILD_URL}\",\"name\":\"${name}\",\"email\":\"${email}\",\"commit\":\"${commit}\",\"configs\":\"${configs}\",\"state\":\"completed\"}"
  echo ""
fi

# 结束换行
echo ""