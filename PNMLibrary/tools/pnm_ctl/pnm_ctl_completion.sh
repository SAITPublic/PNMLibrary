_pnm_ctl_completion() {
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    subcommands_1="-h --help setup-shm destroy-shm reset-imdb setup-dax setup-dax-device destroy-dax-device destroy-dax reset-sls info timeout cleanup leaked"

    if [[ ${COMP_CWORD} == 1 ]] ; then
        COMPREPLY=( $(compgen -W "${subcommands_1}" -- ${cur}) )
        return 0
    fi

    subcmd_1="${COMP_WORDS[1]}"
    case "${subcmd_1}" in
    setup-shm)
        if [[ ${COMP_CWORD} == 2 ]] ; then
            COMPREPLY=( $(compgen -W "-s --sls -i --imdb -h --help" -- ${cur}) )
            return 0
        fi

        COMPREPLY=( $(compgen -W "-m --mem -r --regs -h --help" -- ${cur}) )
        return 0
    ;;
    destroy-shm)
        if [[ ${COMP_CWORD} == 2 ]] ; then
            COMPREPLY=( $(compgen -W "-s --sls -i --imdb -h --help" -- ${cur}) )
            return 0
        fi

        COMPREPLY=( $(compgen -W "-h --help" -- ${cur}) )
        return 0
    ;;
    info)
        info_subcommands="-h --help --hex -r --ranks --state --acquisition-count --free-size --base-size --base-offset --inst-size --inst-offset --cfgr-size --cfgr-offset --tags-size --tags-offset --psum-size --psum-offset"
        COMPREPLY=( $(compgen -W "${info_subcommands}" -- ${cur}) )
        return 0
    ;;
    timeout | cleanup)
        COMPREPLY=( $(compgen -W "-h --help --set" -- ${cur}) )
        return 0
    ;;
    reset-imdb)
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
