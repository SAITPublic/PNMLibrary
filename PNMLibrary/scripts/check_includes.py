#!/usr/bin/env python3

# This script is header usage checks, it checks that headers from
# some parts of the project are not used in others.

import os
import sys
from clang.cindex import Index, CompilationDatabase, TranslationUnit
import argparse

class Action(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if self.dest in ['veto_dirs', 'allowed_dirs']:
            attr = 'veto_and_allowed_dirs'
        else:
            attr = 'check_and_excluded_dirs'
        l = getattr(namespace, attr, [])
        l.append((self.dest, values))
        setattr(namespace, attr, l)

def gen_all_sources_from_dirs(user_dirs, mask_ext):
    srcs = []
    for d in user_dirs:
        for root, dirs, files in os.walk(d):
            for f in files:
                if any(f.endswith(ext) for ext in mask_ext):
                    srcs.append(os.path.abspath(os.path.join(root,f)))
    return list(set(srcs))

def process_dirs(user_dirs, mask_ext, PROJECT_ROOT_DIR):
    # Treat all dirs as veto if user specified -a <allowed_dirs> as a first option in -a/-v chain;
    # Treat all dirs as excluded if user specified -c <check_dirs> as a first option in -c/-e` chain
    if user_dirs[0][0] in ["exclude_dirs", "allowed_dirs"]:
        FILES = gen_all_sources_from_dirs([PROJECT_ROOT_DIR], mask_ext)
    else:
        FILES = []
    for opt, dirs in user_dirs:
        DIRS = dirs.split(":")
        PART_OF_FILES = gen_all_sources_from_dirs(DIRS, mask_ext)
        if opt in ["check_dirs", "veto_dirs"]:
            FILES += PART_OF_FILES
        else:
            FILES = list(set(FILES) - set(PART_OF_FILES))
    return FILES

def main():
    ## Parsing args, description messages
    HELP_MESSAGE = """
    usage: %prog -r ROOT_DIR [-v VETO_DIR1:VETO_DIR2 -a ALLOWED_DIR1:ALLOWED_DIR2] [-c CHECK_DIR1:CHECK_DIR2 -e EXCLUDE_DIR1:EXCLUDE_DIR2]
    the parameters in square brackets [] can occur in any order and will be taken into account in turn from left to right.
    
    If in the sequence -v -a:
    first argument is -a - We prohibit ANY headers, except -a headers and build files.
    To allow headers, you need to write about it explicitly via -a.
    first argument is -v - We prohibit only those headers that are specified -v, except -a headers and build files.
    
    If in the sequence -c -e:
    first argument is -e - We check ALL files, except -e files and build files.
    first argument is -c - We check only -c files, except -e files and build files.

    We basically don't check the build folder.
    """
    parser = argparse.ArgumentParser(HELP_MESSAGE)
    parser.add_argument(
        "-r", "--root-dir", dest = "root_dir",
        help = "root working directory")
    parser.add_argument(
        "-b", "--build-dir", dest = "build_dir",
        default = "build",
        help = "build directory containing all possible builds, " +
        "default = 'build'")
    parser.add_argument(
        "-d", "--comp-db-dir", dest = "db_dir",
        default = "build",
        help = "build directory with CMake compile_commands.json, " +
        "default = 'build'")
    parser.add_argument(
        "-v", "--veto-dirs", dest = "veto_dirs", action=Action,
        help = "directories with prohibited headers")
    parser.add_argument(
        "-a", "--allowed-dirs", dest = "allowed_dirs", action=Action,
        help = "directories with allowed headers from 'veto_dirs'")
    parser.add_argument(
        "-e", "--exclude-dirs", dest = "exclude_dirs", action=Action,
        help = "directories to skip checking, auto included 'build-dir' " +
        "and 'veto-dirs' directories")
    parser.add_argument(
        "-c", "--check-dirs", dest = "check_dirs", action=Action,
        help = "check these directories only, if not specified, " +
        "check all directories in the root")
    opts = parser.parse_args()
    if not os.path.isdir(f"{opts.root_dir}"):
        exit(f"Root='{opts.root_dir}' directory does not exist!")
    os.chdir(opts.root_dir)
    if not os.path.isdir(f"{opts.build_dir}"):
        exit(f"Build='{opts.build_dir}' directory does not exist!")
    if "veto_and_allowed_dirs" not in opts:
        exit("Missed required at least one of --veto-dirs or --allowed-dirs argument")
    if "check_and_excluded_dirs" not in opts:
        exit("Missed required at least one of --check-dirs or --exclude-dirs argument")
    for _, dirs in opts.veto_and_allowed_dirs + opts.check_and_excluded_dirs:
        check_exist_dirs = dirs.split(":")
        for check_dir in check_exist_dirs:
            if not os.path.isdir(check_dir):
                exit(f"Directory {check_dir} does not exist!")

    ## Generate sources
    PROJECT_ROOT_DIR = os.getcwd()
    PROJECT_BUILD_DIR = opts.build_dir
    PROJECT_COMP_DB_DIR = opts.db_dir
    
    BUILD_FILES = gen_all_sources_from_dirs([PROJECT_BUILD_DIR],
                                  [".h", ".c", ".cc", ".cpp"])

    VETO_HEADERS = process_dirs(opts.veto_and_allowed_dirs, [".h"], PROJECT_ROOT_DIR)
    CHECK_FILES = process_dirs(opts.check_and_excluded_dirs, [".h", ".c", ".cc", ".cpp"], PROJECT_ROOT_DIR)

    # Ignore build dir
    VETO_HEADERS = list(set(VETO_HEADERS) - set(BUILD_FILES))
    CHECK_FILES = list(set(CHECK_FILES) - set(BUILD_FILES))

    ## Index and compilation DB prepare
    index = Index.create()
    compdb = CompilationDatabase.fromDirectory(PROJECT_COMP_DB_DIR)

    ## Check
    has_violation = False
    for src in CHECK_FILES:
        # infers compile commands even for missing files
        # like headers, always succeeds
        commands = compdb.getCompileCommands(src)
        # last 2 arguments have nothing to do with flags
        compile_flags = list(commands[0].arguments)[0:-2]
        tu = index.parse(src, compile_flags,
                         options = TranslationUnit.PARSE_INCOMPLETE)
        VETO_HEADERS_NAMES = set(map(os.path.relpath, VETO_HEADERS))
        for i in tu.get_includes():
            if os.path.relpath(i.include.name) in VETO_HEADERS_NAMES:
                has_violation = True
                print(f"Please do not use headers from veto directories!\n"\
                      f"\t Source: {src}\n"\
                      f"\t Prohibited header: {i.include.name}")
    sys.exit(has_violation)
if __name__ == '__main__':
    main()
