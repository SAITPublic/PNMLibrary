#!/bin/sh

INSTALL="unknown"

usage() {
    echo "<Usage> setup_imdb_func_sim.sh <command>

        <command>                              <description>

        --install(-i)                          Setup resources needed for PNM IMDB functional simulator.
        --uninstall(-u)                        Cleanup resources needed for PNM IMDB functional simulator.
    "
}


########################################################
# command-line option check
while [ "$#" -ne 0 ]
do
    case "$1" in
        "--install"|"-i")
            echo "Setting up resources needed for PNM IMDB functional simulator."
            INSTALL="true"
            ;;
        "--uninstall"|"-u")
            echo "Cleaning up resources needed for PNM IMDB functional simulator."
            INSTALL="false"
            ;;
        *)
            usage
            exit 1
            ;;
    esac
    shift
done


setup_sim() {
    # 1. Setup imdb_resource module.
    sudo modprobe -v imdb_resource

    # 2. Setup shared memory for IMDB simulator. `pnm_ctl' must be in PATH.
    pnm_ctl setup-shm --imdb --mem=64G
}

cleanup_sim() {
    # 1. Remove imdb_resource module.
    sudo rmmod imdb_resource

    # 2. Remove shared memory for IMDB simulator.
    pnm_ctl destroy-shm --imdb
}

if [ "x$INSTALL" = "xtrue" ]
then
    setup_sim
elif [ "x$INSTALL" = "xfalse" ]
then
    cleanup_sim
else
    echo "Wrong command for simulator setup!"
    usage
    exit 1
fi
