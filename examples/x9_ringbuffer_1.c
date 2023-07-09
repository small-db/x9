/* x9_example_5.c
 *
 *  Three producers.
 *  Three consumers reading concurrently from the same inbox.
 *  One message type.
 *
 *                                    ┌────────┐
 *  ┌────────┐       ┏━━━━━━━━┓    ─ ─│Consumer│
 *  │Producer│──────▷┃        ┃   │   └────────┘
 *  ├────────┤       ┃        ┃       ┌────────┐
 *  │Producer│──────▷┃ inbox  ┃◁──┤─ ─│Consumer│
 *  ├────────┤       ┃        ┃       └────────┘
 *  │Producer│──────▷┃        ┃   │   ┌────────┐
 *  └────────┘       ┗━━━━━━━━┛    ─ ─│Consumer│
 *                                    └────────┘
 *
 *  This example showcases the use of 'x9_read_from_shared_inbox'.
 *
 *  Data structures used:
 *   - x9_inbox
 *
 *  Functions used:
 *   - x9_create_inbox
 *   - x9_inbox_is_valid
 *   - x9_write_to_inbox_spin
 *   - x9_read_from_shared_inbox
 *   - x9_free_inbox
 *
 *  Test is considered passed iff:
 *   - None of the threads stall and exit cleanly after doing the work.
 *   - All messages sent by the producer(s) are received and asserted to be
 *   valid by the consumer(s).
 *   - Each consumer processes at least one message.
 */

#include <assert.h>  /* assert */
#include <pthread.h> /* pthread_t, pthread functions */
#include <stdbool.h> /* bool */
#include <stdint.h>  /* uint64_t */
#include <stdio.h>   /* printf */
#include <stdlib.h>  /* EXIT_SUCCESS */

#include "../x9.h"

/* Both producer and consumer loops, would commonly be infinite loops, but for
 * the purpose of testing a reasonable NUMBER_OF_MESSAGES is defined. */
#define NUMBER_OF_MESSAGES 100 * 10000

#define NUMBER_OF_PRODUCER_THREADS 3

typedef struct {
  x9_inbox* inbox;
  uint64_t msgs_read;
  uint8_t producer_id;
} th_struct;

typedef struct {
  uint64_t v;
} msg;

static void* producer_fn(void* args) {
  th_struct* data = (th_struct*)args;

  msg m = {0};
  for (uint64_t v = data->producer_id; v < NUMBER_OF_MESSAGES;
       v += NUMBER_OF_PRODUCER_THREADS) {
    m.v = v;
    x9_write_to_inbox_spin_withid(data->inbox, v, sizeof(msg), &m);
    // printf("Producer wrote: %ld\n", m.v);
  }
  printf("Producer done\n");
  return 0;
}

static void* consumer_fn(void* args) {
  th_struct* data = (th_struct*)args;

  msg m = {0};
  uint64_t prev = -1;
  for (;;) {
    if (x9_read_from_shared_inbox(data->inbox, sizeof(msg), &m)) {
      // printf("Consumer read: %ld\n", m.v);
      assert(m.v - prev == 1);
      prev = m.v;
      ++data->msgs_read;
      if (m.v == NUMBER_OF_MESSAGES - 1) {
        printf("Consumer done\n");
        return 0;
      }
    }
  }
}

int main(void) {
  /* Create inbox */
  x9_inbox* const inbox = x9_create_inbox(100, "ibx", sizeof(msg));

  /* Using assert to simplify code for presentation purpose. */
  assert(x9_inbox_is_valid(inbox));

  /* Producers */
  struct producer {
    pthread_t th;
    th_struct data;
  } producers[NUMBER_OF_PRODUCER_THREADS];

  // init producer list
  for (int i = 0; i < NUMBER_OF_PRODUCER_THREADS; ++i) {
    producers[i].data.inbox = inbox;
    producers[i].data.producer_id = i;
  }

  /* Consumers */
  pthread_t consumer_1_th = {0};
  th_struct consumer_1_struct = {.inbox = inbox};

  /* Launch threads */
  for (int i = 0; i < NUMBER_OF_PRODUCER_THREADS; ++i) {
    pthread_create(&producers[i].th, NULL, producer_fn, &producers[i].data);
  }
  pthread_create(&consumer_1_th, NULL, consumer_fn, &consumer_1_struct);

  /* Join them */
  for (int i = 0; i < NUMBER_OF_PRODUCER_THREADS; ++i) {
    pthread_join(producers[i].th, NULL);
  }
  pthread_join(consumer_1_th, NULL);

  /* Assert that all of the consumers read from the shared inbox. */
  assert(consumer_1_struct.msgs_read > 0);

  /* Assert that the total number of messages read == (NUMBER_OF_MESSAGES) */
  printf("Total messages read: %ld\n", consumer_1_struct.msgs_read);
  printf("Total messages written: %d\n", NUMBER_OF_MESSAGES);
  assert(NUMBER_OF_MESSAGES == consumer_1_struct.msgs_read);

  /* Cleanup */
  x9_free_inbox(inbox);

  printf("TEST PASSED: x9_ringbuffer_1.c\n");
  return EXIT_SUCCESS;
}
