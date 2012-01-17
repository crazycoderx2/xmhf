/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * This file is part of the EMHF historical reference
 * codebase, and is released under the terms of the
 * GNU General Public License (GPL) version 2.
 * Please see the LICENSE file for details.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

/* hpt.h - hypervisor page table abstraction
 * 
 * author - Jim Newsome (jnewsome@no-fuss.com)
 *
 * note: the HPT abstraction requires certain target interfaces to
 * be defined by a "provider". EMHF specific target interface
 * definitions live in hpt_emhf.h
 */

#ifndef HPT_H
#define HPT_H

//#include "_types.h"
#include <bitfield.h>
#include <stddef.h>
#include <assert.h>

#ifndef hpt_log_trace
# define hpt_log_trace(fmt, args...) while(0)
#endif

typedef enum {
  HPT_TYPE_NORM=0, /* x86 'normal'\legacy */
  HPT_TYPE_PAE=1, /* Physical address extension */
  HPT_TYPE_LONG=2, /* AMD Long Mode */
  HPT_TYPE_EPT=3, /* Intel Extended Page Tables */

  HPT_TYPE_INVALID=4, /* same number as below intentionally; saves memory in static arrays */
  HPT_TYPE_NUM=4 /* dummy entry. must be last */
} hpt_type_t;

#define HPT_MAX_LEVEL 4

#define HPT_LVL_PT1 1
#define HPT_LVL_PD2 2
#define HPT_LVL_PDP3 3
#define HPT_LVL_PDPT3 3
#define HPT_LVL_PML4 4

typedef u64 hpt_pme_t; /* page map entry (any level) */
typedef hpt_pme_t* hpt_pm_t; /* page map (any level) */

/* NOTE- do not dereference directly.
   Some page table types use 32-bit entries instead
   of 64-bit. */

typedef u64 hpt_prot_t; /* pme protection flags type */
typedef u64 hpt_va_t; /* virtual address type */
typedef u64 hpt_pa_t; /* physical address type */

typedef struct {
  hpt_pm_t pm;
  hpt_type_t t;
  int lvl;
} hpt_pmo_t;

typedef struct {
  hpt_pme_t pme;
  hpt_type_t t;
  int lvl;
} hpt_pmeo_t;

typedef hpt_pa_t (*hpt_ptr2pa_t)(void *ctx, void *ptr); /* translate a referencable pointer to a physical address */
typedef void* (*hpt_pa2ptr_t)(void *ctx, hpt_pa_t pa); /* translate a physical address to a referenceable pointer */
typedef void* (*hpt_get_zeroed_page_t)(void *ctx, size_t alignment, size_t sz);
                       
typedef struct {
  hpt_type_t t;
  hpt_get_zeroed_page_t gzp;
  void *gzp_ctx;
  hpt_pa2ptr_t pa2ptr;
  void *pa2ptr_ctx;
  hpt_ptr2pa_t ptr2pa;
  void *ptr2pa_ctx;
} hpt_walk_ctx_t;

#define HPT_UNUSED_ARGUMENT(x) (void)x

#define HPT_PM_SIZE 4096

/* page map sizes, in bytes. note that zero index is invalid. */
extern const u16 hpt_pm_sizes[HPT_TYPE_NUM][HPT_MAX_LEVEL+1];

/* page map sizes, in bytes. note that zero index is invalid. */
extern const u16 hpt_pm_alignments[HPT_TYPE_NUM][HPT_MAX_LEVEL+1];

/* high bit of va used to index into the page map of the given level.
 * we treat level '0' specially here, so that the low bit of the index
 * can consistently be found by looking up the entry for 'level-1'.
 */
extern const u8 hpt_va_idx_hi[HPT_TYPE_NUM][HPT_MAX_LEVEL+1];

extern const u8 hpt_type_max_lvl[HPT_TYPE_NUM];

size_t hpt_pm_size(hpt_type_t t, int lvl);

#include <hpt_consts.h> /* XXX - remove this once fn bodies are moved
                           into hpt.c */

/* Abstract protection flags. */
#define HPT_PROT_READ_BIT 0
#define HPT_PROT_WRITE_BIT 1
#define HPT_PROT_EXEC_BIT 2
#define HPT_PROT_READ_MASK (1ull<<HPT_PROT_READ_BIT)
#define HPT_PROT_WRITE_MASK (1ull<<HPT_PROT_WRITE_BIT)
#define HPT_PROT_EXEC_MASK (1ull<<HPT_PROT_EXEC_BIT)

#define HPT_PROTS_RWX (HPT_PROT_READ_MASK|HPT_PROT_WRITE_MASK|HPT_PROT_EXEC_MASK)
#define HPT_PROTS_RW (HPT_PROT_READ_MASK|HPT_PROT_WRITE_MASK)
#define HPT_PROTS_RX (HPT_PROT_READ_MASK|HPT_PROT_EXEC_MASK)
#define HPT_PROTS_R (HPT_PROT_READ_MASK)
#define HPT_PROTS_WX (HPT_PROT_WRITE_MASK|HPT_PROT_EXEC_MASK) /* never valid */
#define HPT_PROTS_W (HPT_PROT_WRITE_MASK) /* never valid */
#define HPT_PROTS_X (HPT_PROT_EXEC_MASK) /* never valid */
#define HPT_PROTS_NONE (0)

static inline u64 hpt_cr3_set_address(hpt_type_t t, u64 cr3, hpt_pa_t a)
{
  if (t==HPT_TYPE_NORM) {
    cr3 = BR64_COPY_BITS_HL(cr3, a, HPT_CR3_PML2_NORM_HI, HPT_CR3_PML2_NORM_LO, 0);
    cr3 = BR64_COPY_BITS_HL(cr3, 0, HPT_CR3_MBZ11_NORM_HI, HPT_CR3_MBZ11_NORM_LO, 0);
    cr3 = BR64_COPY_BITS_HL(cr3, 0, HPT_CR3_MBZ2_HI, HPT_CR3_MBZ2_LO, 0);
  } else if (t==HPT_TYPE_PAE) {
    cr3 = BR64_COPY_BITS_HL(cr3, a, HPT_CR3_PML3_PAE_HI, HPT_CR3_PML3_PAE_LO, 0);
    cr3 = BR64_COPY_BITS_HL(cr3, 0, HPT_CR3_MBZ2_HI, HPT_CR3_MBZ2_LO, 0);
  } else if (t==HPT_TYPE_LONG) {
    cr3 = BR64_COPY_BITS_HL(cr3, a, HPT_CR3_PML4_LONG_HI, HPT_CR3_PML4_LONG_LO, 0);
    cr3 = BR64_COPY_BITS_HL(cr3, 0, HPT_CR3_MBZ2_HI, HPT_CR3_MBZ2_LO, 0);
  } else if (t==HPT_TYPE_EPT) {
    assert(0); /* N\A. set EPTP ptr */
  } else {
    assert(0);
  }
  return cr3;
}

static inline hpt_pa_t hpt_cr3_get_address(hpt_type_t t, u64 cr3)
{
  if (t==HPT_TYPE_NORM) {
    return BR64_COPY_BITS_HL(0, cr3, HPT_CR3_PML2_NORM_HI, HPT_CR3_PML2_NORM_LO, 0);
  } else if (t==HPT_TYPE_PAE) {
    return BR64_COPY_BITS_HL(0, cr3, HPT_CR3_PML3_PAE_HI, HPT_CR3_PML3_PAE_LO, 0);
  } else if (t==HPT_TYPE_LONG) {
    return BR64_COPY_BITS_HL(0, cr3, HPT_CR3_PML4_LONG_HI, HPT_CR3_PML4_LONG_LO, 0);
  } else if (t==HPT_TYPE_EPT) {
    assert(0); /* N\A. set EPTP ptr */
  } else {
    assert(0);
  }
  assert(0); return (hpt_pa_t)0; /* unreachable; appeases compiler */
}

static inline u64 hpt_eptp_set_address(hpt_type_t t, u64 eptp, hpt_pa_t a)
{
  assert(t == HPT_TYPE_EPT);
  return BR64_COPY_BITS_HL(eptp, a, HPT_EPTP_PML4_HI, HPT_EPTP_PML4_LO, 0);
}

static inline hpt_pa_t hpt_eptp_get_address(hpt_type_t t, u64 eptp)
{
  assert(t == HPT_TYPE_EPT);
  return BR64_COPY_BITS_HL(0, eptp, HPT_EPTP_PML4_HI, HPT_EPTP_PML4_LO, 0);
}

#define MAX(x, y) (((y) > (x)) ? (y) : (x))
#define MIN(x, y) (((y) < (x)) ? (y) : (x))

typedef enum {
  HPT_PMT_UC=0, HPT_PMT_WC=1, HPT_PMT_WT=4, HPT_PMT_WP=5, HPT_PMT_WB=6
} hpt_pmt_t;

/* assumes lvl is valid for the given type */
static inline bool hpt_prot_is_valid(hpt_type_t t, int lvl, hpt_prot_t perms)
{
  /* consider making this a table lookup. however, if perms is passed
     a compile-time constant, the compiler should constant fold this
     whole function to the corresponding constant output. */
  return
    (t == HPT_TYPE_NORM)
    ? (perms == HPT_PROTS_NONE
       || perms == HPT_PROTS_RX
       || perms == HPT_PROTS_RWX)
    : (t == HPT_TYPE_PAE)
    ? ((perms == HPT_PROTS_NONE
        || perms == HPT_PROTS_RWX
        || (((lvl != HPT_LVL_PDPT3) &&
            (perms == HPT_PROTS_R))
            || (perms == HPT_PROTS_RX)
            || (perms == HPT_PROTS_RW))))
    : (t == HPT_TYPE_LONG)
    ? (perms == HPT_PROTS_NONE
       || perms == HPT_PROTS_R
       || perms == HPT_PROTS_RX
       || perms == HPT_PROTS_RW
       || perms == HPT_PROTS_RWX)
    : (t == HPT_TYPE_EPT)
    ? (true)
    : (false);
}

static inline bool hpt_lvl_is_valid(hpt_type_t t, int lvl)
{
  return lvl <= hpt_type_max_lvl[t];
}

static inline bool hpt_type_is_valid(hpt_type_t t)
{
  return t < HPT_TYPE_NUM;
}

static inline int hpt_root_lvl(hpt_type_t t)
{
  assert(hpt_type_is_valid(t));
  return hpt_type_max_lvl[t];
}

static inline hpt_pme_t hpt_pme_setuser(hpt_type_t t, int lvl, hpt_pme_t entry, bool user_accessible)
{
  if (t == HPT_TYPE_NORM) {
    return BR64_SET_BIT(entry, HPT_NORM_US_L21_MP_BIT, user_accessible);
  } else if (t == HPT_TYPE_PAE) {
    if (lvl == 3) {
      assert(user_accessible);
      return entry;
    } else {
      return BR64_SET_BIT(entry, HPT_PAE_US_L21_MP_BIT, user_accessible);
    }
  } else if (t == HPT_TYPE_LONG) {
    return BR64_SET_BIT(entry, HPT_LONG_US_L4321_MP_BIT, user_accessible);
  } else if (t == HPT_TYPE_EPT) {
    assert(user_accessible);
    return entry;
  }
  assert(0); return 0; /* unreachable; appeases compiler */
}

static inline bool hpt_pme_getuser(hpt_type_t t, int lvl, hpt_pme_t entry)
{
  if (t == HPT_TYPE_NORM) {
    return BR64_GET_BIT(entry, HPT_NORM_US_L21_MP_BIT);
  } else if (t == HPT_TYPE_PAE) {
    if (lvl == 3) {
      return true;
    } else {
      return BR64_GET_BIT(entry, HPT_PAE_US_L21_MP_BIT);
    }
  } else if (t == HPT_TYPE_LONG) {
    return BR64_GET_BIT(entry, HPT_LONG_US_L4321_MP_BIT);
  } else if (t == HPT_TYPE_EPT) {
    return true;
  }
  assert(0); return false; /* unreachable; appeases compiler */
}

static inline hpt_pme_t hpt_pme_setprot(hpt_type_t t, int lvl, hpt_pme_t entry, hpt_prot_t perms)
{
  hpt_pme_t rv=entry;
  assert(hpt_lvl_is_valid(t, lvl));
  assert(hpt_prot_is_valid(t, lvl, perms));

  if (t == HPT_TYPE_NORM) {
    rv = BR64_SET_BIT(rv, HPT_NORM_P_L21_MP_BIT, perms & HPT_PROT_READ_MASK);
    rv = BR64_SET_BIT(rv, HPT_NORM_RW_L21_MP_BIT, perms & HPT_PROT_WRITE_MASK);
  } else if (t == HPT_TYPE_PAE) {
    rv = BR64_SET_BIT(rv, HPT_PAE_P_L321_MP_BIT, perms & HPT_PROT_READ_MASK);
    if (lvl == 2 || lvl == 1) {
      rv = BR64_SET_BIT(rv, HPT_PAE_RW_L21_MP_BIT, perms & HPT_PROT_WRITE_MASK);
      rv = BR64_SET_BIT(rv, HPT_PAE_NX_L21_MP_BIT, !(perms & HPT_PROT_EXEC_MASK));
    }
  } else if (t == HPT_TYPE_LONG) {
    rv = BR64_SET_BIT(rv, HPT_LONG_P_L4321_MP_BIT, perms & HPT_PROT_READ_MASK);
    rv = BR64_SET_BIT(rv, HPT_LONG_RW_L4321_MP_BIT, perms & HPT_PROT_WRITE_MASK);
    rv = BR64_SET_BIT(rv, HPT_LONG_NX_L4321_MP_BIT, !(perms & HPT_PROT_EXEC_MASK));
  } else if (t == HPT_TYPE_EPT) {
    rv = BR64_SET_BR(rv, HPT_EPT_PROT_L4321_MP, perms);
  } else {
    assert(0);
  }

  return rv;
}

static inline hpt_prot_t hpt_pme_getprot(hpt_type_t t, int lvl, hpt_pme_t entry)
{
  hpt_prot_t rv=HPT_PROTS_NONE;
  bool r,w,x;
  assert(hpt_lvl_is_valid(t, lvl));

  if (t == HPT_TYPE_NORM) {
    r= entry & MASKBIT64(HPT_NORM_P_L21_MP_BIT);
    w= entry & MASKBIT64(HPT_NORM_RW_L21_MP_BIT);
    x= r;
  } else if (t == HPT_TYPE_PAE) {
    r= entry & MASKBIT64(HPT_PAE_P_L321_MP_BIT);
    if (lvl == 2 || lvl == 1) {
      w= entry & MASKBIT64(HPT_PAE_RW_L21_MP_BIT);
      x= !(entry & MASKBIT64(HPT_PAE_NX_L21_MP_BIT));;
    } else {
      w=r;
      x=r;
    }
  } else if (t == HPT_TYPE_LONG) {
    r=entry & MASKBIT64(HPT_LONG_P_L4321_MP_BIT);
    w=entry & MASKBIT64(HPT_LONG_RW_L4321_MP_BIT);
    x=!(entry & MASKBIT64(HPT_LONG_NX_L4321_MP_BIT));
  } else if (t == HPT_TYPE_EPT) {
    r=entry & MASKBIT64(HPT_EPT_R_L4321_MP_BIT);
    w=entry & MASKBIT64(HPT_EPT_W_L4321_MP_BIT);
    x=entry & MASKBIT64(HPT_EPT_X_L4321_MP_BIT);
  } else {
    assert(0);
  }
  rv = HPT_PROTS_NONE;
  rv = rv | (r ? HPT_PROT_READ_MASK : 0);
  rv = rv | (w ? HPT_PROT_WRITE_MASK : 0);
  rv = rv | (x ? HPT_PROT_EXEC_MASK : 0);

  return rv;
}

//XXX: TODO we need to get rid of these equates. leave them in here
//for the time being until we get a concrete interface list for
//emhf-memprot component
//#define hpt_pme_setprot emhf_memprot_pagemapentry_setprot
//#define hpt_pme_getprot emhf_memprot_pagemapentry_getprot


static inline hpt_pme_t hpt_pme_setunused(hpt_type_t t, int lvl, hpt_pme_t entry, int hi, int lo, hpt_pme_t val)
{
  hpt_pme_t rv=entry;
  assert(hi>lo);
  HPT_UNUSED_ARGUMENT(lvl);

  /* bits 9, 10, and 11 are unused in all levels of all current page
     types. For convenience and simplicity we map those to\from bits 2-0.

     If we need more unused bits, for some page table types and levels other
     bits are available. 
  */
     
  if(t == HPT_TYPE_NORM
     || t == HPT_TYPE_PAE
     || t == HPT_TYPE_LONG
     || t == HPT_TYPE_EPT) {
    assert(hi <= 2); /* we can remove this limitation for some table
                        types and levels if necessary. */
    rv = BR64_COPY_BITS_HL(rv, val, MIN(2,hi), MAX(0,lo), (9-0));
  } else {
    assert(0);
  }
  return rv;
}

static inline hpt_pme_t hpt_pme_getunused(hpt_type_t t, int lvl, hpt_pme_t entry, int hi, int lo)
{
  hpt_pme_t rv = 0ull;
  assert(hi>lo);
  HPT_UNUSED_ARGUMENT(lvl);

  if(t == HPT_TYPE_NORM
     || t == HPT_TYPE_PAE
     || t == HPT_TYPE_LONG
     || t == HPT_TYPE_EPT) {
    assert(hi <= 2); /* we can remove this limitation for some table
                        types and levels if necessary. */
    rv = BR64_COPY_BITS_HL(rv, entry, MIN(2,hi), MAX(0,lo), (9-0));
    rv = BR64_GET_HL(entry, MIN(2,hi)+9, MAX(0,lo)+9);
  } else {
    assert(0);
  }
  return rv;
}

static inline bool hpt_pme_is_present(hpt_type_t t, int lvl, hpt_pme_t entry)
{
  /* a valid entry is present iff read access is enabled. */
  return hpt_pme_getprot(t, lvl, entry) & HPT_PROT_READ_MASK;
}

static inline bool hpt_pme_is_page(hpt_type_t t, int lvl, hpt_pme_t entry)
{
  if (t== HPT_TYPE_NORM) {
    assert(lvl<=2);
    return lvl == 1 || (lvl==2 && BR64_GET_BIT(entry, HPT_NORM_PS_L2_MP_BIT));
  } else if (t == HPT_TYPE_PAE) {
    assert(lvl<=3);
    return lvl == 1 || (lvl==2 && BR64_GET_BIT(entry, HPT_PAE_PS_L2_MP_BIT));
  } else if (t == HPT_TYPE_LONG) {
    assert(lvl<=3);
    return lvl == 1 || ((lvl==2 || lvl==3) && BR64_GET_BIT(entry, HPT_LONG_PS_L32_MP_BIT));
  } else if (t == HPT_TYPE_EPT) {
    assert(lvl<=4);
    return lvl == 1 || ((lvl==2 || lvl==3) && BR64_GET_BIT(entry, HPT_EPT_PS_L32_MP_BIT));
  } else {
    assert(0);
    return false;
  }
}

static inline hpt_pa_t hpt_pme_get_address(hpt_type_t t, int lvl, hpt_pme_t entry)
{
  if (t == HPT_TYPE_NORM) {
    assert(lvl<=2);
    if(lvl==2) {
      if (hpt_pme_is_page(t,lvl,entry)) {
        /* 4 MB page */
        hpt_pa_t rv = 0;
        rv = BR64_COPY_BITS_HL(rv, entry,
                               39, 32,
                               32-HPT_NORM_ADDR3932_L2_P_LO);
        rv = BR64_COPY_BITS_HL(rv, entry,
                               31, 22,
                               22-HPT_NORM_ADDR3122_L2_P_LO);
        return rv;
      } else {
        return BR64_COPY_BITS_HL(0, entry,
                                 HPT_NORM_ADDR_L2_M_HI,
                                 HPT_NORM_ADDR_L2_M_LO, 0);
      }
    } else {
      return BR64_COPY_BITS_HL(0, entry,
                               HPT_NORM_ADDR_L1_P_HI,
                               HPT_NORM_ADDR_L1_P_LO, 0);
    }
  } else if (t == HPT_TYPE_PAE) {
    assert(lvl<=3);
    if (hpt_pme_is_page(t, lvl, entry)) {
      if (lvl == 1) {
        return BR64_COPY_BITS_HL(0, entry,
                                 HPT_PAE_ADDR_L1_P_HI,
                                 HPT_PAE_ADDR_L1_P_LO, 0);
      } else {
        assert(lvl==2);
        return BR64_COPY_BITS_HL(0, entry,
                                 HPT_PAE_ADDR_L2_P_HI,
                                 HPT_PAE_ADDR_L2_P_LO, 0);
      }
    } else {
      return BR64_COPY_BITS_HL(0, entry,
                               HPT_PAE_ADDR_L321_M_HI,
                               HPT_PAE_ADDR_L321_M_LO, 0);
    }
  } else if (t == HPT_TYPE_LONG) {
    assert(lvl<=4);
    if (hpt_pme_is_page(t, lvl, entry)) {
      if(lvl==1) {
        return BR64_COPY_BITS_HL(0, entry,
                                 HPT_LONG_ADDR_L1_P_HI,
                                 HPT_LONG_ADDR_L1_P_LO, 0);
      } else {
        return BR64_COPY_BITS_HL(0, entry,
                                 HPT_LONG_ADDR_L32_P_HI,
                                 HPT_LONG_ADDR_L32_P_LO, 0);
      }
    } else {
      return BR64_COPY_BITS_HL(0, entry,
                               HPT_LONG_ADDR_L4321_M_HI,
                               HPT_LONG_ADDR_L4321_M_LO, 0);
    }
  } else if (t == HPT_TYPE_EPT) {
    assert(lvl<=4);
    return BR64_COPY_BITS_HL(0, entry,
                             HPT_EPT_ADDR_L4321_MP_HI,
                             HPT_EPT_ADDR_L4321_MP_LO, 0);
  } else {
    assert(0);
    return 0;
  }
}

static inline hpt_pme_t hpt_pme_set_address(hpt_type_t t, int lvl, hpt_pme_t entry, hpt_pa_t addr)
{
  if (t == HPT_TYPE_NORM) {
    assert(lvl<=2);
    if(lvl==2) {
      if (hpt_pme_is_page(t,lvl,entry)) {
        hpt_pme_t rv = entry;
        /* 4 MB page */
        rv = BR64_COPY_BITS_HL(entry, addr,
                               HPT_NORM_ADDR3932_L2_P_HI,
                               HPT_NORM_ADDR3932_L2_P_LO,
                               HPT_NORM_ADDR3932_L2_P_LO-32);
        rv = BR64_COPY_BITS_HL(entry, addr,
                               HPT_NORM_ADDR3122_L2_P_HI,
                               HPT_NORM_ADDR3122_L2_P_LO,
                               HPT_NORM_ADDR3122_L2_P_LO-22);
        return rv;
      } else {
        return BR64_COPY_BITS_HL(entry, addr,
                                 HPT_NORM_ADDR_L2_M_HI,
                                 HPT_NORM_ADDR_L2_M_LO, 0);
      }
    } else {
      return BR64_COPY_BITS_HL(entry, addr,
                               HPT_NORM_ADDR_L1_P_HI,
                               HPT_NORM_ADDR_L1_P_LO, 0);
    }
  } else if (t == HPT_TYPE_PAE) {
    assert(lvl<=3);
    if (hpt_pme_is_page(t, lvl, entry)) {
      if (lvl == 1) {
        return BR64_COPY_BITS_HL(entry, addr,
                                 HPT_PAE_ADDR_L1_P_HI,
                                 HPT_PAE_ADDR_L1_P_LO, 0);
      } else {
        assert(lvl==2);
        return BR64_COPY_BITS_HL(entry, addr,
                                 HPT_PAE_ADDR_L2_P_HI,
                                 HPT_PAE_ADDR_L2_P_LO, 0);
      }
    } else {
      return BR64_COPY_BITS_HL(entry, addr,
                               HPT_PAE_ADDR_L321_M_HI,
                               HPT_PAE_ADDR_L321_M_LO, 0);
    }
  } else if (t == HPT_TYPE_LONG) {
    assert(lvl<=4);
    if (hpt_pme_is_page(t, lvl, entry)) {
      if(lvl==1) {
        return BR64_COPY_BITS_HL(entry, addr,
                                 HPT_LONG_ADDR_L1_P_HI,
                                 HPT_LONG_ADDR_L1_P_LO, 0);
      } else {
        return BR64_COPY_BITS_HL(entry, addr,
                                 HPT_LONG_ADDR_L32_P_HI,
                                 HPT_LONG_ADDR_L32_P_LO, 0);
      }
    } else {
      return BR64_COPY_BITS_HL(entry, addr,
                               HPT_LONG_ADDR_L4321_M_HI,
                               HPT_LONG_ADDR_L4321_M_LO, 0);
    }
  } else if (t == HPT_TYPE_EPT) {
    assert(lvl<=4);
    return BR64_COPY_BITS_HL(entry, addr,
                             HPT_EPT_ADDR_L4321_MP_HI,
                             HPT_EPT_ADDR_L4321_MP_LO, 0);
  } else {
    assert(0);
    return 0;
  }
}

/* "internal". use hpt_pme_set_pmt instead */
static inline hpt_pme_t hpt_pme_set_pat(hpt_type_t t, int lvl, hpt_pme_t pme, bool pat)
{
  hpt_pme_t rv;
  if (t == HPT_TYPE_NORM) {
    rv = pme;
    if (hpt_pme_is_page(t, lvl, pme) && lvl==1) {
      rv = BR64_SET_BIT(rv, HPT_NORM_PAT_L1_P_BIT, pat);
    } else if (hpt_pme_is_page(t, lvl, pme) && lvl==2) {
      rv = BR64_SET_BIT(rv, HPT_NORM_PAT_L2_P_BIT, pat);
    } else {
      assert(!pat);
    }
  } else if (t== HPT_TYPE_PAE) {
    rv = pme;
    if (hpt_pme_is_page(t, lvl, pme) && lvl==1) {
      rv = BR64_SET_BIT(rv, HPT_PAE_PAT_L1_P_BIT, pat);
    } else if (hpt_pme_is_page(t, lvl, pme) && lvl==2) {
      rv = BR64_SET_BIT(rv, HPT_PAE_PAT_L2_P_BIT, pat);
    } else {
      assert(!pat);
    }
  } else if (t==HPT_TYPE_LONG) {
    rv = pme;
    if (hpt_pme_is_page(t, lvl, pme) && lvl==1) {
      rv = BR64_SET_BIT(rv, HPT_LONG_PAT_L1_P_BIT, pat);
    } else if (hpt_pme_is_page(t, lvl, pme) && (lvl==2||lvl==3)) {
      rv = BR64_SET_BIT(rv, HPT_LONG_PAT_L32_P_BIT, pat);
    } else {
      assert(!pat);
    }
  } else {
    assert(0);
  }
  return rv;
}

/* "internal". use hpt_pme_get_pmt instead */
static inline bool hpt_pme_get_pat(hpt_type_t t, int lvl, hpt_pme_t pme)
{
  if (t == HPT_TYPE_NORM) {
    if (hpt_pme_is_page(t, lvl, pme) && lvl==1) {
      return BR64_GET_BIT(pme, HPT_NORM_PAT_L1_P_BIT);
    } else if (hpt_pme_is_page(t, lvl, pme) && lvl==2) {
      return BR64_GET_BIT(pme, HPT_NORM_PAT_L2_P_BIT);
    } else {
      return false;
    }
  } else if (t== HPT_TYPE_PAE) {
    if (hpt_pme_is_page(t, lvl, pme) && lvl==1) {
      return BR64_GET_BIT(pme, HPT_PAE_PAT_L1_P_BIT);
    } else if (hpt_pme_is_page(t, lvl, pme) && lvl==2) {
      return BR64_GET_BIT(pme, HPT_PAE_PAT_L2_P_BIT);
    } else {
      return false;
    }
  } else if (t==HPT_TYPE_LONG) {
    if (hpt_pme_is_page(t, lvl, pme) && lvl==1) {
      return BR64_GET_BIT(pme, HPT_LONG_PAT_L1_P_BIT);
    } else if (hpt_pme_is_page(t, lvl, pme) && (lvl==2||lvl==3)) {
      return BR64_GET_BIT(pme, HPT_LONG_PAT_L32_P_BIT);
    } else {
      return false;
    }
  } else {
    assert(0);
  }
  return pme;
}

/* "internal". use hpt_pme_get_pmt instead */
static inline bool hpt_pme_get_pcd(hpt_type_t t, int __attribute__((unused)) lvl, hpt_pme_t pme)
{
  if (t == HPT_TYPE_NORM) {
    return BR64_GET_BIT(pme, HPT_NORM_PCD_L21_MP_BIT);
  } else if (t== HPT_TYPE_PAE) {
    return BR64_GET_BIT(pme, HPT_PAE_PCD_L321_MP_BIT);
  } else if (t==HPT_TYPE_LONG) {
    return  BR64_GET_BIT(pme, HPT_LONG_PCD_L4321_MP_BIT);
  } else {
    assert(0);
  }
  assert(0); return false; /* unreachable; appeases compiler */  
}

/* "internal". use hpt_pme_set_pmt instead */
static inline hpt_pme_t hpt_pme_set_pcd(hpt_type_t t, int __attribute__((unused)) lvl, hpt_pme_t pme, bool pcd)
{
  if (t == HPT_TYPE_NORM) {
    return BR64_SET_BIT(pme, HPT_NORM_PCD_L21_MP_BIT, pcd);
  } else if (t== HPT_TYPE_PAE) {
    return BR64_SET_BIT(pme, HPT_PAE_PCD_L321_MP_BIT, pcd);
  } else if (t==HPT_TYPE_LONG) {
    return  BR64_SET_BIT(pme, HPT_LONG_PCD_L4321_MP_BIT, pcd);
  } else {
    assert(0);
  }
  assert(0); return (hpt_pme_t)0; /* unreachable; appeases compiler */  
}

/* "internal". use hpt_pme_get_pmt instead */
static inline bool hpt_pme_get_pwt(hpt_type_t t, int __attribute__((unused)) lvl, hpt_pme_t pme)
{
  if (t == HPT_TYPE_NORM) {
    return BR64_GET_BIT(pme, HPT_NORM_PWT_L21_MP_BIT);
  } else if (t== HPT_TYPE_PAE) {
    return BR64_GET_BIT(pme, HPT_PAE_PWT_L321_MP_BIT);
  } else if (t==HPT_TYPE_LONG) {
    return  BR64_GET_BIT(pme, HPT_LONG_PWT_L4321_MP_BIT);
  } else {
    assert(0);
  }
  assert(0); return false; /* unreachable; appeases compiler */  
}

/* "internal". use hpt_pme_set_pmt instead */
static inline hpt_pme_t hpt_pme_set_pwt(hpt_type_t t, int __attribute__((unused)) lvl, hpt_pme_t pme, bool pwt)
{
  if (t == HPT_TYPE_NORM) {
    return BR64_SET_BIT(pme, HPT_NORM_PWT_L21_MP_BIT, pwt);
  } else if (t== HPT_TYPE_PAE) {
    return BR64_SET_BIT(pme, HPT_PAE_PWT_L321_MP_BIT, pwt);
  } else if (t==HPT_TYPE_LONG) {
    return  BR64_SET_BIT(pme, HPT_LONG_PWT_L4321_MP_BIT, pwt);
  } else {
    assert(0);
  }
  assert(0); return (hpt_pme_t)0; /* unreachable; appeases compiler */  
}

/* Assumes PAT register has default values */
static inline hpt_pmt_t hpt_pme_get_pmt(hpt_type_t t, int lvl, hpt_pme_t pme)
{
  hpt_pmt_t rv;
  if (t == HPT_TYPE_EPT) {
    assert(lvl <= 3 && hpt_pme_is_page(t, lvl, pme));
    rv = BR64_GET_HL(pme, HPT_EPT_MT_L321_P_HI, HPT_EPT_MT_L321_P_LO);
  } else if (t == HPT_TYPE_PAE || t == HPT_TYPE_LONG || t == HPT_TYPE_NORM) {
    bool pcd, pwt;
    pcd = hpt_pme_get_pcd(t, lvl, pme);
    pwt = hpt_pme_get_pwt(t, lvl, pme);
    if (!pcd && !pwt) {
      return HPT_PMT_WB;
    } else if (!pcd && pwt) {
      return HPT_PMT_WT;
    } else if (pcd && !pwt) {
      return HPT_PMT_WC; /* really UC- unless overriden by mtrr */
    } else if (pcd && pwt) {
      return HPT_PMT_UC;
    }
  } else {
    assert(0);
  }
  return rv;
}

/* Always clears PAT bit when applicable. */
static inline hpt_pmt_t hpt_pme_set_pmt(hpt_type_t t, int lvl, hpt_pme_t pme, hpt_pmt_t pmt)
{
  hpt_pme_t rv;
  if (t == HPT_TYPE_EPT) {
    assert(lvl <= 3 && hpt_pme_is_page(t, lvl, pme));
    rv = BR64_SET_HL(pme, HPT_EPT_MT_L321_P_HI, HPT_EPT_MT_L321_P_LO, pmt);
  } else if (t == HPT_TYPE_NORM || t == HPT_TYPE_PAE || t == HPT_TYPE_LONG) {
    bool pat, pcd, pwt;
    pat=0;
    if (pmt == HPT_PMT_UC) {
      pcd=1;
      pwt=1;
    } else if (pmt == HPT_PMT_WC) {
      pcd=1;
      pwt=0; /* this is actually 'UC-'. can be overriden to WC by setting MTRR */
    } else if (pmt == HPT_PMT_WT) {
      pcd=0;
      pwt=1;
    } else if (pmt == HPT_PMT_WP) {
      assert(0); /* can only get this by manipulating PAT register */
    } else if (pmt == HPT_PMT_WB) {
      pcd=0;
      pwt=0;
    } else {
      assert(0);
    }
    rv = pme;
    rv = hpt_pme_set_pat(t, lvl, rv, pat);
    rv = hpt_pme_set_pcd(t, lvl, rv, pcd);
    rv = hpt_pme_set_pwt(t, lvl, rv, pwt);
  } else {
    assert(0);
  }
  return rv;
}

static inline
unsigned int hpt_get_pm_idx(hpt_type_t t, int lvl, hpt_va_t va)
{
  unsigned int lo;
  unsigned int hi;
  assert(hpt_type_is_valid(t));
  assert(hpt_lvl_is_valid(t, lvl));

  hi = hpt_va_idx_hi[t][lvl];
  lo = hpt_va_idx_hi[t][lvl-1]+1;

  return BR64_GET_HL(va, hi, lo);
}

static inline
hpt_pme_t hpt_pm_get_pme_by_idx(hpt_type_t t, int lvl, hpt_pm_t pm, int idx)
{
  HPT_UNUSED_ARGUMENT(lvl);
  if(t == HPT_TYPE_EPT || t == HPT_TYPE_PAE || t == HPT_TYPE_LONG) {
    return ((u64*)pm)[idx];
  } else if (t == HPT_TYPE_NORM) {
    return ((u32*)pm)[idx];
  } else {
    assert(0);
    return 0;
  }
}

static inline
void hpt_pm_set_pme_by_idx(hpt_type_t t, int lvl, hpt_pm_t pm, int idx, hpt_pme_t pme)
{
  HPT_UNUSED_ARGUMENT(lvl);
  if(t == HPT_TYPE_EPT || t == HPT_TYPE_PAE || t == HPT_TYPE_LONG) {
    ((u64*)pm)[idx] = pme;
  } else if (t == HPT_TYPE_NORM) {
    ((u32*)pm)[idx] = pme;
  } else {
    assert(0);
  }
}

static inline
hpt_pme_t hpt_pm_get_pme_by_va(hpt_type_t t, int lvl, hpt_pm_t pm, hpt_va_t va)
{
  return hpt_pm_get_pme_by_idx(t, lvl, pm, hpt_get_pm_idx(t, lvl, va));
}

static inline
void hpt_pm_set_pme_by_va(hpt_type_t t, int lvl, hpt_pm_t pm, hpt_va_t va, hpt_pme_t pme)
{
  hpt_pm_set_pme_by_idx(t, lvl, pm, hpt_get_pm_idx(t, lvl, va), pme);
}

/* attempt to descend one level. on success, lvl and pm are set
   accordingly, and true is returned. on failure, lvl and pm are
   untouched and false is returned. */
static inline
bool hpt_walk_next_lvl(const hpt_walk_ctx_t *ctx, int *lvl, hpt_pm_t *pm, hpt_va_t va)
{
  hpt_pme_t pme = hpt_pm_get_pme_by_va(ctx->t, *lvl, *pm, va);
  if (!hpt_pme_is_present(ctx->t, *lvl, pme)
      || hpt_pme_is_page(ctx->t, *lvl, pme)) {
    return false;
  } else {
    *pm = ctx->pa2ptr(ctx->pa2ptr_ctx, hpt_pme_get_address(ctx->t, *lvl, pme));
    (*lvl)--;
    return true;
  }
}

/* returns the lowest-level page map containing va, down to
 * end_lvl. end_lvl is set to the level of the returned page map.
 */
static inline
hpt_pm_t hpt_walk_get_pm(const hpt_walk_ctx_t *ctx, int lvl, hpt_pm_t pm, int *end_lvl, hpt_va_t va)
{
  assert(lvl >= *end_lvl);

  while(lvl > *end_lvl) {
    if (!hpt_walk_next_lvl(ctx, &lvl, &pm, va)) {
      *end_lvl = lvl;
      return pm;
    }
  }
  return pm;
}

/* returns the lowest-level page map _entry_ containing va, down to
 * end_lvl. end_lvl is set to the level of the returned page map
 * containing the returned entry.
 */
static inline
hpt_pme_t hpt_walk_get_pme(const hpt_walk_ctx_t *ctx, int lvl, hpt_pm_t pm, int *end_lvl, hpt_va_t va)
{
  pm = hpt_walk_get_pm(ctx, lvl, pm, end_lvl, va);
  return hpt_pm_get_pme_by_va(ctx->t, *end_lvl, pm, va);
}

/* returns the page map of level end_lvl containing va, allocating
   maps if necessary. Note that the end_lvl may be a higher level than requested
   if the address is mapped via a large page.
*/
static inline
hpt_pm_t hpt_walk_get_pm_alloc(const hpt_walk_ctx_t *ctx, int lvl, hpt_pm_t pm, int *end_lvl, hpt_va_t va)
{
  assert(lvl >= *end_lvl);
  while(lvl > *end_lvl) {
    hpt_pme_t pme = hpt_pm_get_pme_by_va(ctx->t, lvl, pm, va);

    hpt_log_trace("hpt_walk_get_pm_alloc: lvl:%d pm:%x end_lvl:%d va:%Lx\n",
                  lvl, (u32)pm, *end_lvl, va);
    hpt_log_trace("hpt_walk_get_pm_alloc: pme:%Lx\n",
            pme);
    if (hpt_pme_is_page(ctx->t, lvl, pme)) {
      *end_lvl = lvl;
      return pm;
    }
    if (!hpt_pme_is_present(ctx->t, lvl, pme)) {
      hpt_pm_t new_pm = ctx->gzp(ctx->gzp_ctx,
                                 HPT_PM_SIZE/*FIXME*/,
                                 hpt_pm_size(ctx->t, lvl-1));
      hpt_log_trace("hpt_walk_get_pm_alloc: allocated pm at hva:%x spa:%Lx\n",
              (u32)new_pm, ctx->ptr2pa(ctx->ptr2pa_ctx, new_pm));
      if(!new_pm) {
        return NULL;
      }
      pme = hpt_pme_set_address(ctx->t, lvl, pme, ctx->ptr2pa(ctx->ptr2pa_ctx, new_pm));
      pme = hpt_pme_setprot(ctx->t, lvl, pme, HPT_PROTS_RWX);
      pme = hpt_pme_setuser(ctx->t, lvl, pme, true);
      hpt_pm_set_pme_by_va(ctx->t, lvl, pm, va, pme);
      hpt_log_trace("hpt_walk_get_pm_alloc: inserted pme:%Lx\n", pme);
    }
    assert(hpt_walk_next_lvl(ctx, &lvl, &pm, va));
  }
  assert(lvl==*end_lvl);
  return pm;
}

/* inserts pme into the page map of level tgt_lvl containing va, allocating
 * maps if necessary. returns 0 on success, other on failure.
 * Will fail if one of the intermediate entries is a large page
 */
static inline
int hpt_walk_insert_pme_alloc(const hpt_walk_ctx_t *ctx, int lvl, hpt_pm_t pm, int tgt_lvl, hpt_va_t va, hpt_pme_t pme)
{
  int end_lvl=tgt_lvl;
  hpt_log_trace("hpt_walk_insert_pme_alloc: lvl:%d pm:%x tgt_lvl:%d va:%Lx pme:%Lx\n",
          lvl, (u32)pm, tgt_lvl, va, pme);
  pm = hpt_walk_get_pm_alloc(ctx, lvl, pm, &end_lvl, va);
  hpt_log_trace("hpt_walk_insert_pme_alloc: got pm:%x end_lvl:%d\n",
          (u32)pm, end_lvl);

  if(pm == NULL || tgt_lvl != end_lvl) {
    return 1;
  }
  hpt_pm_set_pme_by_va(ctx->t, tgt_lvl, pm, va, pme);
  return 0;
}

#endif
