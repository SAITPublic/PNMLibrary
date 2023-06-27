/*
 * Copyright (C) 2023 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */
#ifndef __LIBIMDB_H__
#define __LIBIMDB_H__

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h> // NOLINT(modernize-deprecated-headers)
#endif

// The path to IMDB device file that allows to mmap CSRs
#define IMDB_CSR_DEVICE "/dev/cxl/imdb0"
#define IMDB_CSR_DEVICE_SIM_NAME "/imdb-control"
#define IMDB_CSR_DEVICE_SIM "/dev/shm" IMDB_CSR_DEVICE_SIM_NAME

//  IMDB registers region size
#define REG_AREA_SIZE 0x1900UL

// For POC only, not used
// The IMDB memory space mapped to BAR4 of PCIe interface.
// Access to control registers performs by mapping /dev/mem with offset defined
// by BAR4 Should be read from device/os/etc. Predefined value is:
#define BAR4_OFFSET 0x203EFFF70000UL

// For POC only, not used
// The IMDB memory physical address  (CXL_MEM_OFFSET) is mapped at 66GB.
// The access to memory storage performed by DAX via mapping the
// IMDB_DEVMEMORY_PATH file. In PoC CXL_MEM_OFFSET is equal to:
#define CXL_MEM_OFFSET 0x1080000000UL

#define MAX_THREADS 3

// For POC only, not used
// The memory for DB is allocated in DAX mapped file from the DAX-file start to
// DB_MAX_OFFSET value (in PoC)
#define DB_MAX_OFFSET 0x400000000

// offset`s for Common CSR`s
#define COMMON_CSR_OFFSET 0UL
#define THREAD_STATUS_OFFSET 0x440UL

#define THREAD_CSR_SIZE 0x400UL
#define THREAD_CSR_BASE_OFFSET 0x1000UL
// offset for Thread`s Column Scan CSR`s
#define THREAD_1_CSR_OFFSET (THREAD_CSR_BASE_OFFSET)
#define THREAD_2_CSR_OFFSET (THREAD_CSR_BASE_OFFSET + THREAD_CSR_SIZE)
#define THREAD_3_CSR_OFFSET (THREAD_CSR_BASE_OFFSET + THREAD_CSR_SIZE * 2)

// Parts of RESET csr
#define IMDB_RESET_ENABLE 0
#define IMDB_RESET_DISABLE 0b1U

// Parts of CTRL csr
#define THREAD_CTRL_STATUS_MASK 0b01U
#define THREAD_CTRL_OUTPUT_TYPE_MASK 0b10U

#define THREAD_CTRL_START 0b1U
#define THREAD_CTRL_BV 0
#define THREAD_CTRL_IV 0b10U

#define THREAD_STATUS_BUSY 0b10U

// Parts of COMP_MODE csr
#define THREAD_COMP_MODE_ENTRY_BIT_SIZE_MASK ((1U << 5) - 1)
#define THREAD_COMP_MODE_OPERATION_TYPE_MASK (1U << 8)

#define THREAD_COMP_MODE_IN_RANGE 0
#define THREAD_COMP_MODE_IN_LIST (1U << 8)

#define THREAD_MIN_OPCODE_VALUE 0b100U
#define THREAD_MAX_OPCODE_VALUE 0b010U

#define IMDB_DEVMEMORY_PATH "/dev/dax0.0"
#define IMDB_DEVMEMORY_PATH_SIM_NAME "/imdb-data"
#define IMDB_DEVMEMORY_PATH_SIM "/dev/shm" IMDB_DEVMEMORY_PATH_SIM_NAME

#pragma pack(push, 1)

/** common CSR`s **/

// located at BAR4_OFFSET + COMMON_CSR_OFFSET
struct CommonCSR {
  // [TODO: @a.korzun] create mask/offset definitions for IMDB_ID and IMDB_CAP.

  // Represent device type, info about threads count and FPGA version
  // bits 0:3 (Minor version) - 1 = Initial, 2 = ER2 release, 3 = CA release
  // bits 4:7 (Major version) - 0 = IMDB 2 Threads, 1 = IMDB 3 threads
  // bits 8:11 (HW Accel type) - 0 = Column scan, 1 = Aggregation,
  //                             2 = Aggr + Column scan, 3 = Undefined
  /** 0x0000 -  IMDB Identification Register **/
  uint32_t IMDB_ID;

  // Represent number of supported scan threads, cores and memory capacity
  // bits 0:15 - DRAM memory capacity in GBs
  // bits 16:17 - Number of cores
  // bits 18:19 - Threads number
  /** 0x0004 -  IMDB Capability register **/
  uint32_t IMDB_CAP;

  // Host has to Enable and Disable soft reset. No self clearing by HW.
  // bit 0 - 0 = Enable soft-reset, 1 = Disable soft reset
  /** 0x0008 - IMDB Reset Register **/
  uint32_t IMDB_RESET;
};

// located at BAR4_OFFSET + THREAD_STATUS_OFFSET
struct StatusCSR {

  // Three bits that represent scan operation status for 3 engines.
  // for each of bit 0:2 - 0 = Operation is in process, 1 = Operation is done
  /** 0x0440 - IMDB Common Status Register **/
  uint32_t IMDB_COM_STATUS;
};

/** thread CSR`s **/

// located at BAR4_OFFSET + THREAD_N_CSR_OFFSET
struct ThreadCSR {

  // bit 0 - start flag
  // bit 1 - output_type, 0=BitVector, 1=IndexVector
  /** 0x00 Column Scan Control Register **/
  uint32_t THREAD_CTRL;

  // bits 0:1 - Internal done, set 0b10 after IMDB operation
  /** 0x04 Column Scan Status Register **/
  uint32_t THREAD_STATUS;

  /** 0x08 Column Scan Error Register **/
  uint32_t THREAD_ERROR;

  // bits 0-4 - entry bit size
  // bits 8-9 - 0 = InRange, 1 = InList, >1 = undefined
  /** 0x0C Set scan type and compression mode **/
  uint32_t THREAD_COMP_MODE;

  /** 0x10 Column Scan Start Address  - 64bit **/
  uint64_t THREAD_START_ADDR;

  // should be aligned by 4KB
  uint64_t alignment_1;

  /** 0x20 Number of elements for the column scan operation - 64bit **/
  uint64_t THREAD_SIZE_SB;

  // should be aligned by 4KB
  uint64_t alignment_2;

  /** 0x30 The lover bound (min) (InRange) **/
  uint32_t THREAD_MIN_VALUE;

  /** 0x34 The upper bound (max) (InRange) **/
  uint32_t THREAD_MAX_VALUE;

  // should be aligned by 4KB
  uint64_t alignment_3;

  /** 0x40 Operation at MIN point **/
  uint32_t THREAD_MIN_OPCODE;

  /** 0x44 Operation at MAX point **/
  uint32_t THREAD_MAX_OPCODE;

  // should be aligned by 4KB
  uint64_t alignment_4;

  /** 0x50 Column scan result address - 64bits **/
  uint64_t THREAD_RES_ADDR;

  // NOTE: Not using this - always =0

  /** 0x58 Column scan result index offset (Inlist) - 64bits **/
  uint64_t THREAD_RES_INDEX_OFFSET;

  /** 0x60 Number of elements on the result column (Inlist) - 64bits **/
  uint64_t THREAD_RES_SIZE_SB;

  // should be aligned by 4KB
  uint64_t alignment_5;

  /** 0x70 Record Tick value for column scan starting event **/
  uint32_t THREAD_START_TIME;

  /** 0x74 Record Tick value for column scan completion event **/
  uint32_t THREAD_DONE_TIME;

  // should be aligned by 4KB
  uint64_t alignment_6;

  /** 0x?080 Address of predictor BitVector (Inlist) - 64bits **/
  uint64_t THREAD_INLIST_ADDR;

  /** 0x88 Size of the bit-vector (Inlist) **/
  uint32_t THREAD_INLIST_SIZE;

  // NOTE: Not using this - always =0

  /** 0x8C The index of the column from which to start **/
  uint32_t THREAD_COLUMN_OFFSET;

  /** 0x90 The max number of output indices (IndexVector) **/
  uint32_t THREAD_RSLT_LMT;

  // end at 0x098
};

#pragma pack(pop)

#endif /* __LIBIMDB_H__ */
