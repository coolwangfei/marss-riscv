/**
 * Memory Controller
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "circular_queue.h"
#include "memory_controller.h"
#include "dramsim_wrapper_c_connector.h"

MemoryController *
mem_controller_init(const SimParams *p)
{
    MemoryController *m;

    m = (MemoryController *)calloc(1, sizeof(MemoryController));
    assert(m);
    m->dram_burst_size = p->dram_burst_size;
    m->mem_model_type = p->mem_model_type;
    m->mem_access_latency = p->mem_access_latency;
    m->pte_rw_latency = p->pte_rw_latency;

    m->frontend_mem_access_queue.max_size = FRONTEND_MEM_ACCESS_QUEUE_SIZE;
    m->frontend_mem_access_queue.entry = (PendingMemAccessEntry *)calloc(
        m->frontend_mem_access_queue.max_size, sizeof(PendingMemAccessEntry));
    assert(m->frontend_mem_access_queue.entry);

    m->backend_mem_access_queue.max_size = BACKEND_MEM_ACCESS_QUEUE_SIZE;
    m->backend_mem_access_queue.entry = (PendingMemAccessEntry *)calloc(
        m->backend_mem_access_queue.max_size, sizeof(PendingMemAccessEntry));
    assert(m->backend_mem_access_queue.entry);

    cq_init(&m->mem_request_queue.cq, MEM_REQUEST_QUEUE_SIZE);
    memset((void *)m->mem_request_queue.entry, 0,
           sizeof(PendingMemAccessEntry) * MEM_REQUEST_QUEUE_SIZE);

    PRINT_INIT_MSG("Setting up dram");

    switch (m->mem_model_type)
    {
        case MEM_MODEL_BASE:
        {
            PRINT_INIT_MSG("Setting up base dram model");
            m->mem_controller_update_internal = &mem_controller_update_base;
            mem_controller_set_dram_burst_size(m, p->dram_burst_size);
            break;
        }
        case MEM_MODEL_DRAMSIM:
        {
            PRINT_INIT_MSG("Setting up DRAMSim2");
            dramsim_wrapper_init(
                p->dramsim_ini_file, p->dramsim_system_ini_file,
                p->dramsim_stats_dir, p->core_name, p->guest_ram_size,
                &m->frontend_mem_access_queue, &m->backend_mem_access_queue);

            m->mem_controller_update_internal = &mem_controller_update_dramsim;
            mem_controller_set_dram_burst_size(m, dramsim_get_burst_size());

            /* Check if the cache line size equals DRAMSim burst size */
            if ((p->words_per_cache_line * sizeof(target_ulong))
                != dramsim_get_burst_size())
            {
                fprintf(stderr, "error: cache line size and dramsim burst size "
                                "not equal\n");
                exit(1);
            }
            break;
        }
        default:
        {
            fprintf(stderr, "error: invalid memory model\n");
            exit(1);
        }
    }

    return m;
}

void
mem_controller_free(MemoryController **m)
{
    switch ((*m)->mem_model_type)
    {
        case MEM_MODEL_BASE:
        {
            break;
        }
        case MEM_MODEL_DRAMSIM:
        {
            dramsim_wrapper_destroy();
            break;
        }
        default:
        {
            fprintf(stderr, "error: invalid memory model\n");
            exit(1);
        }
    }
    free((*m)->backend_mem_access_queue.entry);
    (*m)->backend_mem_access_queue.entry = NULL;
    free((*m)->frontend_mem_access_queue.entry);
    (*m)->frontend_mem_access_queue.entry = NULL;
    free(*m);
    *m = NULL;
}

void
mem_controller_reset(MemoryController *m)
{
    m->current_latency = 0;
    m->max_latency = 0;
    m->mem_access_active = 0;
    m->last_accessed_page_num = 0;

    mem_controller_flush_stage_mem_access_queue(&m->frontend_mem_access_queue);
    mem_controller_flush_stage_mem_access_queue(&m->backend_mem_access_queue);

    cq_reset(&m->mem_request_queue.cq);
}

void
mem_controller_set_dram_burst_size(MemoryController *m, int dram_burst_size)
{
    m->dram_burst_size = dram_burst_size;
}

int
mem_controller_access_dram(MemoryController *m, target_ulong paddr, int bytes_to_access,
           MemAccessType type, void *p_mem_access_info)
{
    int index, stage;
    target_ulong start_offset;

    /* Get stage id which has generated this access, 0 for fetch, 1 for memory */
    stage = *(int *)p_mem_access_info;

    /*  Align the address for this access to the dram_burst_size */
    start_offset = paddr % m->dram_burst_size;
    if (0 != start_offset)
    {
        bytes_to_access += start_offset;
        paddr -= start_offset;
    }

    /* Read/Write data dram_burst_size bytes at a time */
    while (bytes_to_access > 0)
    {
        /* Add transaction for this access to either of the front-end or back-end
         * queue */
        if (stage == FETCH)
        {
            m->frontend_mem_access_queue.entry[m->frontend_mem_access_queue.cur_idx].addr = paddr;
            m->frontend_mem_access_queue.entry[m->frontend_mem_access_queue.cur_idx].type = type;
            m->frontend_mem_access_queue.entry[m->frontend_mem_access_queue.cur_idx].valid = 1;
            ++m->frontend_mem_access_queue.cur_idx;
            ++m->frontend_mem_access_queue.cur_size;
        }
        else if (stage == MEMORY)
        {
            m->backend_mem_access_queue.entry[m->backend_mem_access_queue.cur_idx].addr = paddr;
            m->backend_mem_access_queue.entry[m->backend_mem_access_queue.cur_idx].type = type;
            m->backend_mem_access_queue.entry[m->backend_mem_access_queue.cur_idx].valid = 1;
            ++m->backend_mem_access_queue.cur_idx;
            ++m->backend_mem_access_queue.cur_size;
        }
        else
        {
            /* Only fetch and memory stage generate memory access */
            assert(0);
        }

        /* Add transaction for this access to dram dispatch queue */
        index = cq_enqueue(&m->mem_request_queue.cq);

        assert(index != -1);
        m->mem_request_queue.entry[index].addr = paddr;
        m->mem_request_queue.entry[index].type = type;
        m->mem_request_queue.entry[index].bytes_to_access = m->dram_burst_size;
        m->mem_request_queue.entry[index].valid = 1;
        m->mem_request_queue.entry[index].req_pte = 0;

        /* Calculate remaining transactions for this access */
        bytes_to_access -= m->dram_burst_size;
        paddr += m->dram_burst_size;
    }

    return 0;
}

int
mem_controller_add_pte_to_dram_queue(MemoryController *m, target_ulong paddr, int bytes_to_access,
           MemAccessType type, void *p_mem_access_info)
{
    int index, stage;

    /* Get stage id which has generated this access, 0 for fetch, 1 for memory */
    stage = *(int *)p_mem_access_info;

    /* Add transaction for this access to either of the front-end or back-end
     * queue */
    if (stage == FETCH)
    {
        m->frontend_mem_access_queue.entry[m->frontend_mem_access_queue.cur_idx].addr = paddr;
        m->frontend_mem_access_queue.entry[m->frontend_mem_access_queue.cur_idx].type = type;
        m->frontend_mem_access_queue.entry[m->frontend_mem_access_queue.cur_idx].valid = 1;
        ++m->frontend_mem_access_queue.cur_idx;
        ++m->frontend_mem_access_queue.cur_size;
    }
    else if (stage == MEMORY)
    {
        m->backend_mem_access_queue.entry[m->backend_mem_access_queue.cur_idx].addr = paddr;
        m->backend_mem_access_queue.entry[m->backend_mem_access_queue.cur_idx].type = type;
        m->backend_mem_access_queue.entry[m->backend_mem_access_queue.cur_idx].valid = 1;
        ++m->backend_mem_access_queue.cur_idx;
        ++m->backend_mem_access_queue.cur_size;
    }
    else
    {
        /* Only fetch and memory stage generate memory access */
        assert(0);
    }

    /* Add transaction for this access to dram dispatch queue */
    index = cq_enqueue(&m->mem_request_queue.cq);

    assert(index != -1);
    m->mem_request_queue.entry[index].addr = paddr;
    m->mem_request_queue.entry[index].type = type;
    m->mem_request_queue.entry[index].bytes_to_access = m->dram_burst_size;
    m->mem_request_queue.entry[index].valid = 1;
    m->mem_request_queue.entry[index].req_pte = 1;

    return 0;
}

/* Read callback used by base DRAM model */
static void
read_complete(MemoryController *m, target_ulong addr)
{
    int i;

    for (i = 0; i < m->frontend_mem_access_queue.cur_idx; ++i)
    {
        if ((m->frontend_mem_access_queue.entry[i].valid)
            && (m->frontend_mem_access_queue.entry[i].addr == addr)
            && (m->frontend_mem_access_queue.entry[i].type == Read))
        {
            m->frontend_mem_access_queue.entry[i].valid = 0;
            --m->frontend_mem_access_queue.cur_size;
            return;
        }
    }

    for (i = 0; i < m->backend_mem_access_queue.cur_idx; ++i)
    {
        if ((m->backend_mem_access_queue.entry[i].valid)
            && (m->backend_mem_access_queue.entry[i].addr == addr)
            && (m->backend_mem_access_queue.entry[i].type == Read))
        {
            m->backend_mem_access_queue.entry[i].valid = 0;
            --m->backend_mem_access_queue.cur_size;
            return;
        }
    }
}

/* Write callback used by base DRAM model */
static void
write_complete(MemoryController *m, target_ulong addr)
{
    int i;

    for (i = 0; i < m->frontend_mem_access_queue.cur_idx; ++i)
    {
        if ((m->frontend_mem_access_queue.entry[i].valid)
            && (m->frontend_mem_access_queue.entry[i].addr == addr)
            && (m->frontend_mem_access_queue.entry[i].type == Write))
        {
            m->frontend_mem_access_queue.entry[i].valid = 0;
            --m->frontend_mem_access_queue.cur_size;
            return;
        }
    }

    for (i = 0; i < m->backend_mem_access_queue.cur_idx; ++i)
    {
        if ((m->backend_mem_access_queue.entry[i].valid)
            && (m->backend_mem_access_queue.entry[i].addr == addr)
            && (m->backend_mem_access_queue.entry[i].type == Write))
        {
            m->backend_mem_access_queue.entry[i].valid = 0;
            --m->backend_mem_access_queue.cur_size;
            return;
        }
    }
}

void
mem_controller_update_base(MemoryController *m)
{
    uint64_t current_page_num;
    PendingMemAccessEntry *e;

    if (!m->mem_access_active)
    {
        if (!cq_empty(&m->mem_request_queue.cq))
        {
            e = &m->mem_request_queue
                     .entry[cq_front(&m->mem_request_queue.cq)];

            if (e->req_pte)
            {
                /* This access is related to reading/writing page table entry */
                m->max_latency = m->pte_rw_latency;
            }
            else
            {
                m->max_latency = m->mem_access_latency;
            }

            /* Remove page offset to get current page number, page size is always 4KB */
            current_page_num = e->addr >> 12;

            if (m->last_accessed_page_num == current_page_num)
            {
                /* Page hit */
                m->max_latency = m->max_latency * 0.6;
            }
            else
            {
                /* Page miss */
                m->last_accessed_page_num = current_page_num;
            }

            /* Send a write complete callback to the calling pipeline stage as
             * we don't want the pipeline stage to wait for write to complete.
             * But, simulate this write delay asynchronously via the memory
             * controller */
            if (e->type == Write)
            {
                write_complete(m, e->addr);
            }

            m->mem_access_active = 1;
            m->current_latency = 1;
        }
    }

    if (m->mem_access_active)
    {
        e = &m->mem_request_queue.entry[cq_front(&m->mem_request_queue.cq)];

        if (m->current_latency == m->max_latency)
        {
            m->mem_access_active = 0;
            m->max_latency = 0;
            m->current_latency = 0;

            if (e->type == Read)
            {
                read_complete(m, e->addr);
            }

            /* Remove the entry */
            e->valid = 0;
            cq_dequeue(&m->mem_request_queue.cq);
        }
        else
        {
            m->current_latency++;
        }
    }
}

void
mem_controller_update_dramsim(MemoryController *m)
{
    PendingMemAccessEntry *e;

    while (!cq_empty(&m->mem_request_queue.cq))
    {
        e = &m->mem_request_queue.entry[cq_front(&m->mem_request_queue.cq)];
        if (dramsim_wrapper_can_add_transaction(e->addr))
        {
            dramsim_wrapper_add_transaction(e->addr, e->type);
            cq_dequeue(&m->mem_request_queue.cq);
        }
        else
        {
            break;
        }
    }

    dramsim_wrapper_update();
}

void
mem_controller_flush_stage_mem_access_queue(StageMemAccessQueue *q)
{
    q->cur_idx = 0;
    q->cur_size = 0;
}

void
mem_controller_flush_dram_queue(MemoryController *m)
{
    cq_reset(&m->mem_request_queue.cq);
}