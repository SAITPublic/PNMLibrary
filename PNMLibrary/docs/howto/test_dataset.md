## How to create test dataset for secndp_demo

1. Tools built with any other build by default.

- [General build instruction](../../README.md)

2. Add tools directory with `create_tables` and `create_indices` to `PATH`

```shell
export PATH=$PATH:`pwd`/build/tools
```

3. Run `create_test_dataset.sh` with path to directory where tests should be located.

```shell
./scripts/create_test_dataset.sh --root ~/path/to/test_folder/testset_1 --secndp
```

Script will create `testset_1` directory and several subdirectories with embedded tables, indices and golden vectors.
You can look at the dataset via common hex editors.

This script also allows to create embedded tables for DLRM.

```shell
./scripts/create_test_dataset.sh --root ~/path/to/test_folder/testset_1 --dlrm
```

In this case two directories (`DLRM_INT` and `DLRM_FLOAT`) will be created in `~/path/to/test_folder/testset_1`.

The `--secndp` and `--dlrm`options can be combined. If you want rewrite existing input data you can use `--force` flag.

```shell
./scripts/create_test_dataset.sh --root ~/path/to/test_folder/testset_1 --dlrm --secndp --force
```