/**
 * Memory Management Unit
 *
 * MARSS-RISCV : Micro-Architectural System Simulator for RISC-V
 *
 * Copyright (c) 2017-2019 Gaurav Kothari {gkothar1@binghamton.edu}
 * State University of New York at Binghamton
 *
 * Copyright (c) 2018-2019 Parikshit Sarnaik {psarnai1@binghamton.edu}
 * State University of New York at Binghamton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _MMU_H_
#define _MMU_H_

#include "sim_params_stats.h"
#include "cache.h"
#include "memory_controller.h"
#include "riscv_sim_typedefs.h"

typedef struct MemoryManagementUnit
{
    MemoryController *mem_controller;
    Cache *icache;
    Cache *dcache;
    Cache *l2_cache;
    int caches_enabled;
} MMU;


MMU *mmu_init(const SimParams *p);
MMU *dw_mmu_init(const SimParams *p);



int mmu_insn_read(MMU *mmu, target_ulong paddr, int bytes_to_read,
                  int stage_id, int priv);
int mmu_data_read(MMU *mmu, target_ulong paddr, int bytes_to_read,
                  int stage_id, int priv);
int mmu_data_write(MMU *mmu, target_ulong paddr, int bytes_to_write,
                   int stage_id, int priv);
int mmu_pte_read(MMU *mmu, target_ulong paddr, int bytes_to_read, int stage_id, int priv);
int mmu_pte_write(MMU *mmu, target_ulong paddr, int bytes_to_write,
                  int stage_id, int priv);
void mmu_free(MMU **mmu);



#endif
