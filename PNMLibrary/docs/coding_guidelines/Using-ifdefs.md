# Outline
Try avoiding ifdefs as much as possible, consider C-style ifs or `constexpr if` instead.

# Reason
We are trying to minimize using of ifdefs due to the following reason: ifdef'ed code **doesn't get compiled** if macros is not defined.

This leads to the following problems:

* The code under ifdef can easily become stale (e.g. after some refactoring) leading to broken builds.
* Static analysis tools usually work on preprocessed code which will miss ifdef'ed code unless static analyzer hacks compiler preprocessor.

# Approach
Sometimes ifdefs are inevitable (e.g. predefined `NDEBUG` macros and heavy debug checks). In this case we try to convert ifdefs into C-style ifs or `constexpr if` whenever possible.
Considering `NDEBUG`:

1) Define additional macros `DEBUG` in well defined place (e.g. `axdimm_common.h`) as follows:
```c++
#ifdef NDEBUG
#define DEBUG 0
#else
#define DEBUG 1
#endif
```
2) Use `DEBUG` in your code as follows:
```c++
if (DEBUG) {
  inst.print_processed_inst(job_info.rank_id);
}
```

3) In this case even if we build in **Release** mode, compiler will perform syntactic checks for the code under `DEBUG` branch. Note that modern compilers are smart enough to figure out that this code is dead and eliminate it later in optimizing passes so there is no performance penalty for the branch.
