#!/bin/bash 
get_version_info()
{
    #local REMOTE_REPO_TAG="`${CMD_REMOTE_SH} "cd ${PATH_CDC_SRC}; git tag | tail -n 1 "`"
    #local REPO_TAG=`echo "${REMOTE_REPO_TAG}" | awk '{print $1}'`
    #cd ${SOURCE_PATH}

    local REMOTE_REPO_TAG="`git for-each-ref --sort=taggerdate --format '%(refname)' | tail -n 1 | grep  \"refs/tags/\"`"
    local REPO_TAG=`echo "${REMOTE_REPO_TAG}" | awk -F "refs/tags/" '{print $2}'`

    local REMOTE_REPO_BRANCH="` git branch | grep \* `"
    local REPO_BRANCH=`echo "${REMOTE_REPO_BRANCH}" | awk '{print $2}'`

    local REMOTE_REPO_COMMIT="` git log -1 | grep \"^commit \"`"

    local REPO_COMMIT=`echo "${REMOTE_REPO_COMMIT}" | awk '{print $2}'`

    #local REMOTE_REPO_DATE="` git for-each-ref --sort=taggerdate --format '%(refname) %(taggerdate)' | tail -n 1 | grep  \"refs/tags/\"`"
    #local REPO_DATE=`echo "${REMOTE_REPO_DATE}" | cut -d ' ' -f 2-7`
    local REMOTE_REPO_DATE="` git log -1 | grep \"^Date:   \"`"
    local REPO_DATE="${REMOTE_REPO_DATE#"Date:   "}"

    local REMOTE_REPO_AUTHOR="` git log -1 | grep "^Author:"`"
    local REPO_AUTHOR="` echo ${REMOTE_REPO_AUTHOR} | cut -d ' ' -f 2`"

    local REMOTE_REPO_CHANGE_ID="$( git log -1 | grep "Change-Id:")"
    local REPO_CHANGE_ID="$( echo ${REMOTE_REPO_CHANGE_ID} | cut -d ' ' -f 2)"

    local RELEASE_AUTHOR=`whoami`

    echo "--${REPO_TAG} : ${REPO_BRANCH} : ${REPO_COMMIT} : ${REPO_DATE} : ${REPO_AUTHOR} : ${REPO_CHANGE_ID} : ${RELEASE_AUTHOR}----"

    sed -i "/^#define REPO_TAG/c#define REPO_TAG \"${REPO_TAG}\"" ./mpp_version.h
	sed -i "/^#define REPO_BRANCH/c#define REPO_BRANCH \"${REPO_BRANCH}\"" ./mpp_version.h
	sed -i "/^#define REPO_COMMIT/c#define REPO_COMMIT \"${REPO_COMMIT}\"" ./mpp_version.h
    sed -i "/^#define REPO_DATE/c#define REPO_DATE \"${REPO_DATE}\"" ./mpp_version.h
    sed -i "/^#define REPO_AUTHOR/c#define REPO_AUTHOR \"${REPO_AUTHOR}\"" ./mpp_version.h
    sed -i "/^#define REPO_CHANGE_ID/c#define REPO_CHANGE_ID \"${REPO_CHANGE_ID}\"" ./mpp_version.h
    sed -i "/^#define RELEASE_AUTHOR/c#define RELEASE_AUTHOR \"${RELEASE_AUTHOR}\"" ./mpp_version.h
}
get_version_info