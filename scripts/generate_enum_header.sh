#!/bin/bash

# Description : This script is used to generated custom enum-header files using glib-mkenums
#               and sed. It takes the standard header templates used in 99% of projects and is
#               inspired from meson's gnome.mkeums_simple() function. Additionally, it allows
#               you to specify a sed string which edits the file based upon the regex specified.
#               The sed string allows for extended regexp (sed's -E flag).
#
# Args        : $1 (required)
#                       Relative path (relative to root of source code) of the enum
#                       header file to be generated.
#                       For eg:
#                       If you want to generate 'gdata-enums.h' file which resides in
#                       'gdata/' directory from source code root, the argument should be
#                       'gdata-enums.h'. The script uses the $MESON_SUBDIR and $MESON_SOURCE_ROOT
#                       environment variables to get the absolute paths of the header files.
#
#               $2 (required):
#                       This argument is varargs in nature, meaning that if you give n
#                       arguments to this script, the last n-1 will be used as the sources
#                       for glib-mkenums. These are also header-files just like $1.
#
#               -s [Sed Expression] (optional):
#                       This argument is optional and corresponds to the sed expression which
#                       needs to be applied on the output from glib-mkenums. The script can accept
#                       multiple sed expression, each given with a separate -s flag.
#
# Outputs     : File $1 in the directory relative to the root of source code
# Author      : Mayank Sharma (mayank8019@gmail.com)

# Make the script exit on first failure
set -e

function usage() {
    echo "usage generate_enum_header: [-s SED_EXPRESSION_STRING]
                            OUTPUT_HEADER_FILE
                            SOURCES_FOR_GLIB_MKENUMS...
positional arguments:
                            First argument is the name of the
                            of enum header file to be generated.

                            Subsequent arguments should also be
                            names of source header files and
                            will be provided to glib-mkenums as sources
optional arguments:
    -s                      sed expression string
    -h                      show this help message and exit
    "
}

if [ $# -eq 0 ]
then
    usage
    exit -1
fi

if [[ -z "${MESON_SOURCE_ROOT}" || -z "${MESON_SUBDIR}" ]]
then
    echo "The environment variables $MESON_SOURCE_ROOT and $MESON_SUBDIR must be set."
    exit -1
fi

# Get the directory in which this script is present (/scripts)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TIMES_SED_NEEDED=0
SED_STRINGS=()
NUM_ARGS=$#
SOURCES=()

# Parse the commandline arguments (and flags) provided
while getopts ":s:" opt; do
    case $opt in
        s)
            TIMES_SED_NEEDED=$(($TIMES_SED_NEEDED + 1))
            SED_STRINGS+=("$OPTARG")
            ;;
        h)
            usage
            ;;
        \?)
            usage
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires a sed string as argument." >&2
            exit 1
            ;;
    esac
done

# For each -s provided, shift the set of positional arguments by 2
for ((i = 0; i < $TIMES_SED_NEEDED; i++))
do
    shift 2
    NUM_ARGS=$(($NUM_ARGS - 2))
done

HEADER_FILE="$MESON_SOURCE_ROOT/$MESON_SUBDIR/""${1}"

# Remove the HEADER_FILE from the list of positional arguments
shift 1

# Compute the absolute paths of the sources provided
for i in "${@}"
do
    SOURCES+=("$MESON_SOURCE_ROOT/$MESON_SUBDIR/""${i}")
done

fhead=""
fprod=""
ftail=""
vhead=""
vprod=""
vtail=""

# File-header section
fhead=$(cat <<- EOF
    $fhead\n
    #pragma once\n
    #include <glib-object.h>\n
    \n
    G_BEGIN_DECLS\n
EOF
)

# File-production section
fprod="$fprod\n"'/* enumerations from "@basename@" */\n'

# Value-header section
vhead=$(cat <<- EOF
    $vhead\n
    GType @enum_name@_get_type (void) G_GNUC_CONST;\n
    #define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())
EOF
)

# File-tail section
ftail="$ftail\n"'G_END_DECLS'

# Generate the enum-header file
ENUM_HEADER="$(glib-mkenums --fhead "$fhead" --fprod "$fprod" --vhead "$vhead" --ftail "$ftail" "${SOURCES[@]}")"

# Apply sed expressions on the $ENUM_HEADER variable
for ((i = 0; i < $TIMES_SED_NEEDED; i++))
do
    ENUM_HEADER=$(echo -e "$ENUM_HEADER" | sed -E "${SED_STRINGS[$i]}")
done

echo -e "$ENUM_HEADER" > "$HEADER_FILE"
exit $?
