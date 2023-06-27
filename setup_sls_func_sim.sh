#!/bin/sh

INSTALL="unknown"

usage() {
    echo "<Usage> setup_sls_func_sim.sh <command>

        <command>                              <description>

        --install(-i)                          Setup resources needed for PNM SLS functional simulator.
        --uninstall(-u)                        Cleanup resources needed for PNM SLS functional simulator.
    "
}


########################################################
# command-line option check
while [ "$#" -ne 0 ]
do
    case "$1" in
        "--install"|"-i")
            echo "Setting up resources needed for PNM SLS functional simulator."
            INSTALL="true"
            ;;
        "--uninstall"|"-u")
            echo "Cleaning up resources needed for PNM SLS functional simulator."
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
    # 1. Setup sls_resource module.
    sudo modprobe -v sls_resource device=CXL

    # 2. Setup shared memory for SLS simulator. `pnm_ctl' must be in PATH.
    pnm_ctl setup-shm --sls --mem=64G
}

cleanup_sim() {
    # 1. Remove sls_resource module.
    sudo rmmod sls_resource

    # 2. Remove shared memory for SLS simulator.
    pnm_ctl destroy-shm --sls
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
