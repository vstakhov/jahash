/*
 * Copyright (c) 2014, Vsevolod Stakhov
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *	 * Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	 * Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR ''AS IS'' AND ANY
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
#ifndef HASH_PTHREAD_H_
#define HASH_PTHREAD_H_

#include <pthread.h>
/*
 * This file defines locking primitives for the hash table using pthreads
 */

/*
 * Define read-write locks for both nodes an resize operations. Suitable for
 * read mostly hash tables
 */
#define HASH_INIT_PTREAD_RWLOCK(head, type, field) do {                        \
    (head)->ops->lock_init = &_hash_pthread_rwlock_init_##type##_##field;      \
    (head)->ops->lock_read_lock = &_hash_pthread_rwlock_rlock_##type##_##field; \
    (head)->ops->lock_write_lock = &_hash_pthread_rwlock_wlock_##type##_##field; \
    (head)->ops->lock_read_lock = &_hash_pthread_rwlock_rlock_##type##_##field; \
    (head)->ops->lock_read_unlock = &_hash_pthread_rwlock_unlock_##type##_##field; \
    (head)->ops->lock_write_unlock = &_hash_pthread_rwlock_unlock_##type##_##field; \
    (head)->ops->lock_destroy = &_hash_pthread_rwlock_dtor_##type##_##field;   \
    (head)->ops->lockn_init = &_hash_pthread_rwlock_init_##type##_##field;     \
    (head)->ops->lockn_write_lock = &_hash_pthread_rwlock_wlock_##type##_##field; \
    (head)->ops->lockn_read_lock = &_hash_pthread_rwlock_rlock_##type##_##field; \
    (head)->ops->lockn_read_unlock = &_hash_pthread_rwlock_unlock_##type##_##field; \
    (head)->ops->lockn_write_unlock = &_hash_pthread_rwlock_unlock_##type##_##field; \
    (head)->ops->lockn_destroy = &_hash_pthread_rwlock_dtor_##type##_##field;  \
    (head)->ops->lockd = NULL;                                                 \
    (head)->ops->locknd = NULL;                                                \
} while(0)

/*
 * The following macro defines hash locks using read-write lock for resizing and
 * mutexes for nodes locks. This locking strategy mostly fits hash tables that
 * have equally distributed read and write operations probability. This is due
 * to priority propagation that makes read-write locks very expensive for writers
 */
#define HASH_INIT_PTREAD_MUTEX(head, type, field) do {                         \
    (head)->ops->lock_init = &_hash_pthread_rwlock_init_##type##_##field;      \
    (head)->ops->lock_read_lock = &_hash_pthread_rwlock_rlock_##type##_##field; \
    (head)->ops->lock_write_lock = &_hash_pthread_rwlock_wlock_##type##_##field; \
    (head)->ops->lock_read_lock = &_hash_pthread_rwlock_rlock_##type##_##field; \
    (head)->ops->lock_read_unlock = &_hash_pthread_rwlock_unlock_##type##_##field; \
    (head)->ops->lock_write_unlock = &_hash_pthread_rwlock_unlock_##type##_##field; \
    (head)->ops->lock_destroy = &_hash_pthread_rwlock_dtor_##type##_##field;   \
    (head)->ops->lockn_init = &_hash_pthread_rwlock_init_##type##_##field;     \
    (head)->ops->lockn_write_lock = &_hash_pthread_mtx_lock_##type##_##field;  \
    (head)->ops->lockn_read_lock = &_hash_pthread_mtx_lock_##type##_##field;   \
    (head)->ops->lockn_read_unlock = &_hash_pthread_mtx_unlock_##type##_##field; \
    (head)->ops->lockn_write_unlock = &_hash_pthread_mtx_unlock_##type##_##field; \
    (head)->ops->lockn_destroy = &_hash_pthread_mtx_dtor_##type##_##field;     \
    (head)->ops->lockd = NULL;                                                 \
    (head)->ops->locknd = NULL;                                                \
} while(0)


/*
 * Generate all pthread helpers for the specified hash type
 */
#define HASH_PTHREAD_GENERATE(type, field)                                     \
  static void* _hash_pthread_mtx_init_##type##_##field(void *d) {              \
    pthread_mutex_t *mtx;                                                      \
    mtx = malloc(sizeof(*mtx));                                                \
    if (mtx != NULL) pthread_mutex_init(mtx, NULL);                            \
    return (void *)mtx;                                                        \
  }                                                                            \
  static void _hash_pthread_mtx_lock_##type##_##field(void *m, void *d) {      \
    pthread_mutex_t *mtx = (pthread_mutex_t *)m;                               \
    pthread_mutex_lock(mtx);                                                   \
  }                                                                            \
  static void _hash_pthread_mtx_unlock_##type##_##field(void *m, void *d) {    \
    pthread_mutex_t *mtx = (pthread_mutex_t *)m;                               \
    pthread_mutex_unlock(mtx);                                                 \
  }                                                                            \
  static void _hash_pthread_mtx_dtor_##type##_##field(void *m, void *d) {      \
    pthread_mutex_t *mtx = (pthread_mutex_t *)m;                               \
    pthread_mutex_destroy(mtx);                                                \
    free(mtx);                                                                 \
  }                                                                            \
  static void* _hash_pthread_rwlock_init_##type##_##field(void *d) {           \
    pthread_rwlock_t *rwlck;                                                   \
    rwlck = malloc(sizeof(*rwlck));                                            \
    if (rwlck != NULL) pthread_rwlock_init(rwlck, NULL);                       \
    return (void *)rwlck;                                                      \
  }                                                                            \
  static void _hash_pthread_rwlock_rlock_##type##_##field(void *m, void *d) {  \
    pthread_rwlock_t *rwlck = (pthread_rwlock_t *)m;                           \
    pthread_rwlock_rdlock(rwlck);                                              \
  }                                                                            \
  static void _hash_pthread_rwlock_wlock_##type##_##field(void *m, void *d) {  \
    pthread_rwlock_t *rwlck = (pthread_rwlock_t *)m;                           \
    pthread_rwlock_wrlock(rwlck);                                              \
  }                                                                            \
  static void _hash_pthread_rwlock_unlock_##type##_##field(void *m, void *d) { \
    pthread_rwlock_t *rwlck = (pthread_rwlock_t *)m;                           \
    pthread_rwlock_unlock(rwlck);                                              \
  }                                                                            \
  static void _hash_pthread_rwlock_dtor_##type##_##field(void *m, void *d) {   \
    pthread_rwlock_t *rwlck = (pthread_rwlock_t *)m;                           \
    pthread_rwlock_destroy(rwlck);                                             \
    free(rwlck);                                                               \
  }                                                                            \

#endif /* HASH_PTHREAD_H_ */
