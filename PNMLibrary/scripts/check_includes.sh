#!/bin/bash
set -eu

ROOT=$(readlink -f $(dirname $0)/..)

function pnmlib() {
    echo "include/pnmlib/${1}"
}

COMP_DB=$ROOT/build

while [ "$#" -ne 0 ]
do
    case "$1" in
        "--comp-db-dir"|"-d")
            shift
            COMP_DB=$(readlink -f "$1")
            ;;
        *)
            echo "Specify compilation dir with --comp-db-dir (default = build)"
            exit 1
            ;;
    esac
    shift
done

echo "Will use compilation database from directory '$COMP_DB'"

cd $ROOT

echo "Checking rule: 'sls' headers can't be included to anywhere except 'benchmarks', 'core/context', 'core/operation_runner/reduction', 'core/device/sls', 'secure', 'test', and 'tools'"
./scripts/check_includes.py -d $COMP_DB -r . -v sls:$(pnmlib sls) -e sls:$(pnmlib sls):secure:$(pnmlib secure):tools:test:benchmarks:core/context:core/operation_runner/reduction:core/device/sls

echo "Checking rule: 'common' can include only 'common' and 'common' API"
./scripts/check_includes.py -d $COMP_DB -r . -a common:$(pnmlib common) -c common

echo "Checking rule: 'common' API can include only 'common' API"
./scripts/check_includes.py -d $COMP_DB -r . -a $(pnmlib common) -c $(pnmlib common)

echo "Checking rule: 'tools' (except 'tools/datagen' and 'tools/test_utils') can include 'tools', 'core' API, 'sls' API, 'imdb' API, 'common', 'common' API"
./scripts/check_includes.py -d $COMP_DB -r . -a tools:$(pnmlib core):$(pnmlib sls):common:$(pnmlib common):$(pnmlib imdb) -c tools -e tools/datagen:tools/test_utils
echo "Checking rule: 'tools/datagen' can include 'tools/datagen', 'imdb' API, 'common', 'common' API, 'imdb/software_scan', 'imdb/utils'"
./scripts/check_includes.py -d $COMP_DB -r . -a tools/datagen:common:$(pnmlib common):$(pnmlib imdb):imdb/utils:imdb/software_scan -c tools/datagen
echo "Checking rule: 'tools/test_utils' can include 'secure', 'secure' API, 'common', 'common' API, 'core' API, 'sls' API"
./scripts/check_includes.py -d $COMP_DB -r . -a secure:$(pnmlib secure):common:$(pnmlib common):$(pnmlib core):$(pnmlib sls) -c tools/test_utils

echo "Checking rule: 'core' API allowed 'common' API and 'core' API"
./scripts/check_includes.py -d $COMP_DB -r . -a $(pnmlib core):$(pnmlib common) -c $(pnmlib core)

echo "Checking rule: Secure common code can't depend on 'plain' SecNDP implementation."
./scripts/check_includes.py -d $COMP_DB -r . -v secure/plain -c secure/common

echo "Checking rule: Encryption Engine can't depend on SecNDP execution mode."
./scripts/check_includes.py -d $COMP_DB -r . -v secure/common:secure/plain -c secure/encryption

echo "Checking rule: 'secure' can include 'sls' API, 'common', 'common' API, 'core' API, 'secure', 'secure' API"
./scripts/check_includes.py -d $COMP_DB -r . -a $(pnmlib sls):$(pnmlib core):$(pnmlib common):$(pnmlib secure):common:secure -c secure

demo_tests_allowed=$(pnmlib sls):$(pnmlib core):$(pnmlib common):common:tools/datagen:test/mocks

echo "Checking rule: TestApp can use only 'common', 'common' API, 'core' API, 'sls' API, 'tools/datagen', 'test/mocks'"
./scripts/check_includes.py -d $COMP_DB -r . -a ${demo_tests_allowed}:test/sls_app/api:test/sls_app/sample_app -c test/sls_app/sample_app
./scripts/check_includes.py -d $COMP_DB -r . -a ${demo_tests_allowed}:test/sls_app/api:test/sls_app/huge_tables -c test/sls_app/huge_tables

secure_demo_tests_allowed=secure:$(pnmlib secure):tools/test_utils:${demo_tests_allowed}

echo "Checking rule: SecNDP demos can include 'common', 'common' API, 'core' API, 'sls' API, 'secure', 'secure' API, 'tools/test_utils', 'tools/datagen'"
./scripts/check_includes.py -d $COMP_DB -r . -a ${secure_demo_tests_allowed}:test/secndp_demo -c test/secndp_demo
./scripts/check_includes.py -d $COMP_DB -r . -a ${secure_demo_tests_allowed} -c test/secure_exec

echo "Checking rule: IMDB can include only 'imdb', 'common', 'core'"
./scripts/check_includes.py -d $COMP_DB -r . -a $(pnmlib core):$(pnmlib common):$(pnmlib imdb):common:core:imdb -c imdb
