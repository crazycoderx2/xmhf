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

#include "kv.h"

#include <assert.h>
#include <malloc.h>
#include <string.h>

#define MAX_KEYS 100

typedef struct {
  void *key;
  size_t key_len;
  void *val;
  size_t val_len;
} kv_node;

struct kv_ctx_s {
  size_t num_keys;
  kv_node data[MAX_KEYS];
};

kv_ctx_t* kv_ctx_new(void)
{
  kv_ctx_t* rv = malloc(sizeof(kv_ctx_t));
  if (!rv) {
    return NULL;
  }
  rv->num_keys = 0;
  return rv;
}

void kv_ctx_del(kv_ctx_t* ctx)
{
  free(ctx);
}

int kv_add(kv_ctx_t* ctx, const void *key, size_t key_len, const void *val, size_t val_len)
{
  assert(ctx && key && val);

  if(kv_get(ctx, key, key_len, NULL, NULL) != KV_ENOTFOUND) {
    return KV_EEXISTS;
  }
  if(ctx->num_keys >= MAX_KEYS) {
    return KV_EFULL;
  }
  ctx->data[ctx->num_keys] = (kv_node) {
      .key = malloc(key_len),
      .key_len = key_len,
      .val = malloc(val_len),
      .val_len = val_len,
  };
  if(!ctx->data[ctx->num_keys].key || !ctx->data[ctx->num_keys].val) {
    free(ctx->data[ctx->num_keys].key);
    free(ctx->data[ctx->num_keys].val);
    return KV_ENOMEM;
  }
  memcpy(ctx->data[ctx->num_keys].key, key, key_len);
  memcpy(ctx->data[ctx->num_keys].val, val, val_len);
  ctx->num_keys++;
  return KV_ENONE;
}

int kv_get(kv_ctx_t* ctx, const void *key, size_t key_len, const void **val, size_t *val_len)
{
  size_t i;
  assert(ctx && key);

  for(i=0; i< ctx->num_keys; i++) {
    if(key_len == ctx->data[i].key_len
       && !memcmp(key, ctx->data[i].key, key_len)) {
      if (val)
        *val = ctx->data[i].val;
      if (val_len)
        *val_len = ctx->data[i].val_len;
      return KV_ENONE;
    }
  }
  return KV_ENOTFOUND;
}
