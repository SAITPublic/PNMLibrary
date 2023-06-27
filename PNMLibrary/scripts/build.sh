#!/bin/sh

set -e

#########################################################
# Please modify the following items according your build environment

PNM_ROOT=$(readlink -f $(dirname $0)/..)
BUILD_DIR=$PNM_ROOT/build

# CMake Default Args
CMAKE_BUILD_TYPE="Debug"
CMAKE_C_FLAGS=""
CMAKE_CXX_FLAGS=""
NUM_THREADS=1

CLEAN="false"
BUILD="true"
VERBOSE="false"
GEN_ONLY="false"

ASAN="OFF"
MSAN="OFF"
TSAN="OFF"
UBSAN="OFF"
EXEC_PLATFORM="UNDEFINED"
TEST_APPS="OFF"
CHECK_ALL="OFF"
CCACHE="OFF"
BENCHMARKS="OFF"
STATIC_LIB_CHECK="OFF"

GENERATOR="Ninja"

usage() {
    echo "<Usage> build.sh <command> <arguments>

        <command>            <argument>                  <description>

        clean                                           Clean the project
        --clean_build(-cb)                              Clean and build the project
        --dir(-d)                                       Set the build directory
        --prefix(-p)                                    Set the installation directory (default is `build/libs`)
        --release(-r)                                   Build in release mode
        --verbose(-v)                                   Verbose build
        --gen_only                                      Cmake configuration only, required for clang-tidy, headers usage
        --hw                                            Build in hardware mode
        --fsim                                          Build in simulator mode
        --psim                                          Build in performance simulator mode
        --asan                                          Build with ASan
        --msan                                          Build with MSan
        --tsan                                          Build with TSan
        --ubsan                                         Build with UBSan
        --tests                                         Build test apps
        --test_tables_root                              Add path to test tables
        --check_all                                     Make the all common checks after the build like clang-format, clang-tidy, headers usage
        --ccache                                        Attempt using CCache to wrap the compilation
        --benchmarks                                    Build benchmarks (google benchmark library is required)
        --static_lib_check                              Enable static library dependency check 
        --use_make                                      Build with make (ninja by default)
        -j                   number of threads          Number of threads to use for building (Default: 1)
    "
}

########################################################
# command-line option check
while [ "$#" -ne 0 ]
do
    case "$1" in
        "clean")
            echo "Clean"
            CLEAN="true"
            BUILD="false"
            ;;
        "--clean_build"|"-cb")
            echo "Clean Build"
            CLEAN="true"
            ;;
        "--dir"|"-d")
            shift
            # NOTE: This should work even when the folder is missing
            BUILD_DIR=$(readlink --canonicalize-missing "$1")
            ;;
        "--prefix"|"-p")
            shift
            INSTALL_DIR=$(realpath "$1")
            ;;
        "--release"|"-r")
            echo "Release "
            CMAKE_BUILD_TYPE="RelWithDebInfo"
            ;;
        "--verbose"|"-v")
            echo "Verbose "
            VERBOSE_FLAG="-v"
            ;;
        "--gen_only")
            GEN_ONLY="true"
            ;;
        "--asan")
            ASAN="ON"
            ;;
        "--msan")
            MSAN="ON"
            ;;
        "--tsan")
            TSAN="ON"
            ;;
        "--ubsan")
            UBSAN="ON"
            ;;
        "--hw")
            EXEC_PLATFORM="HARDWARE"
            ;;
        "--fsim")
            EXEC_PLATFORM="FUNCSIM"
            ;;
        "--psim")
            EXEC_PLATFORM="PERFSIM"
            ;;
        "--tests")
            TEST_APPS="ON"
            ;;
        "--test_tables_root")
            shift
            if [ ! -d "$1" ] 
            then
                echo "Directory with tables DOES NOT exist." 
                echo "$1"
                exit 1
            else
                TEST_TABLES_ROOT=$(readlink -f "$1")
            fi
            ;;
        "--check_all")
            CHECK_ALL="ON"
            ;;
        "--ccache")
            CCACHE="ON"
            ;;
        "--benchmarks")
            BENCHMARKS="ON"
            ;;
        "--static_lib_check")
            STATIC_LIB_CHECK="ON"
            ;;
        "--use_make")
            GENERATOR="Unix Makefiles"
            ;;
        "-j")
            shift
            is_number=$(echo "$1" | grep -qE '^[0-9]+$'; echo $?) || true
            if [ "$is_number" != "0" ]
            then
                echo "Enter number of threads to use"
                usage
                exit 1
            else
                NUM_THREADS="$1"
            fi
            ;;
        *)
            usage
            exit 1
            ;;
    esac
    shift
done

if [ "$EXEC_PLATFORM" = "UNDEFINED" ];
then
  echo "The execution platform should be specified. See --help for detail."
  exit 1
fi

INSTALL_DIR=${INSTALL_DIR:=$BUILD_DIR/libs/}

if [ "x$CLEAN" = "xtrue" ];
then
    rm -fr $BUILD_DIR
fi

if [ "x$BUILD" = "xtrue" ];
then
    # Build user level libraries.
    echo CC=${CC:=clang}
    echo CXX=${CXX:=clang++}
    mkdir -p $BUILD_DIR; cd $BUILD_DIR
    cmake $PNM_ROOT -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
             -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX \
             -DPNM_USE_ASAN="$ASAN" -DPNM_USE_MSAN="$MSAN" -DPNM_USE_TSAN="$TSAN" \
             -DPNM_USE_UBSAN="$UBSAN" \
              ${LIBCXX_INSTALL_DIR:+-DLIBCXX_INSTALL_DIR="$LIBCXX_INSTALL_DIR"} \
             -DPNM_ENABLE_TESTS="$TEST_APPS" \
             ${TEST_TABLES_ROOT:+-DTEST_TABLES_ROOT="$TEST_TABLES_ROOT"} \
             -DPNM_EXECUTION_PLATFORM="$EXEC_PLATFORM" \
             -DPNM_USE_CCACHE=$CCACHE \
             -DPNM_ENABLE_BENCHMARKS=$BENCHMARKS \
             -DPNM_ENABLE_STATIC_LIB_CHECK=$STATIC_LIB_CHECK \
             -DPNM_ENABLE_COMMON_CHECKS=$CHECK_ALL \
             -G "$GENERATOR"
    if [ "x$GEN_ONLY" = "xfalse" ];
    then
        cmake --build . $VERBOSE_FLAG -j$NUM_THREADS; cmake --install .
    fi
fi
