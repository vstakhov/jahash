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

#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct hnode {
  int key;
  int value;
  HASH_ENTRY(hnode) hh;
};

HASH_GENERATE_INT(hnode, hh, key);

HASH_HEAD(, hnode, hh) head;

int
test_sum(struct hnode *node, int *sum)
{
  *sum += node->value;

  return 1;
}

int
test_filter(struct hnode *node, void *unused)
{
  return node->value % 2 == 0;
}

void
fake_free(struct hnode *node, void *unused)
{}

int
main(int argc, char **argv)
{
  struct hnode nodes[32];
  int sum = 0;

  HASH_INIT(&head, hnode, hh);

  for (int i = 0; i < sizeof(nodes) / sizeof(nodes[0]); i ++) {
    nodes[i].key = i;
    nodes[i].value = i + 1;
    HASH_INSERT(&head, hnode, hh, &nodes[i]);
  }

  HASH_ITERATE_FUNC(&head, hnode, hh, test_sum, &sum);

  assert(sum == 528);

  /* Now filter odd elements */
  HASH_FILTER_FUNC(&head, hnode, hh, test_filter, fake_free, NULL);

  sum = 0;
  HASH_ITERATE_FUNC(&head, hnode, hh, test_sum, &sum);

  assert(sum == 272);

  return 0;
}
