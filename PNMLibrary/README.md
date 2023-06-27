## Coding guidelines
See [Coding-guidelines.md](docs/coding_guidelines/Coding-guidelines.md)

## Contents of subfolders:

* `sls/` - High level SLS operation specific logic. This should not depend on anything related to hardware
* `include/pnmlib/sls/` - User API for SLS operation

* `imdb/` - High level Scan operation specific logic. This should not depend on anything related to hardware
* `include/pnmlib/imdb/` - User API for Scan operation

* `secure/` - Everything related to secure SLS execution
* `include/pnmlib/secure/` - User API for secure execution

* `core/` - Device/simulator specific logic
* `include/pnmlib/core/` - User API for device logic

* `common/` - Generic utilities/algorithms/data structures
* `include/pnmlib/common/` - User-facing generic data structures/utilities/etc

* `hw/` - Some headers storing hardware configuration constants
* `tools/` - Tools for interactively communicating with hardware on low level
* `cmake/` - Extra cmake code, e.g. install commands
* `test/` - Tests
* `benchmarks/` - Benchmarks
* `scripts/` - Scripts for device setup/static analysis/input data generation
* `docs/` - Documentation


## Prerequisites
* [PNM enabled Linux kernel](https://github.samsungds.net/SAIT/PNMLinux/blob/axdimm-v6.1.6/PNM_HOW_TO_BUILD.md) for full support, or ...
* ... copy [include/uapi/linux/pnm_sls_mem_topology.h](https://github.samsungds.net/SAIT/PNMLinux/blob/axdimm-v6.1.6/include/uapi/linux/pnm_sls_mem_topology.h), [include/uapi/linux/sls_resources.h](https://github.samsungds.net/SAIT/PNMLinux/blob/axdimm-v6.1.6/include/uapi/linux/sls_resources.h) and [include/uapi/linux/imdb_resources.h](https://github.samsungds.net/SAIT/PNMLinux/blob/axdimm-v6.1.6/include/uapi/linux/imdb_resources.h) headers into `/usr/include/linux/` to enable local build.

Building is supported currently only on Ubuntu 22.04 with GCC 11 and Clang 14 with CMake 3.16.3, though the most thorough testing is performed against trunk LLVM from [their PPA](apt.llvm.org). Other configurations may work, but without any guarantees.

## How to build userspace library
```
$ ./scripts/build.sh --hw -cb -j 32 --tests --test_tables_root <path might be empty folder>
```
See `./scripts/build.sh -h` for more details

Or you can use CMake presets (see `CMakePresets.json` for details on all available configurations):
```
$ cmake --preset release && cmake --build build/presets/release && cmake --install build/presets/release
```

## Demo apps
1. [SecNDP Demo](./test/secndp_demo/README.md)
2. [IMDB Demo](./test/imdb_demo/README.md)

## How to run simple C++ test

### SLS tests

1. Make sure you built library with `--tests` and `--test_tables_root` flags.
2. [Build library](https://github.samsungds.net/SAIT/PNMLibrary#how-to-build-userspace-library).
3. Make sure you have SLS capable kernel and `sls_resource` kernel driver is presented:
```bash
$ sudo find /lib/modules/$(uname -r)/kernel/drivers/pnm -name 'sls_resource.ko'
```
4. Setup `sls_resource` driver:
```bash
$ sudo modprobe -v sls_resource
```
5. Setup DAX device (only for hardware SLS):
```bash
$ ./scripts/setup_dax_device.sh
```
6. Setup shared memory range (only for simulator):

Size of this range can be configured via `--mem` parameter, e. g. `--mem=32G`. This size should not be less than the size of SLS range.
```bash
$ ./build/tools/pnm_ctl setup-shm --sls --mem=32G
```

7. Run the test:
```
$ ./build/test/sample_app 10
```
8. After running the tests shared object can be safely destroyed:
```bash
$ ./build/tools/pnm_ctl destroy-shm --sls
```

### IMDB tests
1. Make sure you built library with `--tests` and `--test_tables_root` flags.
2. [Build library](https://github.samsungds.net/SAIT/PNMLibrary#how-to-build-userspace-library).
3. Make sure you have IMDB capable kernel and `imdb_resource` kernel driver is presented:
```bash
$ sudo find /lib/modules/$(uname -r)/kernel/drivers/pnm -name 'imdb_resource.ko'
```
4. Setup `imdb_resource` driver
```bash
$ modprobe -v imdb_resource
``` 
5. Setup shared memory objects (only for simulator):
```bash
$ ./build/tools/pnm_ctl setup-shm --imdb
```
6. Run the test:
```
$ ./build/test/imdb_test_app 4
```
7. After running the tests shared object can be safely destroyed:
```bash
$ ./build/tools/pnm_ctl destroy-shm --imdb
```

## How to run GoogleTest with cmake tests

1. Load CMake project with -DCMAKE_ENABLE_TESTS=ON.
```shell
cmake -S . -B <output_directory> -DCMAKE_ENABLE_TESTS=ON -DTEST_TABLES_ROOT=<path might be empty folder>
```
2. Build all target.
```shell
cmake --build <output_directory>
```
3. Move to test directory and run tests.
```shell
cd <output_directory>/test
ctest
```
You should get something like this as output:
```
Start  1: AES_OpenSSL.AES_128
1/12 Test  #1: AES_OpenSSL.AES_128 ..............   Passed    0.00 sec
Start  2: AES_OpenSSL.AES_192
2/12 Test  #2: AES_OpenSSL.AES_192 ..............   Passed    0.00 sec
Start  3: AES_OpenSSL.AES_256
3/12 Test  #3: AES_OpenSSL.AES_256 ..............   Passed    0.00 sec
Start  4: AES_NI.AES_128
4/12 Test  #4: AES_NI.AES_128 ...................   Passed    0.00 sec
Start  5: AES_NI.AES_192
5/12 Test  #5: AES_NI.AES_192 ...................   Passed    0.00 sec
Start  6: AES_NI.AES_256
6/12 Test  #6: AES_NI.AES_256 ...................   Passed    0.00 sec
Start  7: HelloTestSet1.BasicAssertions
7/12 Test  #7: HelloTestSet1.BasicAssertions ....   Passed    0.00 sec
Start  8: HelloTestSet2.BasicAssertions
8/12 Test  #8: HelloTestSet2.BasicAssertions ....   Passed    0.00 sec
Start  9: FixedVector.BaseOperation
9/12 Test  #9: FixedVector.BaseOperation ........   Passed    0.00 sec
Start 10: RankIO.SimpleCopy
10/12 Test #10: RankIO.SimpleCopy ................   Passed    0.00 sec
Start 11: EmbeddedTable.SimpleIteration
11/12 Test #11: EmbeddedTable.SimpleIteration ....   Passed    0.00 sec
Start 12: EmbeddedTable.BlockIteration
12/12 Test #12: EmbeddedTable.BlockIteration .....   Passed    0.00 sec

100% tests passed, 0 tests failed out of 12

Total Test time (real) =   0.02 sec
```

3.5 To manage tests order or include/exclude some tests use command line argumnets for ctest app.
More detail at [docs.](https://cmake.org/cmake/help/latest/manual/ctest.1.html)

To exclude large tests from testset use `-E` flag followed by regular expression. All tests with name
that matches to regexpr will excluded from run. Ex:
```bash
ctest -E '(TestApp)|(SecNDPTestFixture)'
```
The command above excludes test with names `*test_app*` and `*SecNDP_Stress*`.	

```bash
ctest -R '(TestApp)|(SecNDPTestFixture)'
```
This command runs test that match regular expression.

## How to write GoogleTest
1. Move to `test` folder in project root directory. 
2. Create new and/or move to exist directory with unit tests.
3. Create new file with `*.cpp` extension or open exist.
4. Write some awesome tests. You can use all staff from GoogleTest framework.
5. If build process for your tests requires additional library or special flags, you should create
a file with name `build.cmake` in the directory with tests. In this file you should specify build command
with regular cmake language.
6. Reload cmake project and check that your target exist.

## How to run DeepRecSys with PNM using functional simulator

1. [Build library](https://github.samsungds.net/SAIT/PNMLibrary#how-to-build-userspace-library).
2. Make sure you have SLS capable kernel and `sls_resource` kernel driver is presented:
```bash
$ sudo find /lib/modules/$(uname -r)/kernel/drivers/pnm -name 'sls_resource.ko'
```
3. Setup SLS resource:
```bash
$ sudo modprobe -v sls_resource
```
4. Setup shared memory range:
```bash
$ ./build/tools/pnm_ctl setup-shm --sls --mem=32G ```
  Size of this range can be configured via `--mem` parameter, e.g. `--mem=32G`. This size should
  not be less than the size of SLS range configured in device driver.
5. Build and install [Pytorch with PNM support](https://github.samsungds.net/SAIT/PNMPytorch), use `USE_PNM=1 PNM_INSTALL_DIR=<path>`
   switches to enable PNM build path.
6. Download [DeepRecSys with PNM support](https://github.samsungds.net/SAIT/PNMDeepRecSys) and run corresponding script:
```bash
$ cd ../ && git clone https://github.samsungds.net/SAIT/PNMDeepRecSys.git && cd PNMDeepRecSys
$ ./run_DeepRecSys_pnm.sh --tables $PATH_TO_TABLES/embedded.bin --use_pnm
```
  If you want to run in CPU mode, just remove `--use_pnm` flag from the command line:
```bash
  $ ./run_DeepRecSys_pnm.sh --tables $PATH_TO_TABLES/embedded.bin
```
7. After running the tests shared object can be safely destroyed:
```bash
$ ../PNMLibrary/build/tools/pnm_ctl destroy-shm --sls
```

## How to run Pytorch Facebook Research DLRM in PNM mode
1. [Build library](https://github.samsungds.net/SAIT/PNMLibrary#how-to-build-userspace-library).
2. Perform 2-4 steps from `How to run DeepRecSys with PNM using functional simulator`.
3. Download [Facebook Research DLRM with PNM support](https://github.samsungds.net/SAIT/PNMdlrm) and run corresponding script:
```bash
$ cd ../ && git clone https://github.samsungds.net/SAIT/PNMdlrm.git && cd PNMdlrm
$ ./run_test.sh --weights-path $PATH_TO_TABLES//embedded.bin --use-pnm <simple, secure>
```

## How to generate data for tests

- [Generate embedded tables](docs/howto/embedded_tables.md)
- [Generate indices](docs/howto/generate_indices.md)
- [Create dataset for secndp_demo](docs/howto/test_dataset.md)

## How to compile uml-diagrams

The UML diagrams can be compiled by [PlantUML](https://plantuml.com/) app.

1. Less preferred option:
In Ubuntu PlantUML can be installed by apt:
(But for some reason, on some correct diagrams, it gives an error)
```bash
sudo apt install plantuml
```
You can compile diagram by next command:
```bash
plantuml ./docs/diagrams # compile all diagrams in docs/diagrams folder
plantuml ./docs/diagrams/sls/decryption.txt # compile only docs/diagrams/sls/decryption.txt file
```

2. The most preferred option:
If you download standalone file from [here](https://plantuml.com/en/download) you can compile diagram by next command:
```bash
java -jar plantuml.jar ./docs/diagrams # compile all diagrams in docs/diagrams folder
java -jar plantuml.jar ./docs/diagrams/sls/decryption.txt # compile only docs/diagrams/sls/decryption.txt file
```

By default, the png file with diagram will be created. Extra command line option are presented
[here](https://plantuml.com/ru/command-line).

If your diagram is cropped, try with command line option:
```bash
java -jar plantuml.jar ./diagram.txt -DPLANTUML_LIMIT_SIZE=100000
```

3. Extra option:
Automatic diagramm generation using the [clang-uml](https://github.com/bkryza/clang-uml)
(Always a up to date diagram, but does not draw header-only classes)

- Install clang-uml
- Add .clang-uml file to project root directory. Its possible contents:
```
# Change to directory where compile_commands.json is
compilation_database_dir: build
# Change to directory where diagram should be written
output_directory: docs/diagrams
diagrams:
  pnm_class_diagram:
    type: class
    glob:
      - core/**
      - ai/**
      - imdb/**
      - include/**
    using_namespace:
      - pnm
      - sls
    include:
      # Include only elements in *** namespace
      namespaces:
        - pnm
        - sls
    exclude:
      # Exclude elements in *** namespace
      namespaces:
        - pnm::threads
        - pnm::utils
        - pnm::log
        - pnm::error
        - pnm::profile
        - std
        - fmt
        - spdlog
```
- Take steps from [here](https://github.com/bkryza/clang-uml/blob/master/docs/quick_start.md)

## How to run all common checks like clang-format, clang-tidy and headers usage
1. You need clang-format, clang-tidy, python3 and python3-clang installed, in Ubuntu:
```bash
sudo apt install clang-format clang-tidy python3-clang
```
2. Make sure you built library with `--check_all` flag or just run the script [common_checks.sh](scripts/common_checks.sh) manually.
All checks will/should be run after the build.
3. [Build library](https://github.samsungds.net/SAIT/PNMLibrary#how-to-build-userspace-library).

## pnm_ctl autocompletion

To turn on autocompletion for `pnm_ctl` tool, call
```bash
source $PNM_LIBRARY_DIR/tools/pnm_ctl/pnm_ctl_completion.sh
```

To be able to call `pnm_ctl` as a bash command, you can add path to executable to your `PATH` environment variable:
```bash
export PATH=${PATH:+${PATH}:}$PNM_LIBRARY_DIR/build/tools/
```
