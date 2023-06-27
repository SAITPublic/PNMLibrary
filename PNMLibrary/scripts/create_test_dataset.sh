#!/bin/sh
set -eu

if ! hash create_tables 2>/dev/null
then
  echo "App 'create_tables' not found. Add tools directory to PATH";
  exit 1
fi

if ! hash create_indices 2>/dev/null
then
  echo "App 'create_indices' not found. Add tools directory to PATH";
  exit 1
fi

usage() {
  echo "<Usage> build.sh <command> <arguments>

        <command>            <argument>                  <description>

        --root                                           Path root folder
        --dlrm                                           Create dataset for DLRM
        --secndp                                         Create dataset for SecNDP
        --force                                          Rewrite existing dataset
    "
}

DLRM=OFF
SECNDP=OFF
FORCE=OFF

while [ "$#" -ne 0 ]; do
  case "$1" in
  "--root")
    shift
    TEST_TABLES_ROOT=$1
    ;;
  "--dlrm")
    DLRM=ON
    ;;
  "--secndp")
    SECNDP=ON
    ;;
  "--force")
    FORCE=ON
    ;;
  *)
    usage
    exit 1
    ;;
  esac
  shift
done

check_and_create_impl(){
  path=$1
  if [ ! -d "$path" ]
  then
    echo "Directory $path not found. Creating..."
    mkdir $path;
    return 0
  else
    echo "Directory $path exists. Skipping..."
    return 0
  fi
}

check_and_create(){
  [ "$FORCE" = "ON" ] && (echo "Force flag set. Deleting directory $1..."; rm "$1" -rf)
  check_and_create_impl "$1"
}

check_and_create_table(){
  table_name=$1
  shift
  if [ ! -f "$table_name/embedded.bin" ]
  then
    echo "Embedded tables $table_name not found. Creating..."
    create_tables "$table_name" "$@"
    if [ ! $? ]
    then
      printf "\t\tCreation failed. Something goes wrong.\n"
      exit 1
    else
      printf "\t\tDone.\n"
    fi
  else
    echo "Embedded table $table_name exists. Skipping...";
  fi
}

check_and_create_indices(){
  table_name=$1
  indices_set=$2
  shift; shift
  if [ ! -f "$table_name/$indices_set.indices.bin" ]
  then
    echo "Indices set $indices_set not found. Creating...";
    create_indices "$table_name" "$indices_set" "$@"
    if [ ! $? ]
    then
       printf "\t\tCreation failed. Something goes wrong.\n"
       exit 1
    else
      printf "\t\tDone.\n"
    fi
  else
    echo "Indices set $indices_set exists. Skipping...";
  fi
}

check_and_create_impl "$TEST_TABLES_ROOT"

# Datasets for SECNDP
if [ "$SECNDP" = "ON" ]
then
  check_and_create "$TEST_TABLES_ROOT/position_16_50_500000"
  check_and_create_table "$TEST_TABLES_ROOT/position_16_50_500000" -n 50 --Ms 500000 --ms 500000 -c 16 -g position
  check_and_create_indices "$TEST_TABLES_ROOT/position_16_50_500000" "seq_16" --Ml 40 --ml 40 --bs 16 -g sequential --et uint32_t
  check_and_create_indices "$TEST_TABLES_ROOT/position_16_50_500000" "rand_16" --Ml 40 --ml 40 --bs 16 -g random --et uint32_t

  check_and_create "$TEST_TABLES_ROOT/random_16_50_500000"
  check_and_create_table "$TEST_TABLES_ROOT/random_16_50_500000/" -n 50 --Ms 500000 --ms 500000 -c 16 -g random
  check_and_create_indices "$TEST_TABLES_ROOT/random_16_50_500000/" "seq_16" --Ml 40 --ml 40 --bs 16 -g sequential --et uint32_t
  check_and_create_indices "$TEST_TABLES_ROOT/random_16_50_500000/" "rand_16" --Ml 40 --ml 40 --bs 16 -g random --et uint32_t

  check_and_create "$TEST_TABLES_ROOT/position_16_6_500000"
  check_and_create_table "$TEST_TABLES_ROOT/position_16_6_500000/" -n 6 --Ms 500000 --ms 500000 -c 16 -g position
  check_and_create_indices "$TEST_TABLES_ROOT/position_16_6_500000/" "seq_128" --Ml 40 --ml 40 --bs 128 -g sequential --et uint32_t
  check_and_create_indices "$TEST_TABLES_ROOT/position_16_6_500000/" "rand_128" --Ml 40 --ml 40 --bs 128 -g random --et uint32_t

  check_and_create "$TEST_TABLES_ROOT/random_16_6_500000"
  check_and_create_table "$TEST_TABLES_ROOT/random_16_6_500000/" -n 6 --Ms 500000 --ms 500000 -c 16 -g random
  check_and_create_indices "$TEST_TABLES_ROOT/random_16_6_500000/" "seq_128" --Ml 40 --ml 40 --bs 128 -g sequential --et uint32_t
  check_and_create_indices "$TEST_TABLES_ROOT/random_16_6_500000/" "rand_128" --Ml 40 --ml 40 --bs 128 -g random --et uint32_t
fi


# Datasets for DLRM
if [ "$DLRM" = "ON" ]
then
  check_and_create "$TEST_TABLES_ROOT/DLRM_INT"
  check_and_create_table "$TEST_TABLES_ROOT/DLRM_INT" -n 60 --Ms 500000 --ms 500000 -c 16 -g random

  check_and_create "$TEST_TABLES_ROOT/DLRM_FLOAT"
  check_and_create_table "$TEST_TABLES_ROOT/DLRM_FLOAT" -n 60 --Ms 500000 --ms 500000 -c 16 -g float_tapp --ga "141 16"
fi
