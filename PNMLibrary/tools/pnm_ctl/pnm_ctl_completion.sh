_pnm_ctl_completion() {
    SLS_RESOURCE=/sys/class/pnm/sls_resource
    IMDB_RESOURCE=/sys/class/pnm/imdb_resource

    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"

    subcommands_sls="sls-reset sls-info sls-timeout sls-cleanup sls-leaked"
    subcommands_imdb="imdb-reset imdb-info imdb-cleanup imdb-leaked"
    subcommands_total="-h --help setup-shm destroy-shm setup-dax setup-dax-device destroy-dax destroy-dax-device"

    shm_args="-h --help -s --sls -i --imdb"

    if test -d "$SLS_RESOURCE"; then
        subcommands_total+=" $subcommands_sls"
    fi

    if test -d "$IMDB_RESOURCE"; then
        subcommands_total+=" $subcommands_imdb"
    fi

    if [[ ${COMP_CWORD} == 1 ]] ; then
        COMPREPLY=( $(compgen -W "${subcommands_total}" -- ${cur}) )
        return 0
    fi

    subcmd_1="${COMP_WORDS[1]}"
    case "${subcmd_1}" in
    setup-shm)
        if [[ ${COMP_CWORD} == 2 ]] ; then
            COMPREPLY=( $(compgen -W "${shm_args}" -- ${cur}) )
            return 0
        fi

        COMPREPLY=( $(compgen -W "-m --mem -r --regs -h --help" -- ${cur}) )
        return 0
    ;;
    destroy-shm)
        if [[ ${COMP_CWORD} == 2 ]] ; then
            COMPREPLY=( $(compgen -W "${shm_args}" -- ${cur}) )
            return 0
        fi

        COMPREPLY=( $(compgen -W "-h --help" -- ${cur}) )
        return 0
    ;;
    sls-info)
        info_subcommands="-h --help --hex -r --ranks --state --acquisition-count --free-size --base-size --base-offset --inst-size --inst-offset --cfgr-size --cfgr-offset --tags-size --tags-offset --psum-size --psum-offset"
        COMPREPLY=( $(compgen -W "${info_subcommands}" -- ${cur}) )
        return 0
    ;;
    sls-timeout | sls-cleanup | imdb-cleanup)
        COMPREPLY=( $(compgen -W "-h --help --set" -- ${cur}) )
        return 0
    ;;
    imdb-reset)
        COMPREPLY=( $(compgen -W "-h --help -H --hardware" -- ${cur}) )
        return 0
    ;;
    *)
        COMPREPLY=( $(compgen -W "-h --help" -- ${cur}) )
        return 0
    ;;
    esac
    return 0
}

alias run_pnm_ctl=./build/tools/pnm_ctl
complete -F _pnm_ctl_completion run_pnm_ctl
complete -F _pnm_ctl_completion pnm_ctl
