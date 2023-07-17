#include <pthread.h> /* pthread_cond, pthread_mutex */
#include <stdbool.h> /* bool */
#include <stdint.h>  /* uint64_t */

/* CPU cache line size */
#define CACHE_LINE_SIZE 64
#define ALIGN_TO_CACHE_LINE() __attribute__((__aligned__(CACHE_LINE_SIZE)))

/* --- Syncronization Primitives --- */

pthread_cond_t reader_proceed_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t reader_proceed_lock = PTHREAD_MUTEX_INITIALIZER;

/* --- Types --- */

typedef struct {
  _Atomic(bool) slot_has_data;
  _Atomic(bool) msg_written;
} msg_header;

typedef struct {
  _Atomic(uint64_t) read_idx ALIGN_TO_CACHE_LINE();
  _Atomic(uint64_t) write_idx ALIGN_TO_CACHE_LINE();
  uint64_t sz ALIGN_TO_CACHE_LINE();
  uint64_t msg_sz;
  void* msgs;
} rb_inbox_internal;

void write(rb_inbox_internal* const inbox, uint64_t const id,
           uint64_t const msg_sz, void const* restrict const msg) {
  register uint64_t const idx = id % inbox->sz;

  // TODO: check result of the "lock"
  pthread_mutex_lock(&reader_proceed_lock);

  while (id >= inbox->read_idx + inbox->sz) {
    pthread_cond_wait(&reader_proceed_cond, &reader_proceed_lock);
  }

  bool f = false;
  register msg_header* const header = x9_header_ptr(inbox, idx);

  for (;;) {
    if (atomic_compare_exchange_weak_explicit(&header->slot_has_data, &f, true,
                                              __ATOMIC_ACQUIRE,
                                              __ATOMIC_RELAXED)) {
      memcpy((char*)header + sizeof(msg_header), msg, msg_sz);
      atomic_store_explicit(&header->msg_written, true, __ATOMIC_RELEASE);
      break;
    } else {
      pthread_cond_wait(&reader_proceed_cond, &reader_proceed_lock);
    }
  }

  pthread_mutex_unlock(&reader_proceed_lock);
}

bool read(rb_inbox_internal* const inbox, uint64_t const msg_sz,
          void* restrict const outparam) {
  bool f = false;
  register uint64_t const idx = x9_load_idx(inbox, true);
  register msg_header* const header = x9_header_ptr(inbox, idx);

  if (atomic_load_explicit(&header->slot_has_data, __ATOMIC_RELAXED)) {
    if (atomic_load_explicit(&header->msg_written, __ATOMIC_ACQUIRE)) {
      memcpy(outparam, (char*)header + sizeof(msg_header), msg_sz);
      atomic_fetch_add_explicit(&inbox->read_idx, 1, __ATOMIC_RELEASE);
      atomic_store_explicit(&header->msg_written, false, __ATOMIC_RELAXED);
      atomic_store_explicit(&header->slot_has_data, false, __ATOMIC_RELEASE);

      pthread_mutex_lock(&reader_proceed_lock);
      pthread_cond_broadcast(&reader_proceed_cond);
      pthread_mutex_unlock(&reader_proceed_lock);

      return true;
    }
  }

  return false;
}