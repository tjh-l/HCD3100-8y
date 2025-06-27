#!/bin/bash

CURRENT_DIR=`pwd`
LOCAL_DIR=`dirname $0`

PROJECT_PATH=${CURRENT_DIR}
CURRENT_TIME=`date +%Y%m%d%H`

PATCH_DESCRIBE=$1
GIT_REPOSITORY=$2
START_COMMITID=$3
END_COMMITID=$4

PATCH_SOURCE_DIR=${PROJECT_PATH}/patch_source
PATCH_SOURCE_DIR="${PATCH_SOURCE_DIR}"/"${PATCH_DESCRIBE}"_"${CURRENT_TIME}"
mkdir -p $PATCH_SOURCE_DIR

SOURCE_DIR=""
PATCH_DIR=""
GIT_FILE=""
COMMIT_FILE=""
README=""

#
#./support/scripts/create_git_patch.sh "hclinux-2022.08.y-支持FM25M4AA_nor_flash" "./" "cce09b6d95da2d9a6f142bfad1aae0e27f29988b" "37b771ec18474eaa4c975626fa7467e123734710"
#./support/scripts/create_git_patch.sh "hclinux-2022.08.y-支持FM25M4AA_nor_flash" "SOURCE/avp" "749cf879e8c14bb1f49a69e892ce592a6a2a9f25" "ad210802cda05d9fb0708be1a5738838d51055e4"
#./support/scripts/create_git_patch.sh "hclinux-2022.08.y-支持FM25M4AA_nor_flash" "SOURCE/avp/components/kernel/source" "317ec0b22082a01f69c4d3d73c105f6ea29ad0d9" "c59cd31462ba41f09d82ec263bb0990ca48661dc"
#

function get_help()
{
	echo "Usage:"
	echo "    ./create_git_patch.sh [patch describe] [git repository path] [start commit id] [end commit id]"
	echo ""
	echo "eg:"
	echo "    ./support/scripts/create_git_patch.sh \"Solve the problem of compilation failure\"  \"SOURCE/hc-examples\" xxx xxx"
	echo "    ./support/scripts/create_git_patch.sh \"hclinux-2022.08.y-支持FM25M4AA_nor_flash\" \"./\" \"cce09b6d95da2d9a6f142bfad1aae0e27f29988b\" \"37b771ec18474eaa4c975626fa7467e123734710\" "
	echo "    ./support/scripts/create_git_patch.sh \"hclinux-2022.08.y-支持FM25M4AA_nor_flash\" \"SOURCE/avp\" \"749cf879e8c14bb1f49a69e892ce592a6a2a9f25\" \"ad210802cda05d9fb0708be1a5738838d51055e4\" "
	echo "    ./support/scripts/create_git_patch.sh \"hclinux-2022.08.y-支持FM25M4AA_nor_flash\" \"SOURCE/avp/components/kernel/source\" \"317ec0b22082a01f69c4d3d73c105f6ea29ad0d9\" \"c59cd31462ba41f09d82ec263bb0990ca48661dc\" "
	echo ""
}

function prepare_env_variables()
{
	GIT_REPOSITORY=$1
	START_COMMITID=$2
	END_COMMITID=$3
	SOURCE_DIR=""
	PATCH_DIR=""
	GIT_FILE=""
	COMMIT_FILE=""
	README=""

	SOURCE_DIR=${PATCH_SOURCE_DIR}/source/${GIT_REPOSITORY}
	PATCH_DIR=${PATCH_SOURCE_DIR}/patch/${GIT_REPOSITORY}
	GIT_FILE=${PATCH_SOURCE_DIR}/log.txt
	COMMIT_FILE=${PATCH_SOURCE_DIR}/commit.txt
	README=${PATCH_SOURCE_DIR}/readme.txt

	echo "PATCH_SOURCE_DIR=${PATCH_SOURCE_DIR}"
	echo "SOURCE_DIR=${SOURCE_DIR}"
	echo "PATCH_DIR=${PATCH_DIR}"
	echo "GIT_FILE=${GIT_FILE}"
	echo "COMMIT_FILE=${COMMIT_FILE}"
	echo "README=${README}"

	if [ ! -d $SOURCE_DIR ] ; then
		mkdir -p $SOURCE_DIR
	fi

	if [ ! -d $PATCH_DIR ] ; then
		mkdir -p $PATCH_DIR
	fi
}

function get_commit()
{
	local start_commit=$1
	local end_commit=$2

	if [ -z "$start_commit" ] ; then
		start_commit="HEAD~0"
	fi

	if [ -z "$end_commit" ] ; then
		end_commit="HEAD~0"
	fi

	git log --pretty=format:"%H" --reverse $start_commit...$end_commit > $COMMIT_FILE
	echo " " >> $COMMIT_FILE
	if [ $? -ne 0 ] ; then
		echo "error: get commit falied"
		exit 1
	fi
}

function filter_patch_subproject()
{
	local input=$1

	awk 'BEGIN { RS="diff --git"; ORS="====diff --git" } {print $0 }' $input > $input.step1
	head -n -1 $input.step1 > $input.step2
	sed -i "s/^-- $/====-- /" $input.step2
	awk 'BEGIN { RS="===="; ORS="" } !/-Subproject/ { print $0 }' $input.step2 > $input
	rm $input.step1
	rm $input.step2
}

function get_patch()
{
	local start_commit=$1
	local end_commit=$2

	if [ -z "$start_commit" ] ; then
		start_commit="HEAD~0"
	fi

	if [ -z "$end_commit" ] ; then
		end_commit="HEAD~0"
	fi

	git format-patch $start_commit...$end_commit -o $PATCH_DIR
	if [ $? -ne 0 ] ; then
		echo "error: git patch failed"
		exit 1
	fi
	for f in $(ls $PATCH_DIR) ; do
		filter_patch_subproject $PATCH_DIR/$f
	done
}

function get_patch_source()
{
	local count=0
	local start_commit=$1
	local end_commit=$2

	local commit=""

	get_commit $start_commit $end_commit

	while read line
	do
		count=0

		commit=$line
		git log --oneline --name-only $commit -1 > $GIT_FILE
		if [ $? -ne 0 ] ; then
			echo "error:get_patch_source failed"
			exit 1
		fi

		while read line
		do
			count=$(($count+1))
			if [ $count -eq 1 ] ; then
				echo "first line skip"
			else
				if [ -d $line ] ; then
					echo "ignore directory $line"
				elif [ -f $line ] ; then
					cp --parents $line $SOURCE_DIR
					md5sum $line >> $README
				elif [ ! -f $line ] ; then
					echo "$line not exist"
					if [ ! -f ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh ] ; then
						echo "!/bin/bash" > ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh
						echo "" >> ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh
						chmod +x ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh
					fi
					if [ ! -f $SOURCE_DIR/remove-deprecated.sh ] ; then
						echo "!/bin/bash" > $SOURCE_DIR/remove-deprecated.sh
						chmod +x $SOURCE_DIR/remove-deprecated.sh
						echo "cd ./${GIT_REPOSITORY}" >> ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh
						echo "./remove-deprecated.sh" >> ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh
						echo "cd -" >> ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh
						echo "" >> ${PATCH_SOURCE_DIR}/source/remove-deprecated-all.sh
					fi
					echo "[[ -f $line ]] && mv $line $line.deprecated" >> $SOURCE_DIR/remove-deprecated.sh
				fi
			fi
		done < $GIT_FILE
	done < $COMMIT_FILE
}

function create_patch()
{
	local src_site1="ssh://git@hichiptech.gitlab.com:33888"
	local src_site2="https://hichiptech.gitlab.com:8023"
	local start_commit=$1
	local end_commit=$2
	local top_repo=$(git -C $GIT_REPOSITORY remote -v | head -n1 | awk '{print $2}')
	local top_sdk=$(basename $(echo $top_repo | sed "s#$src_site1/##" | sed "s#$src_site2/##" | sed 's#\.git$##'))
	local revision_start=""
	local revision_end=""
	local sub_project=""
	local subprj=""
	local old_commit=$(git -C $GIT_REPOSITORY log -1 --pretty=%H)
	local old_repo=$GIT_REPOSITORY

	git -C $GIT_REPOSITORY checkout $start_commit
	git -C $GIT_REPOSITORY submodule update --init --recursive
	echo "$start_commit $GIT_REPOSITORY" > ${PATCH_SOURCE_DIR}/revision-start.info
	git -C $GIT_REPOSITORY submodule foreach --recursive "echo \$sha1 \$displaypath >> $PATCH_SOURCE_DIR/revision-start.info"

	git -C $GIT_REPOSITORY checkout $end_commit
	git -C $GIT_REPOSITORY submodule update --init --recursive
	echo "$end_commit $GIT_REPOSITORY" > ${PATCH_SOURCE_DIR}/revision-end.info
	git -C $GIT_REPOSITORY submodule foreach --recursive "echo \$sha1 \$displaypath >> $PATCH_SOURCE_DIR/revision-end.info"

	rm -f $PATCH_SOURCE_DIR/revision-diff.info
	touch $PATCH_SOURCE_DIR/revision-diff.info

	for subprj in $(cat $PATCH_SOURCE_DIR/revision-start.info | awk '{print $2}') ; do
		revision_start=$(cat $PATCH_SOURCE_DIR/revision-start.info | awk "{ if (\$2==\"$subprj\") print \$1}")
		revision_end=$(cat $PATCH_SOURCE_DIR/revision-end.info | awk "{ if (\$2==\"$subprj\") print \$1}")
		if [ $revision_start != $revision_end ] ; then
			echo "$revision_start $revision_end $subprj" >> $PATCH_SOURCE_DIR/revision-diff.info
			GIT_REPOSITORY=$subprj
			cd $GIT_REPOSITORY
			prepare_env_variables $subprj $revision_start $revision_end
			get_patch $revision_start $revision_end
			get_patch_source $revision_start $revision_end
			cd -
		fi
	done

	git -C $old_repo checkout $old_commit
	git -C $old_repo submodule update --init --recursive
}

if [ "x$PATCH_DESCRIBE" = "x" ] ||  [ "x$PROJECT_PATH" = "x" ] ||  [ "x$GIT_REPOSITORY" = "x" ] ||  [ "x$START_COMMITID" = "x" ] ||  [ "x$END_COMMITID" = "x" ] ; then
	get_help
	exit 1
fi

create_patch $START_COMMITID $END_COMMITID
