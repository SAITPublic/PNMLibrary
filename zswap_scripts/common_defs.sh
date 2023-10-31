# Common functions

# -u Treat unset variables as an error when substituting
# -e Exit immediately if a command exits with a non-zero status
# -x Print commands and their arguments as they are executed
# -E If set, the ERR trap is inherited by shell functions
set -uexE

SCRIPT_NAME=$(basename ${0})
DEBUG=false

function error() {
    local parent_lineno="${1}"
    local error_code="${2}"
    log "Error on or near line ${parent_lineno}; exiting with status ${error_code}"
    exit "${error_code}"
}

function log() {
    local log_str="${@}\n"
    local script_data=""
    if [ ${DEBUG} = true ]; then
        script_data="${SCRIPT_NAME}:${BASH_LINENO}: "
    fi
    printf "${script_data}${log_str}"
    if [ ! -z ${LOG_FILE:-} ]; then
        printf "${script_data}${log_str}" >> $LOG_FILE
    fi
}

# Single quotes are important
# Call function when some command returns non zero code
trap 'error $LINENO $?' ERR

CPP_FILES_EXT="cpp|cc|cp|c\+\+|c|cxx|hh|h|hpp|hxx"

function get_commit_changed_files {
    local commit_hash=${1}
    # Passed variable should be bash array
    declare -n __local_modified_filepaths__=${2}
    # To properly handle file names with spaces, we have to do some bash magic.
    # We set the Internal Field Separator to nothing and read line by line.
    while IFS='' read -r line
    do
        __local_modified_filepaths__+=("$line")

        # Only run it on files that were modified.
        # `git diff-tree` outputs all the files that differ between the different commits.
        # By specifying `--diff-filter=d`, it doesn't report deleted files.
    done < <(git diff-tree --no-commit-id --diff-filter=d --name-only \
             -r "${commit_hash}" | grep -iE "*\.(${CPP_FILES_EXT})$" || true)
}

function get_all_files {
    declare -n __local_all_filepaths__=${1}

    # [TODO: @p.bred] Generalize formatters
    local root_dirs="."
    if [ "${2}" = "linux" ]; then
        root_dirs="mm/pnm lib/pnm include/linux/pnm drivers/pnm scripts/pnm"
    fi

    while IFS='' read -r line
    do
        __local_all_filepaths__+=("$line")

    done < <(find ${root_dirs} -name build -prune -o -regextype posix-extended -regex ".*\.(${CPP_FILES_EXT})$" -print)

}

function get_files_for_format_check {
    if [ ${1} != "master" ]
    then
        get_commit_changed_files ${1} ${2}
    else
        get_all_files ${2} ${3}
    fi
}
