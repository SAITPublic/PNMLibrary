# Indices generation tool

## Build

CLI app for indices generation built with any other build by default

- [General build instruction](../../README.md)

The app will be located in `<binary_dir>/tools` subdirectory with name `create_indices`.

## Usage

There are two ways to generate indices: run CLI app or use API.

### CLI App

To create indices for the embedding tables set run `create_tables` app. The generator properties and embedding
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

There are at least two arguments required to generate indices set: path to embedding tables and prefix -- string that
will be used as a prefix for indices and golden vector files. The extra options allow to customise index generation
process.

```shell
./create_indices test/ --prefix set_random -Mb 200 -mb 150 -l 2000 -g random -et uint32_t
./create_indices test/ --prefix set_seq -Mb 200 -mb 150 -l 2000 -g sequential -et uint32_t
```

The command above will create set of random indices and evaluate golden vector for selected embedding tables and
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
-rw-rw-r-- 1 yalavrinenko yalavrinenko 702238784 мая 16 15:53 embedding.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       940 мая 16 15:53 embedding.info
-rw-rw-r-- 1 yalavrinenko yalavrinenko   1650176 мая 16 16:02 set_random.golden.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko    206272 мая 16 16:02 set_random.indices.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       626 мая 16 16:02 set_random.indices.info
-rw-rw-r-- 1 yalavrinenko yalavrinenko   1651520 мая 16 16:02 set_seq.golden.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko    206440 мая 16 16:02 set_seq.indices.bin
-rw-rw-r-- 1 yalavrinenko yalavrinenko       626 мая 16 16:02 set_seq.indices.info
```

## Indices generation API

To use API you should build tools library. The API presented by set of classes that allow to generate indices and golden vectors.
There are several generators and factories to simplify generation process.

## All-in-one function

You can use function `tools::gen::sls::get_or_create_test_indices` from `tools/datagen/sls/utils.h` file.
It will generate test indices and lengths (if needed) and load it into given vectors.
Golden vector will be generated (if needed), but won't be loaded.
Its signature:

```c++
void get_or_create_test_indices(const GeneratorWithOverflowParams &params,
                                std::vector<uint32_t> &lengths,
                                std::vector<uint32_t> &indices)
```

`GeneratorWithOverflowParams`:

```c++
struct GeneratorWithOverflowParams {
  size_t max_num_lookup;
  size_t min_num_lookup;
  int mini_batch_size;
  std::filesystem::path root;
  std::string prefix;
  std::string generator_lengths_name;
  std::string overflow_type;
  std::string generator_indices_name;
  std::string entry_type;
};
```

Parameters description:

- `params`: input parameters
  + `min_num_lookup`: Minimum number of lookups for each table
  + `max_num_lookup`: Maximum number of lookups for each table
  + `mini_batch_size`: Batch size for each table
  + `root`: Directory, where files will be generated
  + `prefix`: Prefix for every file name that will be generated
  + `generator_lengths_name`: Name of lengths generator. List of supported names:
    - `"random"`: Random lengths generator
    - `"random_from_lookups_per_table`: Random table generator with same lengths for every `mini_batch_size`
  + `overflow_type`: string with overflow type for `"random"` length generator.
    - `0` - No instruction overflow
    - `-1` - Instruction overflow in one random table
    - `n` > 0 - Instruction overflow in each n'th table
  + `generator_indices_name` - Name of indices generator. List of supported names:
    - `"random"` - Random indices generator
    - `"sequential"` - Sequential indices generator
  + `entry_type` - type of entry in embedding tables. Supported types: `"uint32_t"`, `"uint64_t"`, `"float"`
- `lengths`: output vector with lengths
- `indices`: output vector with indices

## Generate indices

There are several steps to generate indices in code:

1. Get or create object with table structure

```c++
const TablesInfo tinfo(num_tables, sparse_feature_size,
                      std::vector<uint32_t>(num_tables, emb_table_len));
```

or use [generated tables](https://github.samsungds.net/SAIT/PNMLibrary/blob/pnm/docs/howto/embedding_tables.md)

```c++
auto tables = get_test_tables_mmap(root);
const TablesInfo &tinfo = tables.info;
```

2. Create length generator

```c++
auto length_generator = LengthsGeneratorFactory::default_factory().create(
        generator_lengths_name, overflow_type);
```

3. Generate length vector

```c++
auto lengths =
     length_generator->create(tinfo.num_tables(), min_num_lookup,
                              max_num_lookup, mini_batch_size);
```

4. Create indices generator

```c++
auto indices_generator = IndicesGeneratorFactory::default_factory().create("sequential");
```

5. Create object with indices structure using length vector

```c++
const IndicesInfo info(mini_batch_size, lengths);
```

6. Call method of `indices_generator` with `iinfo` and `tinfo` objects and required path to the directory with embedding tables

```c++
indices_generator->create_and_store(root, "iprefix", iinfo, tinfo);
```

or if you want keep it in memory

```c++
auto indices = indices_generator->create(iinfo, tinfo);
```

### Generate golden vector

To generate golden vector create golden vector generator and call `compute_golden_sls` or `compute_and_store_golden_sls` method.

```c++
auto indices = tools::gen::sls::get_test_indices(root, "iprefix");
auto tables = tools::gen::sls::get_test_tables_mmap(root);

auto sls_proc = GoldenVecGeneratorFactory::default_factory().create("uint32_t");
sls_proc->compute_and_store_golden_sls(root, "iprefix", tables, indices);
```

or

```c++
auto indices = indices_generator->create(iinfo, tinfo);
auto emb_tables = tables_generator->create(tinfo);

auto sls_proc = GoldenVecGeneratorFactory::default_factory().create("uint32_t");
auto golden = sls_proc->compute_golden_sls(emb_tables.data(), tinfo, indices.data(), iinfo);
```

## Load indices

The `get_test_indices` function return structure with indices meta information and open binary stream from
`<prefix>.indices.bin`

```c++
auto indices = tools::gen::sls::get_test_indices(root, "iprefix");
```

### Load golden vector

The golden vector can be loaded by function `get_golden_vector`. This function opens the binary stream for
`<prefix>.golden.bin`

```c++
auto golden = tools::gen::sls::get_golden_vector(root, "iprefix");
```

## Internal indices structure

Indices are stored as a sequence of 32-bit integer numbers. Firstly the indices for the lookups in the first table
will be stored, next -- for the second and so on.

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

### Internal golden vector structure

A golden vector are stored as an array of values with type `entry_t` in order:

```
sls(table_0, batch_00),sls(table_0, batch_01), sls(table_0, batch_02),
sls(table_1, batch_10),sls(table_1, batch_11), sls(table_1, batch_12), ...
``` 
