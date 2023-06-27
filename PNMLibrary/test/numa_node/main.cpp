#include <fmt/core.h>

#include <numa.h>
#include <numaif.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

#define ALWAYS_INLINE __attribute__((always_inline)) inline

#define bitsperlong (8 * sizeof(unsigned long))
#define CACHE_LINE_SIZE 64

ALWAYS_INLINE void flush(const uint8_t *src, uint64_t size) {
  for (uint64_t i = 0; i < size; i += CACHE_LINE_SIZE) {
    asm volatile("clflush 0(%0)" ::"r"(src + i));
  }
}

ALWAYS_INLINE void mfence() { asm volatile("mfence" ::: "memory"); }

[[maybe_unused]] void wait_migration_done(int src_numa_node) {
  long long free_size = 0;
  long const src_node_size = numa_node_size64(src_numa_node, &free_size);

  fmt::print("Numa node {} MemUsed is {} Kb\n", src_numa_node,
             (src_node_size - free_size) / 1024);

  while (free_size != src_node_size) {
    numa_node_size64(src_numa_node, &free_size);
  }
}

void check_value(void *p, size_t size, int val) {
  flush((uint8_t *)p, size);
  mfence();
  for (size_t i = 0; i < size; ++i) {
    if (((char *)p)[i] != val) {
      fmt::print("{}: Expected: {}, got {}\n", i, val, ((char *)p)[i]);
    }
  }
}

int addr_on_node(void *addr) {
  int node;
  int ret;

  ret = get_mempolicy(&node, nullptr, 0, addr, MPOL_F_NODE | MPOL_F_ADDR);
  if (ret == -1) {
    fmt::print("error getting memory policy for page {:p}\n", addr);
  }
  return node;
}

} // namespace

int main() {
  const size_t size = 128 * 1024;
  const int src_numa_node = 1;
  const int dst_numa_node = 0;

  void *p = numa_alloc_onnode(size, src_numa_node);

  if (!p) {
    fmt::print("Cannot allocate from NUMA node {}!\n", src_numa_node);
    return 1;
  }

  std::memset(p, 0, size);
  check_value(p, size, 0);

  int const max_nodes = numa_num_possible_nodes();

  struct bitmask *fromnodes = numa_bitmask_alloc(max_nodes);
  struct bitmask *tonodes = numa_bitmask_alloc(max_nodes);

  fromnodes = numa_bitmask_setbit(fromnodes, src_numa_node);
  tonodes = numa_bitmask_setbit(tonodes, dst_numa_node);

  struct bitmask *allowed_nodes = numa_get_mems_allowed();
  fmt::print("Allowed nodes: size = {}, bitmask = {}\n", allowed_nodes->size,
             allowed_nodes->maskp[0]);
  fmt::print("From nodes: size = {}, bitmask = {}\n", fromnodes->size,
             fromnodes->maskp[0]);
  fmt::print("To nodes: size = {}, bitmask = {}\n", tonodes->size,
             tonodes->maskp[0]);

  // fmt::print("Press \'c\' to continue with migration\n");
  // while (getchar() != 'c') {
  //   ;
  // }

  sleep(1);
  fmt::print("Addr on node {}\n", addr_on_node(p));

  int const result = numa_migrate_pages(0, fromnodes, tonodes);

  if (result == -1) {
    fmt::print("Cannot migrate pages from node 1 to node 0, reason: {}\n",
               strerror(errno));
    numa_free(p, size);
    return 1;
  }

  std::memset(p, 1, size);
  check_value(p, size, 1);

  fmt::print("Addr on node {}\n", addr_on_node(p));

  // wait_migration_done(src_numa_node);

  std::memset(p, 2, size);
  check_value(p, size, 2);

  fmt::print("Migrated pages from node 1 to node 0.\n{} pages failed to move\n",
             result);
  numa_free(p, size);
  return 0;
}
