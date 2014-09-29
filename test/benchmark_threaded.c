#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include "hash.h"
#include "hash_pthread.h"

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
HASH_PTHREAD_GENERATE(hnode, hh);


typedef unsigned long utime_t;

HASH_HEAD(, hnode, hh) head;
int n, r, k, i, j, th, tht, ret;
float p;
struct hnode *a, *b;
pthread_t *w;
unsigned int position;
utime_t t;


utime_t utime()
{
  struct timeval tv;

  (void) gettimeofday(&tv, NULL);
  return (utime_t) tv.tv_sec * 1000000 + (utime_t) tv.tv_usec;
}

void* lookup(void *unused)
{
  int i;
  struct hnode *pos;
  utime_t t1, t2;
  unsigned long long sum;

  sum = 0;

  t1 = utime();
  for (i = 0; i < r; i ++) {
    pos = HASH_FIND(&head, hnode, hh, &b[i].key);
    if (pos != NULL)
      sum += pos->value;
  }
  t2 = utime();

  __atomic_fetch_add(&t, t2 - t1, __ATOMIC_RELAXED);

  return NULL;
}

void randomize_input(struct hnode *a, int n, struct hnode *b, int r, float p)
{
  int i, hit;

  for (i = 0; i < n; i ++)
    a[i].key = rand();
  for (i = 0; i < r; i ++) {
    hit = ((float) rand() / (float) RAND_MAX) <= p;
    b[i].key = hit ? a[rand() % n].key : rand();
  }
}

void usage(void)
{
  extern char *__progname;

  fprintf(stderr, "usage: %s <size> <requests> <measurements> <hit probability> <threads>\n", __progname);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  if (argc != 6)
    usage();
 
  n = strtol(argv[1], NULL, 10);
  r = strtol(argv[2], NULL, 10);
  k = strtol(argv[3], NULL, 10);
  p = strtof(argv[4], NULL);
  th = strtol(argv[5], NULL, 10);

  a = malloc(n * sizeof *a);
  b = malloc(r * sizeof *b);
  w = calloc(th, sizeof(pthread_t));

  t = 0;
  HASH_INIT(&head, hnode, hh);
  HASH_INIT_PTREAD_MUTEX(&head, hnode, hh);

  for (j = 0; j < k; j ++) {
    randomize_input(a, n, b, r, p);

    for (i = 0; i < n; i ++) {
      a[i].value = i;
      HASH_INSERT(&head, hnode, hh, &a[i]);
    }

    for (tht = 0; tht < th; tht ++) {
      pthread_create(&w[tht], NULL, lookup, NULL);
    }
    for (tht = 0; tht < th; tht ++) {
      pthread_join(w[tht], NULL);
    }
    HASH_CLEANUP_NODES(&head, hnode, hh, NULL);
  }
 
  (void) fprintf(stdout, "%.2f MOPS\n", (double) r * k * th / (double) t);

  return EXIT_SUCCESS;
}

