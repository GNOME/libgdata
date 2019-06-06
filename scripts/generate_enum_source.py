#!/usr/bin/env python3

# If the code below looks horrible and unpythonic, do not panic.
#
# It is. Feel free to create a MR to improve this.

import re
import sys
import argparse
import subprocess

from os import path
from shutil import which

def get_script_path():
    return path.dirname(path.realpath(sys.argv[0]))

def create_parser():
    parser = argparse.ArgumentParser(description='''
    This script is used to generate custom enum-source files using glib-mkenums.
    It takes the standard source templates used in 99% of projects and is
    inspired from meson's gnome.mkeums_simple() function. Additionally, it allows
    you to specify search and replace strings using the
    --regex "string to replace" "post replace string" parameters.
            ''')
    parser.add_argument('output_file', help='Relative path to output enum source file. (see --out-dir)')

    requiredSources = parser.add_argument_group('required named arguments')
    requiredSources.add_argument('--sources', nargs='+', required=True, help='The sources required for generating the enum files.')

    parser.add_argument('--replace', nargs=2, action='append',
            metavar=('STRING_TO_REPLACE', 'STRING_WHICH_REPLACES'),
            help='''Requires an argument pair, of which first one is the
            string which is to be replaced, while the argument immediately
            following is the string which replaces. Regexes are allowed as well.

            You can supply this argument multiple times, thereby sequentially
            replacing the text in generated source file.''')

    parser.add_argument('--source-dir', metavar=('SOURCE_DIR'),
            help='''The absolute path to source directory in which the sources are present.
            If this argument is set, the relative paths will be resolved from
            this directory. (Default value: dir of script)''')

    parser.add_argument('--out-dir', metavar=('OUT_DIR'), nargs='+',
            help='''The absolute paths to directories in which the generated source
            file will be created. More than one argument can be given, and
            output_file will be created in each of those directories.
            (Default value: dir of script)''')
    return parser

def create_enum_source(args, fhead, fprod, ftail, vhead, vprod, vtail):
    try:
        assert(path.isdir(args.source_dir) == True)
    except AssertionError:
        print('Argument given to --source-dir is not a directory.')
        sys.exit(-1)

    if args.source_dir is not None:
        sources = [path.join(args.source_dir, source_file) for source_file in args.sources]
    else:
        sources = [path.join(get_script_path(), source_file) for source_file in args.sources]

    bash_command = [which('glib-mkenums'),
            '--fhead', fhead,
            '--fprod', fprod,
            '--ftail', ftail,
            '--vhead', vhead,
            '--vprod', vprod,
            '--vtail', vtail,
            ] + sources
    process = subprocess.Popen(bash_command, stdout=subprocess.PIPE)
    output, error = process.communicate()
    if (error is not None):
        raise error
    return output.decode('utf-8')

def get_template_sections(args):
    fhead=''
    fprod=''
    ftail=''
    vhead=''
    vprod=''
    vtail=''

    # File-header section
    for s in args.sources:
        fhead += '#include "{}"\n'.format(s)

    fhead += '''
        #define C_ENUM(v) ((gint) v)
        #define C_FLAGS(v) ((gint) v)
    '''

    # File-production section
    fprod = '{}\n/* enumerations from "@basename@" */\n'.format(fprod)

    # Value-header section
    vhead='''
        GType
        @enum_name@_get_type (void)
            {
                static GType etype = 0;
                if (etype == 0) {
                    static const G@Type@Value values[] = {
    '''

    # Value-production section
    vprod += '\n{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" },\n'

    # Value-tail section
    vtail += '''
            { 0, NULL, NULL }
        };
        etype = g_@type@_register_static ("@EnumName@", values);
      }
      return etype;
    }
    '''

    # File-tail section
    ftail='{}G_END_DECLS'.format(ftail)

    return fhead, fprod, ftail, vhead, vprod, vtail

def write_enum_source_file(args, output):
    if (args.out_dir is not None):
        try:
            for out_path in args.out_dir:
                assert(path.isdir(out_path) == True)
        except AssertionError:
            print('One (or more) of the argument(s) given to --out-dir is not a directory.')
            sys.exit(-1)

        for out_path in args.out_dir:
            with open(path.join(out_path, args.output_file), 'w+') as f:
                f.write(output)
    else:
        with open(path.join(get_script_path(), args.output_file), 'w+') as f:
            f.write(output)

if __name__ == '__main__':
    parser = create_parser()
    args = parser.parse_args()

    (fhead, fprod, ftail, vhead, vprod, vtail) = get_template_sections(args)

    output = create_enum_source(args, fhead, fprod, ftail, vhead, vprod, vtail)
    if args.replace is not None:
        for regex_tuple in args.replace:
            try:
                output = re.sub(regex_tuple[0], regex_tuple[1], output)
            except re.error:
                sys.exit(-1)

    write_enum_source_file(args, output)

