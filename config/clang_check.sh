#!/bin/env bash
# Copyright (c) 2017-2021, Mudita Sp. z.o.o. All rights reserved.
# For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

set -euo pipefail

# shellcheck source=../config/format-config.sh
# shellcheck source=../config/clang/colors.sh
# shellcheck source=../config/clang/common.sh

L_GIT_DIR=$(git rev-parse --show-toplevel)
source $L_GIT_DIR/config/format-config.sh
source $L_GIT_DIR/config/clang/clang-common.sh
source $L_GIT_DIR/config/clang/colors.sh

help()
{
    cat <<- EOF
clang tidy diff tool used to perform check on our sources uses 2 defaults:
    .clang-tidy                 : configuration file, from the place where command is run
    compile_commands.json   : compile data, as well from the place where command is run
EOF
}

get_files_to_check()
{
    local files
    local endlist=()

    files=$(git diff -U0 --name-only remotes/origin/master...HEAD)
    for file in ${files}; do
        if [[ ${file} =~ ^.*\.(cpp|hpp|c|h|cxx|gcc|cc)$ ]] && shouldnt_ignore "${file}"; then
            endlist+=("$file")
        fi
    done
    echo "${endlist[*]}"
}

# if not exists create
# could be in better place than tmp
get_compile_commands()
{
    [[ -f build-linux-Debug/compile_commands.json ]] || ./configure.sh linux debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 > /dev/null 2>&1
    cp build-linux-Debug/compile_commands.json /tmp/compile_commands.json
    sed -i 's|-static-libasan||g' /tmp/compile_commands.json
}

main()
{
    if [[ $# -ne 0 ]]; then
        help
        exit 0
    fi
    local tool
    tool=$(get_clang_tidy)

    local files_to_check
    files_to_check=$(get_files_to_check)

    if [[ -z $files_to_check ]]; then
        echo "no files to check"
        exit 0
    fi

    # get the stage
    verify_clang_format_version
    get_compile_commands
    # run tidy
    git diff -U0 --no-color remotes/origin/master...HEAD "$files_to_check" | ${tool[*]} -p 1 -path=/tmp/
}

main