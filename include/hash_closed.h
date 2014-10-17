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


#define _HASH_USE_CLOSED 1
/* No need to store anything in the value itself */
#define HASH_ENTRY(type)                                                       \
struct {                                                                       \
  HASH_TYPE hv;                                                                \
  uint32_t flags;                                                              \
}

#define HASH_HEAD(name, type, field)                                           \
struct name {                                                                  \
    struct _hash_ops_##type##_##field *ops;                                    \
    struct type *nodes;                                                        \
    int n_buckets, size, n_occupied, upper_bound;                              \
}

/* Basic ops */
#define _HASH_NODE_EMPTY(node, field) ((node)->field.flags & 0x1)
#define _HASH_NODE_ERASE(node, field) ((node)->field.flags |= 0x1)
#define _HASH_NODE_FILL(node, field) ((node)->field.flags |= ~0x1)

/*
 * We use quadratic probe here:
 * http://en.wikipedia.org/wiki/Quadratic_probing
 */
#define HASH_INSERT(head, type, field, elm) do {                               \
  HASH_TYPE _hv;                                                               \
  struct type *_h;                                                             \
  _hv = (head)->ops->hash_func((elm), (head)->ops->hashd);                     \
  (elm)->field.hv = _hv;                                                       \
  HASH_FIND_BKT(head, field, _hv, _h);                                         \
  memcpy(_h, elm, sizeof(*_h));                                                \
  _HASH_NODE_FILL(_h, field);                                                  \
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

/*
 * Find hash value in the nodes using quadratic probing with sign alteration
 * The size of hash table *MUST* be prime for this operation
 */
#define HASH_FIND_BKT(head, field, hv, bkt) do {                               \
  HASH_TYPE _idx, _step = 0, _mask, _limit;                                    \
  _mask = (head)->n_buckets;                                                   \
  _limit = _mask;                                                              \
  _idx = _hv % _mask;                                                          \
  _h = &(head)->nodes[_idx];                                                   \
  if (!_HASH_NODE_EMPTY(_h)) {                                                 \
    /* We need to find the place for inserting */                              \
    for(;;) {                                                                  \
    	++_step;                                                                 \
      if (_HASH_NODE_EMPTY(_h)) {                                              \
        break;                                                                 \
      }                                                                        \
      else if (_h->field.hv == _hv) {                                          \
        /* Need to compare */                                                  \
        if ((head)->ops->hash_cmp((elm), _h, (head)->ops->hashd) == 0) {       \
          /* XXX: maybe free value here */                                     \
          break;                                                               \
        }                                                                      \
      }                                                                        \
      else if (_step == _limit) {                                              \
        /* XXX: hash table is full */                                          \
        assert(0);                                                             \
      }                                                                        \
      _idx = _idx & 0x1 ? ((_hv - _step * _step) % mask) :                     \
           ((_hv + _step * _step) % mask);                                     \
      _h = &(head)->nodes[_idx];                                               \
    }                                                                          \
  }                                                                            \
}

#endif /* HASH_CLOSED_H_ */
