file(GLOB SLS_SIMULATOR_SRC *.cpp)

# [TODO: @a.korzun] Making this library OBJECT helps to resolve a cyclic dependency:
# (sls_simulator -> sls_device[NBufferedMemory] -> sls_memblock -> sls_simulator).
# The proper solution might be to break the cycle by separating NBufferedMemory from `sls_device`.
# Alternatively, we can just accept that the `sls_device` library and its subparts are built as 
# a whole, using object libraries.
add_library(sls_simulator OBJECT ${SLS_SIMULATOR_SRC})
target_link_libraries(sls_simulator PUBLIC pnm_common)
