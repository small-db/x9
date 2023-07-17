/* --- Opaque Bypes --- */

typedef struct rb_inbox_internal rb_inbox;

/* --- Public API --- */

/* Creates a rb_inbox with a buffer of size 'sz' and a 'msg_sz' which the inbox
 * is expected to receive.
 *
 * Example:
 *   rb_inbox* inbox = create_inbox(99, sizeof(<some struct>));
 */
__attribute__((nonnull)) rb_inbox* create_inbox(uint64_t const sz,
                                                char const* restrict const name,
                                                uint64_t const msg_sz);

__attribute__((nonnull)) void write(rb_inbox* const inbox, uint64_t const id,
                                    uint64_t const msg_sz,
                                    void const* restrict const msg);

__attribute__((nonnull)) bool read(rb_inbox* const inbox, uint64_t const msg_sz,
                                   void* restrict const outparam);