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

struct hnode {
  const char *key;
  int cookie;
  HASH_ENTRY(hnode) hh;
};

HASH_GENERATE_STR(hnode, hh, key);

HASH_HEAD(, hnode, hh) head;

int
main(int argc, char **argv)
{
  struct hnode *node;
  char buf[BUFSIZ], *value, *key;
  HASH_ITER(hnode, hh) it = HASH_ITER_INITIALIZER;

  HASH_INIT(&head, hnode, hh);

  while(fgets(buf, sizeof (buf) - 1, stdin)) {
    value = buf;
    key = strsep(&value, " ");
    node = malloc(sizeof(*node));
    node->key = strdup(key);
    node->cookie = atoi(value);
    HASH_INSERT(&head, hnode, hh, node);
  }

  HASH_ITERATE(&head, hnode, hh, it, node) {
    if (node != NULL) {
      printf("%s: %d\n", node->key, node->cookie);
    }
  }

  return 0;
}
