ENVDIR="$(dirname "${BASH_SOURCE[0]}")"

export PATH="${PATH:+${PATH}:}${ENVDIR}/bin"

source ${ENVDIR}/fzf.bash
#export FZF_DEFAULT_COMMAND='fd --type file'
#export FZF_DEFAULT_COMMAND='fd --type file --color=always --hidden --exclude .git --exclude .cscope'
export FZF_DEFAULT_COMMAND='fd --type file --hidden --exclude .git --exclude .cscope --exclude .repo'
export FZF_CTRL_T_COMMAND=$FZF_DEFAULT_COMMAND
export FZF_ALT_C_COMMAND="fd -t d . "

source ${ENVDIR}/z/z.sh

unalias z
z() {
	if [[ -z "$*" ]]; then
		cd "$(_z -l 2>&1 | fzf +s --tac | sed 's/^[0-9,.]* *//')"
	else
		_last_z_args="$@"
			_z "$@"
	fi
}

zz() {
	cd "$(_z -l 2>&1 | sed 's/^[0-9,.]* *//' | fzf -q "$_last_z_args")"
}

# fzf and z is much better than fasd
# [ -f ~/.my/bin/fasd ] && eval "$(fasd --init auto)"

# b) function hcenv_cd
# This function defines a 'cd' replacement function capable of keeping, 
# displaying and accessing history of visited directories, up to 10 entries.
# To use it, uncomment it, source this file and try 'cd --'.
# acd_func 1.0.5, 10-nov-2004
# Petar Marinov, http:/geocities.com/h2428, this is public domain
hcenv_cd ()
{
  local x2 the_new_dir adir index
  local -i cnt
  local last_dir

  if [[ $1 == "---" ]]; then
    last_dir=`cat $HOME/.last_dir`
    if [[ $last_dir == "" ]]; then
      return 0
    fi
    hcenv_cd $last_dir
    return 0
  fi

  if [[ $1 ==  "--" ]]; then
    dirs -v
    return 0
  fi

  the_new_dir=$1
  [[ -z $1 ]] && the_new_dir=$HOME

  if [[ ${the_new_dir:0:1} == '-' ]]; then
    #
    # Extract dir N from dirs
    index=${the_new_dir:1}
    [[ -z $index ]] && index=1
    adir=$(dirs +$index)
    [[ -z $adir ]] && return 1
    the_new_dir=$adir
  fi

  #
  # '~' has to be substituted by ${HOME}
  [[ ${the_new_dir:0:1} == '~' ]] && the_new_dir="${HOME}${the_new_dir:1}"

  #
  # Now change to the new dir and add to the top of the stack
  pushd "${the_new_dir}" > /dev/null
  [[ $? -ne 0 ]] && return 1
  the_new_dir=$(pwd)

  echo $the_new_dir > $HOME/.last_dir

  #
  # Trim down everything beyond 11th entry
  popd -n +11 2>/dev/null 1>/dev/null

  #
  # Remove any other occurence of this dir, skipping the top of the stack
  for ((cnt=1; cnt <= 10; cnt++)); do
    x2=$(dirs +${cnt} 2>/dev/null)
    [[ $? -ne 0 ]] && return 0
    [[ ${x2:0:1} == '~' ]] && x2="${HOME}${x2:1}"
    if [[ "${x2}" == "${the_new_dir}" ]]; then
      popd -n +$cnt 2>/dev/null 1>/dev/null
      cnt=cnt-1
    fi
  done

  return 0
}
alias cd=hcenv_cd

hcenv_check_sub_and_cd()
{
	local path="$1"
	local sub="$2"
	local tsub=""

	if [ "$sub" == "" ]; then
		echo cd $path/$sub
		cd "$path"/$sub
		return 0
	fi

	for tsub in $sub; do
		if [ -d "$path"/$tsub ]; then
			echo cd $path/$tsub
			cd "$path"/$tsub
			return 0
		fi
	done

	return 1
}

hcenv_check_and_cd()
{
	local path="`pwd`"
	local folder=""
	local sub=""
	local check=""

	if [ $# -eq 2 ] ; then
		sub="$2"
	fi

	while [ "$path" != "/" ]; do
		folder=`basename $path`
		dir=`dirname $path`
		for check in $1; do
			a=`echo $folder | grep "$check"`
			b=`ls $dir | grep "$check"`
			if [ "$a" != "" ] || [ "$b" != "" ]; then
				path="`dirname $path`"
				hcenv_check_sub_and_cd "$path" "$sub"
				return 0
			fi
		done
		path="`dirname $path`"
	done

	path="`pwd`"
	for check in $1; do
		b=`ls $path | grep "$check"`
		if [ "$b" != "" ]; then
			hcenv_check_sub_and_cd "$path" "$sub"
			return 0
		fi
	done

	return 1
}

hcenv_is_hclinux()
{
	local path="`pwd`"
	local folder=""
	local sub=""

	while [ "$path" != "/" ]; do
		folder=`basename $path`
		dir=`dirname $path`
		a=`echo $folder | grep -E "buildroot|hclinux"`
		b=`ls $dir | grep "external.desc"`
		if [ "$a" != "" ]; then
			echo TRUE
			return 0
		else
			if [ "$b" != "" ]; then
				echo TRUE
				return 0
			else
				path="`dirname $path`"
			fi
		fi
	done

	path="`pwd`"
	b=`ls $path | grep "external.desc"`
	if [ "$b" != "" ]; then
		echo TRUE
		return 0
	fi

	echo FALSE
	return 1
}

hcenv_cd_top()
{
	local temp=""
	temp=`pwd`
	a=`echo $temp | grep "buildroot"`
	if [ "$a" != "" ]; then
		hcenv_check_and_cd "package"
		if [ $? -eq 0 ]; then
			return
		fi
	fi
	hcenv_check_and_cd "source"
	if [ $? -eq 0 ]; then
		return
	fi
	hcenv_check_and_cd "components"
}

hcenv_cd_top_of_top()
{
	local temp=""
	local a=""
	local notop=""
	local sub=""

	if [ $# -eq 1 ] ; then
		sub="$1"
	fi

	temp=`pwd`
	a=`echo $temp | grep -E "buildroot|hclinux"`
	if [ "$a" != "" ]; then
		hcenv_check_and_cd "external.desc" "$sub"
		if [ $? -eq 0 ]; then
			return
		fi
	fi
	hcenv_check_and_cd "external.desc" "$sub"
	if [ $? -eq 0 ]; then
		return
	fi
	hcenv_check_and_cd "components" "$sub"
}

hcenv_check_sub_and_get()
{
	local path="$1"
	local sub="$2"
	local tsub=""

	if [ "$sub" == "" ]; then
		echo "$path"/$sub
		return 0
	fi

	for tsub in $sub; do
		if [ -d "$path"/$tsub ]; then
			echo "$path"/$tsub
			return 0
		fi
	done

	return 1
}

hcenv_check_and_get()
{
	local path="`pwd`"
	local folder=""
	local sub=""
	local check=""

	if [ $# -eq 2 ] ; then
		sub="$2"
	fi

	while [ "$path" != "/" ]; do
		folder=`basename $path`
		dir=`dirname $path`
		for check in $1; do
			a=`echo $folder | grep "$check"`
			b=`ls $dir | grep "$check"`
			if [ "$a" != "" ] || [ "$b" != "" ]; then
				path="`dirname $path`"
				hcenv_check_sub_and_get "$path" "$sub"
				return 0
			fi
		done
		path="`dirname $path`"
	done

	path="`pwd`"
	for check in $1; do
		b=`ls $path | grep "$check"`
		if [ "$b" != "" ]; then
			hcenv_check_sub_and_get "$path" "$sub"
			return 0
		fi
	done

	return 1
}

hcenv_get_top_of_top()
{
	local temp=""
	local a=""
	local notop=""
	local sub=""

	if [ $# -eq 1 ] ; then
		sub="$1"
	fi

	temp=`pwd`
	a=`echo $temp | grep -E "buildroot|hclinux"`
	if [ "$a" != "" ]; then
		hcenv_check_and_get "external.desc" "$sub"
		if [ $? -eq 0 ]; then
			return
		fi
	fi
	hcenv_check_and_get "external.desc" "$sub"
	if [ $? -eq 0 ]; then
		return
	fi
	hcenv_check_and_get "components" "$sub"
}

alias ..2='cd ../..'
alias ..3='cd ../../..'
alias ..4='cd ../../../..'
alias ..5='cd ../../../../..'
alias ..6='cd ../../../../../..'
alias cdtt=hcenv_cd_top_of_top

getroot()
{
	hcenv_get_top_of_top
}

croot()
{
	cdtt
}

ckernel()
{
	cdtt "buildroot/output/build/linux-4.4.186 buildroot/output/build/linux-5.12.4 components/kernel/source"
}

cbr()
{
	cdtt buildroot
}

cout()
{
	cdtt "buildroot/output output"
}

csource()
{
	cdtt SOURCE
}

cavp()
{
	cdtt SOURCE/avp
}

cavp_kernel()
{
	cdtt SOURCE/avp/components/kernel/source
}

cboot()
{
	cdtt "SOURCE/avp/components/applications/apps-bootloader/source components/applications/apps-bootloader/source"
}

cdriver()
{
	cdtt SOURCE/linux-drivers
}

__mkernel()
{
	local is_linux_sdk="`hcenv_is_hclinux`"
	if [ "$is_linux_sdk" == "TRUE" ]; then
    		make "${@}" linux-rebuild
		[ $? != 0 ] && return 1;
	else
		make "${@}" kernel-clean kernel-rebuild cmds-clean cmds-rebuild
		[ $? != 0 ] && return 1;
	fi
	return 0;
}

__mkboot()
{
	local is_linux_sdk="`hcenv_is_hclinux`"
	if [ "$is_linux_sdk" == "TRUE" ]; then
		make "${@}" hcboot-apps-bootloader-dirclean hcboot-kernel-clean hcboot-kernel-rebuild hcboot-apps-bootloader-clean hcboot-apps-bootloader-rebuild hcboot-all hcboot-rebuild
		[ $? != 0 ] && return 1;
	else
		make "${@}" O=output-bl apps-bootloader-dirclean kernel-clean kernel-rebuild apps-bootloader-clean apps-bootloader-rebuild all
		[ $? != 0 ] && return 1;
	fi
	return 0;
}

__mkavp()
{
	local is_linux_sdk="`hcenv_is_hclinux`"
	if [ "$is_linux_sdk" == "TRUE" ]; then
    		make "${@}" avp-kernel-clean avp-kernel-rebuild avp-cmds-clean avp-cmds-rebuild avp-all avp-rebuild
		[ $? != 0 ] && return 1;
	fi
	return 0;
}

mkboot()
{
    __mkboot "${@}"
    [ $? != 0 ] && return 1;
    make "${@}" all
}

mkernel()
{
    __mkernel "${@}"
    [ $? != 0 ] && return 1;
    make "${@}" all
}

mkavp()
{
    __mkavp "${@}"
    [ $? != 0 ] && return 1;
    make "${@}" all
}

mkall()
{
    __mkboot "${@}"
    [ $? != 0 ] && return 1;
    __mkavp "${@}"
    [ $? != 0 ] && return 1;
    __mkernel "${@}"
    [ $? != 0 ] && return 1;
    make "${@}" all
}

hconfig_hclinux()
{
	local defconfig=""

	defconfig=$(ls configs | fzf -e)
	if [ "${defconfig}" == "" ] ; then
		return
	fi

	echo "BOARD defconfig is using configs/${defconfig}"

	make -C buildroot "${@}" BR2_EXTERNAL=$PWD $defconfig
}

hbuild_hclinux()
{
	local defconfig=""

	defconfig=$(ls configs | fzf -e)
	if [ "${defconfig}" == "" ] ; then
		return
	fi

	echo "BOARD defconfig is using configs/${defconfig}"

	make -C buildroot "${@}" BR2_EXTERNAL=$PWD $defconfig
	make -C buildroot "${@}" all
}

hconfig_hcrtos()
{
	local solution=""
	local defconfig=""
	local bl_defconfig=""

	solution=$(ls configs | grep "_bl_defconfig$" | sed 's/_bl_defconfig//' | fzf -e)
	defconfig=${solution}_defconfig
	bl_defconfig=${solution}_bl_defconfig

	if [ "${solution}" == "" ] ; then
		return
	fi

	echo "BOARD defconfig is using configs/{${bl_defconfig}, ${defconfig}"

	make "${@}" O=output-bl ${bl_defconfig}
	make "${@}" ${defconfig}
}

hbuild_hcrtos()
{
	local solution=""
	local defconfig=""
	local bl_defconfig=""

	solution=$(ls configs | grep "_bl_defconfig$" | sed 's/_bl_defconfig//' | fzf -e)
	defconfig=${solution}_defconfig
	bl_defconfig=${solution}_bl_defconfig

	if [ "${solution}" == "" ] ; then
		return
	fi

	echo "BOARD defconfig is using configs/{${bl_defconfig}, ${defconfig}"

	make "${@}" O=output-bl ${bl_defconfig}
	make "${@}" ${defconfig}
	make "${@}" O=output-bl all
	[ $? != 0 ] && return 1;
	make "${@}" all
}

hconfig()
{
	local is_linux_sdk="`hcenv_is_hclinux`"
	if [ "$is_linux_sdk" == "TRUE" ]; then
		hconfig_hclinux "${@}"
	else
		hconfig_hcrtos "${@}"
	fi
}

hbuild()
{
	local is_linux_sdk="`hcenv_is_hclinux`"
	if [ "$is_linux_sdk" == "TRUE" ]; then
		hbuild_hclinux "${@}"
	else
		hbuild_hcrtos "${@}"
	fi
}

hmkmk()
{
	local currentDir=`pwd`
	local rootDir=`getroot`
	if [ "$rootDir" == "" ]; then
		echo "No sdk root dir found, output Makefile failed!"
	fi
	make -C $rootDir outputmakefile SOLUTION=$currentDir
}

source ${ENVDIR}/hrepo
