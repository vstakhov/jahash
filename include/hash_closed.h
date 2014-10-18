/* Copyright (c) 2014, Vsevolod Stakhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef HASH_CLOSED_H_
#define HASH_CLOSED_H_

#include <assert.h>

#define _HASH_USE_CLOSED 1
#define _HASH_UPPER_BOUND 0.77
/* No need to store anything in the value itself */
#define HASH_ENTRY(type)                                                       \
struct {                                                                       \
  HASH_TYPE hv;                                                                \
  uint32_t flags;                                                              \
}

#define HASH_HEAD(name, type, field)                                           \
struct name {                                                                  \
   struct _hash_ops_##type##_##field *ops;                                     \
   struct type *nodes;                                                         \
   unsigned n_buckets, n_occupied, upper_bound;                                \
   unsigned need_expand;                                                       \
   unsigned generation;                                                        \
}

/* Basic ops */
#define _HASH_NODE_EMPTY(node, field) (((node)->field.flags & 0x1) == 0)
#define _HASH_NODE_ERASE(node, field) ((node)->field.flags &= ~0x1)
#define _HASH_NODE_FILL(node, field) ((node)->field.flags |= 0x1)

/*
 * We use quadratic probe here:
 * http://en.wikipedia.org/wiki/Quadratic_probing
 */
#define HASH_INSERT(head, type, field, elm) do {                               \
  if ((head)->nodes == NULL) HASH_MAKE_TABLE(head);                            \
  HASH_TYPE _hv;                                                               \
  struct type *_h;                                                             \
  if ((head)->n_occupied >= (head)->upper_bound) {                             \
    (head)->need_expand = 1;                                                   \
  }                                                                            \
  _hv = (head)->ops->hash_func((elm), (head)->ops->hashd);                     \
  (elm)->field.hv = _hv;                                                       \
  HASH_FIND_BKT(head, field, _hv, _h);                                         \
  memcpy(_h, elm, sizeof(*_h));                                                \
  _HASH_NODE_FILL(_h, field);                                                  \
  (head)->n_occupied ++;                                                       \
  if ((head)->need_expand == 1) {                                              \
    HASH_EXPAND_BUCKETS(head, type, field);                                    \
  }                                                                            \
} while(0)

#define HASH_FIND_ELT(head, type, field, elm, found) do {                      \
  if ((head)->nodes == NULL) (found) = NULL;                                   \
  else {                                                                       \
    HASH_TYPE _hv;                                                             \
    struct type *_h = (found);                                                 \
    _hv = (head)->ops->hash_func((elm), (head)->ops->hashd);                   \
    HASH_FIND_BKT(head, field, _hv, _h);                                       \
    if (_HASH_NODE_EMPTY(_h, field)) (found) = NULL;                           \
    else (found) = _h;                                                         \
  }                                                                            \
} while(0)

#define HASH_DELETE_ELT(head, type, field, elm) do {                           \
  if ((head)->buckets != NULL) {                                               \
    struct type *_h;                                                           \
    HASH_FIND_BKT(head, field, _hv, _h);                                       \
    if (!_HASH_NODE_EMPTY(_h, field)) {                                        \
      _HASH_NODE_ERASE(_h,field);                                              \
      (head)->n_occupied --;                                                   \
    }                                                                          \
  }                                                                            \
} while(0)

#define HASH_CLEANUP_NODES(head, type, field, free_func) do {                  \
  if ((head)->nodes != NULL) {                                                 \
      struct type *_bkt;                                                       \
      for (unsigned _i = 0; _i < (head)->n_buckets; _i ++) {                   \
        _bkt = &(head)->nodes[_i];                                             \
        if(_HASH_NODE_EMPTY(_bkt, field)) continue;                            \
        if ((free_func) != NULL) _hash_op_##type##_##field##_delete_node((free_func), _bkt); \
        _HASH_NODE_ERASE(_bkt, field);                                         \
      }                                                                        \
      (head)->n_occupied = 0;                                                  \
    }                                                                          \
} while(0)

#define HASH_DESTROY(head, type, field, free_func) do {                        \
  HASH_CLEANUP_NODES(head, type, field, free_func);                            \
  HASH_FREE_NODES((head), (head)->nodes, (head)->n_buckets);                   \
  (head)->nodes = NULL;                                                        \
  (head)->n_buckets = 0;                                                       \
  (head)->n_occupied = 0;                                                      \
  (head)->generation = 0;                                                      \
} while(0)

/*
 * Find hash value in the nodes using quadratic probing
 * The size of hash table *MUST* be power of two as c1=c2=1/2
 */
#define HASH_FIND_BKT(head, field, h, bkt) do {                                \
  unsigned _idx, _step = 0, _mask, _limit;                                     \
  _mask = (head)->n_buckets - 1;                                               \
  _limit = _mask + 1;                                                          \
  _idx = (h) & _mask;                                                          \
  _h = &(head)->nodes[_idx];                                                   \
  if (!_HASH_NODE_EMPTY(_h, field)) {                                          \
    /* We need to find the place for inserting */                              \
    for(;;) {                                                                  \
    	++_step;                                                                 \
      if (_HASH_NODE_EMPTY(_h, field)) {                                       \
        break;                                                                 \
      }                                                                        \
      else if (_h->field.hv == (h)) {                                          \
        /* Need to compare */                                                  \
        if ((head)->ops->hash_cmp((bkt), _h, (head)->ops->hashd) == 0) {       \
          /* XXX: maybe free value here */                                     \
          break;                                                               \
        }                                                                      \
      }                                                                        \
      else if (_step == _limit) {                                              \
        /* XXX: hash table is full */                                          \
        assert(0);                                                             \
      }                                                                        \
      _idx = ((h) + (_step*_step + _step) / 2) & _mask;                    \
      /*_idx = ((_idx) + _step) & _mask; */                                      \
      _h = &(head)->nodes[_idx];                                               \
    }                                                                          \
  }                                                                            \
  (bkt) = _h;                                                                  \
} while(0)

#define HASH_ALLOC_NODES(head, nodes, size) do {                               \
  if ((head)->ops->alloc) (nodes) = (head)->ops->alloc(sizeof(*(nodes)) * (size), \
      (head)->ops->allocd);                                                    \
  else (nodes) = malloc(sizeof(*(nodes)) * (size));                            \
  memset(nodes, 0, sizeof(*(nodes)) * (size));                                 \
} while(0)

#define HASH_FREE_NODES(head, nodes, size) do {                                \
  if ((head)->ops->free) (head)->ops->free(sizeof(*(nodes)) * (size), (nodes), (head)->ops->allocd); \
  else free(nodes);                                                            \
} while(0)

#define HASH_EXPAND_BUCKETS(head, type, field)                                 \
do {                                                                           \
  unsigned _saved_generation = (head)->generation;                             \
  if ((head)->generation == _saved_generation) {                               \
    struct type *old_nodes = (head)->nodes;                                    \
    unsigned _old_num = (head)->n_buckets;                                     \
    unsigned _new_num = (head)->n_buckets + 1;                                 \
    HASH_ROUNDUP32(_new_num);                                                  \
    HASH_ALLOC_NODES((head), (head)->nodes, _new_num);                         \
    if ((head)->nodes != NULL) {                                               \
    (head)->n_buckets = _new_num;                                              \
    for (unsigned _i = 0; _i < _old_num; _i ++) {                              \
      struct type *_h, *_cur;                                                  \
      _cur = &old_nodes[_i];                                                   \
      if(_HASH_NODE_EMPTY(_cur, field)) continue;                              \
      HASH_FIND_BKT(head, field, _cur->field.hv, _h);                          \
      memcpy(_h, _cur, sizeof(*_h));                                           \
    }                                                                          \
    HASH_FREE_NODES(head, old_nodes, _old_num);                                \
    (head)->generation ++;                                                     \
    (head)->upper_bound = ((head)->n_buckets * _HASH_UPPER_BOUND + 0.5);       \
    }                                                                          \
    else (head)->nodes = old_nodes;                                            \
  }                                                                            \
  (head)->need_expand = 0;                                                     \
} while(0)

#define HASH_MAKE_TABLE(head) do {                                             \
  (head)->n_buckets = HASH_INITIAL_NUM_BUCKETS;                                \
  HASH_ALLOC_NODES((head), (head)->nodes, (head)->n_buckets);                  \
} while(0)

#endif /* HASH_CLOSED_H_ */
