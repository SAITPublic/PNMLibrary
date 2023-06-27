# Indices generation tool

## Build

CLI app for indices generation built with any other build by default

- [General build instruction](../../README.md)

The app will be located in `<binary_dir>/tools` subdirectory with name `create_indices`.

## Usage

There are two ways to generate indices: run CLI app or use API.

### CLI App

To create indices for the embedded tables set run `create_tables` app. The generator properties and embedded
tables path are specified by command line arguments. The list of argument can be obtained by `-h` (`--help`)
arg.

```shell
./create_tables -h
```

The output will be like:

```
./create_table tables_root prefix <command> <args>

path -- path to the root directory where test tables will locate

<command>       <args>        <description>

   --prefix, -p               string                    Prefix for indices file
   --max_batch_count, -Mb     number of batches         Max number of batches for each table.
   --min_batch_count, -mb     number of batches         Min number of batches for each table.
   --lookups, -l              number of lookups         Number of lookups per table
   --generator, -g            name of generator         Name of generator that create indices
   --gargs, -ga               string with args          Arguments for generator packed into a string
   --entry_t, -et             string                    Type of table's entry. Required for golden psum vec.

The generator names are:
  random -- create indices with random value
  sequential -- create set of indices with value in range [0, minibatch_size)
```

There are at least two arguments required to generate indices set: path to embedded tables and prefix -- string that
will be used as a prefix for indices and golden vector files. The extra options allow to customise index generation
process.

```shell
./create_indices test/ --prefix set_random -Mb 200 -mb 150 -l 2000 -g random -et uint32_t
./create_indices test/ --prefix set_seq -Mb 200 -mb 150 -l 2000 -g sequential -et uint32_t
```

The command above will create set of random indices and evaluate golden vector for selected embedded tables and
generated indices. The number of batches for each table will be random value from range `[-Mb, -mb]`. The number
of lookups is fixed for all batches and equal to `-l`.

To calculate golden vector the app should know table's entry type. This value is specified by arg `--entry_t (-et)`.
App will perform SLS and store the result as a binary file.

After command above there are 6 files will be created. Three files with prefix  `set_random` and three with `set_seq`
for random and sequential generators respectively.

In group of three files two files contain binary information about indices (`.indices.bin`) and golden
vector (`.golden.bin`). The last file is a text file (`.indices.info`) and contains meta for indices binary file.

```shell
$ ls -l test/
total 689436
-rw-rw-r-- 1 yalavrinenko yalavrinenko 702238784 мая 16 15:53 embedded.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       940 мая 16 15:53 embedded.info
-rw-rw-r-- 1 yalavrinenko yalavrinenko   1650176 мая 16 16:02 set_random.golden.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko    206272 мая 16 16:02 set_random.indices.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       626 мая 16 16:02 set_random.indices.info
-rw-rw-r-- 1 yalavrinenko yalavrinenko   1651520 мая 16 16:02 set_seq.golden.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko    206440 мая 16 16:02 set_seq.indices.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       626 мая 16 16:02 set_seq.indices.info
```

### Indices generation API

To use API you should build tools library. The API presented by set of classes that allow to generate indices
and golden vectors. There are several generators and factories to simplify generation process.
The usage of API was presented in `test/utils_test/indices_gen.cpp`.

There are four steps to generate indices in code:

1. create indices generator (ex. call factory with preregistered builders)

```c++
auto igen = IndicesGeneratorFactory::default_factory().create("sequential");
```

2. create object with indices structure

```c++
class IndicesInfo {
//.......
  IndicesInfo(size_t num_lookup, std::vector<size_t> minibatch_sizes);
//.......
};

auto num_lookup = 10;
std::vector<size_t> minibatch_sizes{5, 6, 7, 8, 9} 

IndicesInfo iinfo(num_lookup, std::move(minibatch_sizes));
```

3. create or load structure with embedded tables meta

```c++
class TablesInfo {
//.......
  TablesInfo(size_t num_tables, size_t cols, std::vector<size_t> rows_in_tables);
//.......
};

auto num_tables = 5;
auto cols = 16;
std::vector<size_t> rows_in_table{30, 40, 20, 100, 14};

TablesInfo tinfo(num_tables, cols, std::move(rows_in_table));
```

or

```c++
auto tables = axdimm::tests::get_test_tables_mmap(root);
auto tinfo = tables.info;
```

4. call method of `igen` with `iinfo` and `tinfo` objects and required path to the directory with embedded tables

```c++
igen->create_and_store(root, "seq_ok", iinfo, tinfo);
```

or if you want keep it in memory

```c++
auto indices = igen->create(iinfo, tinfo);
```

To generate golden vector create SLS computer and call `compute_golden_sls` or `compute_and_store_golden_sls` method.

```c++
auto indices = axdimm::tests::get_test_indices(root, "seq_ok");
auto tables = axdimm::tests::get_test_tables_mmap(root);

auto sls_proc = GoldenVecGeneratorFactory::default_factory().create("uint32_t");
sls_proc->compute_and_store_golden_sls(root, "seq_ok", tables, indices);
```

or

```c++
auto indices = igen->create(iinfo, tinfo);
auto emb_tables = igen->create(tinfo);

auto sls_proc = GoldenVecGeneratorFactory::default_factory().create("uint32_t");
auto golden = sls_proc->compute_golden_sls(emb_tables.data(), tables_meta, indices.data(), indices_meta);
```

Indices are stored as a sequence of 32-bit integer numbers. Firstly the indices for the lookups in the first table
will be stored, next -- for the second and so on.

A golden vector are stored as an array of values with type `--entry_t` in order:

```
sls(table_0, batch_00),sls(table_0, batch_01), sls(table_0, batch_02),
sls(table_1, batch_10),sls(table_1, batch_11), sls(table_1, batch_12), ...
``` 

### Load indices

The `get_test_indices` function return structure with indices meta information and open binary stream from
`<prefix>.indices.bin`

```c++
auto indices = axdimm::tests::get_test_indices(root, "seq_ok");
```

The golden vector can be loaded by function `get_golden_vector`. This function opens the binary stream for
`<prefix>.golden.bin`

```c++
auto golden = axdimm::tests::get_golden_vector(root, "seq_ok");
```

## Internal indices structure

If we were to generate a minibatch of indices for `3 tables`, where we want `2` SLS queries per table and SLS operations
for each respective table to sum `{4, 3, 4}` indices, then we could do this like so:

```c++
TablesInfo tinfo{...};
...
IndicesInfo iinfo(2, queries_per_table);
auto igen = IndicesGeneratorFactory::default_factory().create(...);
auto indices = igen->create(iinfo, tinfo);
```

Or like so:

```c++
std::vector<uint32_t> indices = {
  // 2 SLS queries of size 4 for table 0
  0, 1, 2, 3, // indices for one SLS query
  2, 5, 4, 3,

  // 2 SLS queries of size 3 for table 1
  4, 5, 4, // indices for one SLS query
  3, 1, 6,

  // 2 SLS queries of size 4 for table 2
  3, 2, 2, 4, // indices for one SLS query
  1, 1, 3, 1,
};
```

And then later on when we need to use "lengths" when computing SLS (e.g. in `secure` API), we could do it like so:

```c++
std::vector<uint32_t> lengths = iinfo.lengths();
```

Or like so:

```c++
std::vector<uint32_t> lengths = {
  4, 4, // 2 queries on table 0, each one is an operation involving 4 indices
  3, 3, // 2 queries on table 1, each one is an operation involving 3 indices
  4, 4, // 2 queries on table 2, each one is an operation involving 4 indices
};
```