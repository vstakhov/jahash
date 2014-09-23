# Just Another Hash Library

## Introduction

This library is based on splendid [uthash](http://troydhanson.github.io/uthash/) library and is inspired by its design principles.
However, jahash tries to follow \*BSD `sys/queue.h` and `sys/tree.h` interfaces. Moreover, it tries to adopt more for custom
hash and compare functions, allocators, blooming filters and so on and so forth. Furthermore, `jahash` tries to provide really
thread safe access model based on custom rwlock implementation. Another goal of `jahash` is to reduce overhead per element. The current
version uses just extra 8 bytes per hash element (uthash uses about 56 bytes per element, on the contrary).

## Uthash deriving
`jahash` is based on algorithms from [uthash](http://troydhanson.github.io/uthash/) hash table. That means that the primitive operations are
all well tested and the performance of those operations is optimized as well. Moreover, `uthash` comes with the extensive API that is documented
and supported by `jahash`.

## Design principles
You need to define `HASH_ENTRY` element in your target structure, then you need to declare head in top-level structure by adding
`HASH_HEAD(name, type)` somewhere in it, for example:

~~~c
struct node {
	...
	HASH_ENTRY(node) hh;
}
struct top {
	...
	HASH_HEAD(, node) head;
}
~~~

Then you need to generate operations functions for your defined type. In a trivial case you should just write something like this:

~~~c
/* 
 * This generates a static global variable named _hash_ops_node_hh {...}
 * and a set of auxiliary functions named _hash_op_node_hh_hash and
 * _hash_op_node_hh_cmp
 */
HASH_GENERATE_STR(node, hh);
...
/* Somewhere in the code */
void init(struct top *top) 
{
	struct node *new, *found, search;
	HASH_INIT(&top->head, node, hh);
	...
	/* Add elements */
	HASH_ADD(&head, node, hh, new);
	
	/* Search for an element */
	search.key = "test";
	found = HASH_FIND(&top->head, node, hh, &search);
	
	/* Or even simpler */
	found = HASH_FIND_STR(&top->head, node, hh, "test");
}
~~~

This defines all standard methods (with no locking, no blooming and using `strcmp` as the compare routine). You can also define more complex
definitions for discovering all features available. ***TODO*** describe advanced interface.

## Hashing principles

`jahash` is designed to be able to use custom compare and hashing routines. This is extremely useful if you have, for instance, a case-insensitive
hash. In that case you can just define the caseless compare and hash ops and you are done. Another useful appliance is using of keyed hash functions,
such as `siphash` for creating secure hash tables.

***TODO*** examples.

## Locking principles

Whilst `uthash` requires to have external locking, `jahash` offers an advanced locking techniques based on individual hash bucket read/write locking.
However, an additional `rwlock` is used for the global hash table to allow resize operations to be implemented in a safe matter. If your hash table is
mostly immutable you can use very effective locking primitives to reach the maximum performance of hash tables.

***TODO*** describe the exact use cases.

## Memory management
***TODO*** describe custom memory management

## Built-in operations
***TODO*** implement and document

## Performance comparision
***Here are dragons***
