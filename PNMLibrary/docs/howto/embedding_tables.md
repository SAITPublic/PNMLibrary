# Tables generation tool

## Build

CLI app for embedding tables generation built with any other build by default

- [General build instruction](../../README.md)

The app will be located in `<binary_dir>/tools` subdirectory with name `create_tables`.

## Usage

There are two ways to generate embedding tables: run CLI app or use API.

### CLI App

To create embedding tables run `create_tables` app with specific arguments. To get all argument list run apps without
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
   --num_tables, -n           number of tables          Number of embedding tables
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

In the directory `test_root` there are two files will be created: binary file `embedding.bin` with tables and text files
`embedding.info` with tables' parameters.

```shell
$ ls test_root/ -l
total 698852
-rw-rw-r-- 1 yalavrinenko yalavrinenko 715612544 мая 11 12:30 embedding.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       940 мая 11 12:30 embedding.info
```

The app assume that in one directory there is only one set of embedding tables.

More examples of app calls:

```shell
#tables with position defined entries
$ ./create_tables positioned_tables --num_tables 42 -Ms 1000000 -ms 1000000 --cols 4 -g position
#tables with float values. -ga is flag to pass knum_idx_values to generator
$ ./create_tables float_values --num_tables 10 -Ms 10000 -ms 10000 --cols 16 -g float_tapp -ga "17"
```

### Tables generation API

To use API you should build tools library. The API presented by set of classes that allow to generate tables.
There are several generators and one factory that can build generators.

## All-in-one function

You can use function `tools::gen::sls::get_or_create_test_tables` from `tools/datagen/sls/utils.h` file.
It will generate test tables (if needed) and load it into a given vector. Its signature:

```c++
void get_or_create_test_tables(const std::filesystem::path &root,
                               const std::string &generator_name,
                               const std::string &generator_args,
                               size_t sparse_feature_size,
                               size_t num_tables,
                               size_t emb_table_len,
                               std::vector<uint8_t> &tables)
```

Parameters description:

- `root`: directory, where files will be generated
- `generator_name`: Name of generator. List of supported names:
  + `"random"` - uniformly distributed random value from 0 to `std::numeric_limits<uint16_t>::max()` (type `uint32_t`)
  + `"positioned"` - value of entry depends only on position in tables, i.e. table number, row number and column number (type `uint32_t`)
  + `"uint32_t_tapp"` - values that reproduce `test_app` entries (type `uint32_t`)
  + `"float_tapp"` - values that reproduce `test_app` entries (type `float`)
  + `"fixed"` - return fixed value (type `uint32_t`)
- `generator_args`: string with arguments. Arguments to a given generator:
  + `"random"` - None
  + `"positioned"` - None
  + `"uint32_t_tapp"` - `uint32_t kvalue`, `uint32_t sparse_feature_size` (see [NumericEntry::operator()](https://github.samsungds.net/SAIT/PNMLibrary/blob/pnm/tools/datagen/sls/tables_generator/default_entry_gens.h))
  + `"float_tapp"` - `uint32_t kvalue`, `uint32_t sparse_feature_size`
  + `"fixed"` - `uint32_t` value to be returned
- `sparse_feature_size`: number of columns in each table
- `num_tables`: number of tables
- `emb_table_len`: number of rows in each table
- `tables` - output vector with tables

## Generate tables

There are three steps to generate tables in code:

1. Create tables generator (ex. call factory with preregistered builders). Possible generator names are listed above.

```c++
auto table_generator = TablesGeneratorFactory::default_factory().create(
        generator_name, generator_args);
```

2. Create object with tables structure

```c++
const TablesInfo info(num_tables, sparse_feature_size,
                      std::vector<uint32_t>(num_tables, emb_table_len));
```

3. Call method of `table_generator` with `tinfo` object and path to directory where tables should be stored

```c++
gen->create_and_store(root, tinfo);
```

or only with `tinfo` to get vector of bytes with tables data

```c++
gen->create(tinfo);
```

## Load tables

The embedding tables are stored as an array of bytes in file `embedding.bin` with info file `embedding.info`.
These files can be loaded in your app manually or by call function `get_test_tables_mmap` from `tools/datagen/sls/utils.h`.

The `get_test_tables_mmap` function return structure with tables information and read-only pointer to embedding tables.
If you need to modify tables, you can copy data to vector from `pnm::views::common`.

```c++
auto tables = tools::gen::sls::get_test_tables_mmap(root);

for (auto rows_count : tables.info.rows()) {
  std::cout << rows_count << " ";
}
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