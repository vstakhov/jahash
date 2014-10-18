#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include "hash.h"

struct hnode {
  int key;
  int value;
  HASH_ENTRY(hnode) hh;
};

static HASH_TYPE hf(const struct hnode *n, void *d)
{
	return n->key;	
}

static int cmpf(const struct hnode *n1, const struct hnode *n2, void *d)
{
	return n1->key - n2->key;	
}

HASH_GENERATE_OPS(hnode, hh, key, hf, cmpf, NULL);

HASH_HEAD(, hnode, hh) head;

typedef unsigned long utime_t;

utime_t utime()
{
  struct timeval tv;

  (void) gettimeofday(&tv, NULL);
  return (utime_t) tv.tv_sec * 1000000 + (utime_t) tv.tv_usec;
}

utime_t lookup(struct hnode *b, int r)
{
  int i;
  struct hnode *pos;
  utime_t t1, t2;
  unsigned long long sum;
 
  sum = 0;

  t1 = utime();
  for (i = 0; i < r; i ++)
    {
	  pos = HASH_FIND(&head, hnode, hh, &b[i].key);
      if (pos != NULL)
        sum += pos->value;
    }
  t2 = utime();

  return t2 - t1;
}

void randomize_input(struct hnode *a, int n, struct hnode *b, int r, float p)
{
  int i, hit;

  for (i = 0; i < n; i ++)
    a[i].key = rand();
  for (i = 0; i < r; i ++)
    {
      hit = ((float) rand() / (float) RAND_MAX) <= p;
      b[i].key = hit ? a[rand() % n].key : rand();
    }
}

void usage()
{
  extern char *__progname;

  fprintf(stderr, "usage: %s <size> <requests> <measurements> <hit probability>\n", __progname);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  int n, r, k, i, j, ret;
  float p;
  struct hnode *a, *b;
  unsigned int position;
  utime_t t;

  if (argc != 5)
    usage();
 
  srand(time(NULL));
  n = strtol(argv[1], NULL, 0);
  r = strtol(argv[2], NULL, 0);
  k = strtol(argv[3], NULL, 0);
  p = strtof(argv[4], NULL);

  a = malloc(n * sizeof *a);
  b = malloc(r * sizeof *b);

  t = 0;
  HASH_INIT(&head, hnode, hh);
  for (j = 0; j < k; j ++)
    {
      randomize_input(a, n, b, r, p);

      for (i = 0; i < n; i ++)
        {
		  a[i].value = i;
          HASH_INSERT(&head, hnode, hh, &a[i]);
        }

      t += lookup(b, r);
	  HASH_CLEANUP_NODES(&head, hnode, hh, NULL);
    }
 
  (void) fprintf(stdout, "%.2f MOPS\n", (double) r * k / (double) t);

  return EXIT_SUCCESS;
}

