# Tables generation tool

## Build

CLI app for embedded tables generation built with any other build by default

- [General build instruction](../../README.md)

The app will be located in `<binary_dir>/tools` subdirectory with name `create_tables`.

## Usage

There are two ways to generate embedded tables: run CLI app or use API.

### CLI App

To create embedded tables run `create_tables` app with specific arguments. To get all argument list run apps without
arguments or with `-h` flag

```shell
./create_tables -h
```

The output will be like:

```
./create_table path <command> <args>

path -- path to the root directory where test tables will locate

<command>       <args>        <description>

   --help, -h                                           Help message
   --num_tables, -n           number of tables          Number of embedded tables
   --max_table_size, -Ms      number of rows            Max number or rows for each table.
   --min_table_size, -ms      number of rows            Min number or rows for each table.
   --cols, -c                 number of cols            Number of columns in table (sparse_feature_size)
   --generator, -g            name of generator         Name of generator that create tables
   --gargs, -ga               string with args          Arguments for generator packed into a string

The generator names are:
  random -- create tables with random values
  position -- create tables with value evaluated like (tid << 16) + (rid << 8) + cid;
  float_tapp -- create tables with float values from test app
  uint32_t_tapp -- create tables with uint32_t values from test app
```

To create tables you should specify path to directory where tables will be created and all or part of arguments.

```shell
./create_tables test_root/ --num_tables 148 -Ms 100000 -ms 50000 --cols 16 -g random
```

In the directory `test_root` there are two files will be created: binary file `embedded.bin` with tables and text files
`embedded.info` with tables' parameters.

```shell
$ ls test_root/ -l
total 698852
-rw-rw-r-- 1 yalavrinenko yalavrinenko 715612544 мая 11 12:30 embedded.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       940 мая 11 12:30 embedded.info
```

The app assume that in one directory there is only one set of embedded tables.

More examples of app calls:

```shell
#tables with position defined entries
$ ./create_tables positioned_tables --num_tables 42 -Ms 1000000 -ms 1000000 --cols 4 -g position
#tables with float values. -ga is flag to pass knum_idx_values to generator
$ ./create_tables float_values --num_tables 10 -Ms 10000 -ms 10000 --cols 16 -g float_tapp -ga "17"
```

### Tables generation API

To use API you should build tools library. The API presented by set of classes that allow to generate tables.
There are several generators and one factory that can build generators. The usage of API was presented in
`test/utils_test/tables_load.cpp`.

There are three steps to generate tables in code:

1. create tables generator (ex. call factory with preregistered builders)

```c++
auto gen = TablesGeneratorFactory::default_factory().create("position"));
```

2. create object with tables structure

```c++
TablesInfo tinfo{4, 16, {5, 20, 56, 42}};
```

3. call method of `gen` with `tinfo` object and path to directory where tables should be stored

```c++
gen->create_and_store(root, tinfo);
```

or only with `tinfo` to get vector of bytes with tables data

```c++
gen->create(tinfo);
```

## Load tables

The embedded tables are stored as an array of bytes in file `embedded.bin` with info file `embedded.info`. These files
can be loaded in your app manually or by call function `get_test_tables_mmap` from `tools/utils.h`.

The `get_test_tables_mmap` function return structure with tables information and read-only pointer to embedded tables.
If you need to modify tables, you can copy data to vector from `axdimm::common_view`.

```c++
auto tables = axdimm::tests::get_test_tables_mmap(root);

for (auto rows_count : tables.info.rows())
  std::cout << rows_count << " ";
std::cout << tables.info.cols() << "\n";
std::cout << tables.info.num_tables() <<  "\n";

auto tables_view = tables.mapped_file.get_view<uint32_t>();
auto tables_vector = std::vector(tables_view.begin(), tables_view.end());
```

## Internal table structure

If we were to create `3 tables` of type `uint32_t` with `16 columns` and `row sizes {6, 7, 5}` then we could do this
like so:

```c++
TablesInfo tinfo(3, 16, {6, 7, 5});
auto tgen = TablesGeneratorFactory::default_factory().create(...);
auto emb_tables = tgen->create(tinfo);
```

Or like so:

```c++
std::vector<uint32_t> emb_tables = {
  // Table 0
  // row 0
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 1
  101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
  // row 2
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 3
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 4
  201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216,
  // row 5
  16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,

  // Table 1
  // row 0
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 1
  101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
  // row 2
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 3
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 4
  201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216,
  // row 5
  16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
  // row 6
  16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,

  // Table 2
  // row 0
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 1
  101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
  // row 2
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 3
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  // row 4
  201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216
};
```