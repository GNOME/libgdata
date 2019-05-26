#!/bin/bash

# Description : This script is used to generated custom enum-source files using glib-mkenums
#               and sed. It takes the standard source templates used in 99% of projects and is
#               inspired from meson's gnome.mkeums_simple() function. Additionally, it allows
#               you to specify a sed string which edits the file based upon the regex specified.
#               The sed string allows for extended regexp (sed's -E flag).
#
# Args        : $1 (required)
#                       Relative path (relative to root of source code) of the enum
#                       source file to be generated.
#                       For eg:
#                       If you want to generate 'gdata-enums.c' file which resides in
#                       'gdata/' directory from source code root, the argument should be
#                       'gdata/gdata-enums.c'. The script uses the $MESON_SUBDIR and $MESON_SOURCE_ROOT
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
    echo "usage generate_enum_source: [-s SED_EXPRESSION_STRING]
                            OUTPUT_HEADER_FILE
                            SOURCES_FOR_GLIB_MKENUMS...
positional arguments:
                            First argument is the name of the
                            of enum source file to be generated.

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
RELATIVE_SOURCES=()

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

ENUM_SOURCE_FILE="$MESON_SOURCE_ROOT/$MESON_SUBDIR/""${1}"

# Remove the ENUM_SOURCE_FILE from the list of positional arguments
shift 1

# Compute the absolute paths of the sources provided
for i in "${@}"
do
    SOURCES+=("$MESON_SOURCE_ROOT/$MESON_SUBDIR/""${i}")
    RELATIVE_SOURCES+=("$MESON_SUBDIR/${i}")
done

fhead=""
fprod=""
ftail=""
vhead=""
vprod=""
vtail=""

# File-header section
for ((i = 0; i < "${#RELATIVE_SOURCES[@]}"; i++))
do
    header_file="$(echo ${RELATIVE_SOURCES[$i]} | sed -E 's@.*/(.*\.h)$@\1@g')"
    fhead="$fhead\n$(printf '#include "%s"' $header_file)"
done
fhead="$fhead\n"'#define C_ENUM(v) ((gint) v)'
fhead="$fhead\n"'#define C_FLAGS(v) ((gint) v)\n'

# File-production section
fprod="$fprod\n"'/* enumerations from "@basename@" */\n'

# Value-header section
vhead=$(cat <<- EOF
$vhead\n
GType\n
@enum_name@_get_type (void)\n
{\n
  static GType etype = 0;\n
  if (etype == 0) {\n
    static const G@Type@Value values[] = {\n
EOF
)

# Value-production section
vprod="$vprod\n""      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" },\n"

# TODO: Remove this section later, it was taken from gnome.mkenums_simple()'s source code
# Value-tail section
#vtail=$(cat <<- EOF
#$vtail\n
    #{ 0, NULL, NULL }\n
  #};\n
  #if (g_once_init_enter (&gtype_id)) {\n
    #GType new_type = g_@type@_register_static (g_intern_static_string ("@EnumName@"), values);\n
    #g_once_init_leave (&gtype_id, new_type);\n
  #}\n
  #return (GType) gtype_id;\n
#}\n
#EOF
#)
vtail=$(cat <<- EOF
$vtail\n
      { 0, NULL, NULL }\n
    };\n
    etype = g_@type@_register_static ("@EnumName@", values);\n
  }\n
  return etype;\n
}\n
EOF
)

# File-tail section
ftail="$ftail\n"'G_END_DECLS'

# Generate the enum-source file
ENUM_SOURCE="$(glib-mkenums --fhead "$fhead" --fprod "$fprod" --vhead "$vhead" --vprod "$vprod" --vtail "$vtail" --ftail "$ftail" "${SOURCES[@]}")"

# Apply sed expressions on the $ENUM_SOURCE variable
for ((i = 0; i < $TIMES_SED_NEEDED; i++))
do
    ENUM_SOURCE=$(echo -e "$ENUM_SOURCE" | sed -E "${SED_STRINGS[$i]}")
done

echo -e "$ENUM_SOURCE" > "$ENUM_SOURCE_FILE"
exit $?
