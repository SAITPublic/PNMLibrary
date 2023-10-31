# System
Currently we support *Ubuntu 22.04* based systems with fresh CMake (see below). You can try use different systems,
but be aware that they are not tested. Some known restrictions are:

* GCC version >= 11.0 (needs proper C++17 support).
* Clang version >= 14.0
* Cmake version >= 3.18

# Install prerequisites (Ubuntu 22.04)
```bash
# PNM library dependencies
$ sudo apt install ndctl libndctl-dev daxctl libdaxctl-dev numactl clang

# Linux kernel build dependencies
$ sudo apt install fakeroot ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison rsync kmod cpio dpkg-dev ccache

# Pytorch build dependencies
$ sudo apt install python3-pip libopenblas-dev

# DRAMSim3 and performance simulator build dependencies
$ sudo apt install libglib2.0-dev libpixman-1-dev scons libboost-regex-dev libboost-program-options-dev

# (Optional) Gramine dependencies
$ sudo apt install autoconf bison gawk nasm python3-click python3-jinja2 python3-pyelftools libprotobuf-c-dev protobuf-c-compiler

# Pytorch python dependencies
$ pip3 install -U protobuf==3.13 future yappi numpy==1.23.5 pydot typing_extensions ninja onnx scikit-learn tqdm tensorboard sympy filelock jinja2 networkx

# (Optional) Gramine python dependencies
$ pip3 install -U "meson>=0.56" "toml>=0.10" cryptography Markupsafe==1.1.0

# Install cmake from snap
$ sudo snap install cmake --classic
# or from source code
$ mkdir cmake_src; cd cmake_src; wget https://github.com/Kitware/CMake/releases/download/v3.27.5/cmake-3.27.5.tar.gz
$ tar -zxvf cmake-3.27.5.tar.gz
$ cd cmake-3.27.5; sudo ./bootstrap; sudo make; sudo make install
```

# Download and prepare repositories
This command will download OSS projects and apply PNM patches making them ready for build.
```bash
$ RELEASE_DIR=`pwd`
$ ./setup_repos.sh $RELEASE_DIR
```

# Build PNM software
These commands will build and install PNM-enabled software (kernel and userspace programs).
```bash
$ ./build_kernel.sh $RELEASE_DIR
...
$ sudo systemctl reboot
...
$ ./build_userspace.sh $RELEASE_DIR
```

# Build Gramine (optional)
```bash
$ ./build_gramine.sh $RELEASE_DIR
```

# Add `pnm_ctl' tool into your $PATH
```
$ export PATH=$RELEASE_DIR/PNMLibrary/build/tools/:$PATH
```

# Execute DLRM with PNM-SLS

## Setup PNM SLS device functional simulator
```bash
$ ./setup_sls_func_sim.sh -i
```

## Create DLRM embedding tables
```bash
$ EMB_TABLES_DIR=$RELEASE_DIR/emb_tables/; mkdir -p $EMB_TABLES_DIR
$ cd $RELEASE_DIR/PNMLibrary
$ ./scripts/create_test_dataset.sh --dlrm --root $EMB_TABLES_DIR
```

## Run DLRM or DeepRecSys
```bash
# Run facebookresearch DLRM
$ cd $RELEASE_DIR/upstream/dlrm
$ ./run_test.sh --weights-path $EMB_TABLES_DIR/DLRM_FLOAT/embedding.bin --use-pnm simple # use 'secure' for SecNDP

# Run DeepRecSys
$ cd $RELEASE_DIR/upstream/DeepRecSys
$ ./run_DeepRecSys_pnm.sh --tables $EMB_TABLES_DIR/DLRM_FLOAT/embedding.bin --use_pnm --num_engines 5
```

## Cleanup PNM SLS device functional simulator
```bash
$ ./setup_sls_func_sim.sh -u
```

# Execute IMDB test application with PNM-IMDB

## Setup PNM IMDB device functional simulator
```bash
$ ./setup_imdb_func_sim.sh -i
```

## Prepare a directory for IMDB test data
```bash
$ IMDB_DATA_DIR=$RELEASE_DIR/imdb_data/; mkdir -p $IMDB_DATA_DIR
```

## Run IMDB test application
```bash
$ cd $RELEASE_DIR/PNMLibrary
$ ./build/test/imdb_demo $IMDB_DATA_DIR
```

## Cleanup PNM IMDB device functional simulator
```bash
$ ./setup_imdb_func_sim.sh -u
```

# Run performance simulator

## Build performance simulator and QEMU
```bash
$ cd $RELEASE_DIR; ./build_perfsim.sh $RELEASE_DIR
```

## Run performance simulator
Please refer to performance simulator README.md and README_REAL_APP_MODE.md located in 'DRAMsim3' directory after running `setup_repos.sh`.

# PNM library Logging
The PNM software stack uses [spdlog](https://github.com/gabime/spdlog) as a logging library. To enable additional (e.g. debug) logging,
use `SPDLOG_LEVEL` environment variable, e.g.:
```bash
$ export SPDLOG_LEVEL=debug
```

# CXL-PNM zSwap simulator using QEMU

## Setup GRUB_CMDLINE_LINUX
In QEMU, add following line in `/etc/default/grub` and regenerate grub config:
```bash
$ vim /etc/default/grub
...
GRUB_CMDLINE_LINUX="memmap=32G\\\$4G quiet splash console=ttyS0,115200n8 console=tty1"

$ sudo grub-update
$ sudo shutdown -h now
```

## Boot QEMU with correct NUMA parameters and run CXL-PNM zSwap tests
On host:
```bash
$ sudo qemu-system-x86_64 -display none -m 36G -boot d -enable-kvm -smp 16 -drive file=testing-image.img,if=virtio -serial stdio -cpu host,l3-cache=on \
                          -numa node,cpus=0-7,memdev=mem0,nodeid=0 -object memory-backend-ram,id=mem0,size=4G \
                          -numa node,cpus=8-15,memdev=mem1,nodeid=1 -object memory-backend-ram,id=mem1,size=32G

```
In QEMU:
```bash
$ cd $RELEASE_DIR/upstream/linux
$ bash ../../zswap_scripts/tests_at_once.sh

```
