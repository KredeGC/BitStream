# Interface


## bit_writer and bit_reader interface
The `bit_writer` and `bit_reader` hold very little state themselves and rely on an external buffer instead. This means that nothing is allocated, and therefore no cleanup code is required. It also means that copying is not possible, since the streams don't have shared state. Move is still possible, and invalidates the object that was moved from. You can even move a bit_writer to a bit_reader.

As a general rule, most operations will return a boolean, indicating whether the operation was a success or not. To keep the library fast, operations never throw, unless debug-mode is enabled. An operation will usually only be unsuccessful if there's not enough space left in the buffer or if the parameters are out of range.

When calling any operation it is important to check the return value, as it's not wise to continue writing or reading after a failure.

| Method | Description |
| --- | --- |
| `bool align()` | Pads the buffer with up to 8 zeros, so that the next read/write is byte-aligned.<br/>The `bit_reader` will return false if the padded bits are not zeros. |
| `bool can_serialize_bits(uint32_t num_bits) const` | Returns true if the number of bits can fit in the buffer. |
| `uint32_t get_num_bits_serialized() const` | Returns the number of bits which have been read/written. |
| `uint32_t get_remaining_bits() const` | Returns the number of bits which have not been serialized.<br/>The same as `get_total_bits() - get_num_bits_serialized()`. |
| `uint32_t get_total_bits() const` | Returns the size of the buffer, in bits. |
| `bool pad_to_size(uint32_t num_bytes)` | Pads the buffer up to the given number of bytes.<br/>Returns false if the current size of the buffer is bigger than `num_bytes`<br/>The `bit_reader` will return false if the padded bits are not zeros. |
| `bool serialize<Trait>(Args&&...)` | Writes/reads into the buffer, using the given `Trait`. The various traits to use can be found in the [`traits/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/traits/) directory.<br/>It is also possible to use your own custom traits by specializing `serialize_traits<T>`. |
| `bool serialize_bits(uint32_t& value, uint32_t num_bits)` | Writes/reads the first `num_bits` bits of `value` into the buffer.<br/>Returns false if `num_bits` is less than 1 or greater than 32.<br/>Returns false if reading/writing the given number of bits would overflow the buffer. |
| `bool serialize_bytes(uint8_t* bytes, uint32_t num_bits)` | Writes/reads the first `num_bits` bits of the given byte array, 32 bits at a time.<br/>Returns false if `num_bits` is less than 1.<br/>Returns false if reading/writing the given number of bits would overflow the buffer. |
| `bool serialize_checksum(uint32_t protocol)` | When reading, this method compares the first 32 bits of the buffer with a checksum of the buffer + `protocol`.<br/>When writing, this method writes the first 32 bits as a checksum of the buffer + `protocol`.<br/>Returns false if there's no space for the 32 bits, or if the protocol doesn't match the checksum. |

## bit_writer specific interface
There are some methods that only the `bit_writer` has, since the `bit_reader` would have no use for them.

| Method | Description |
| --- | --- |
| `uint32_t flush()` | Flushes any remaining bits into the buffer. Use this when you no longer intend to write anything to the buffer. |
| `bool prepend_checksum()` | Instructs the writer that you intend to use `serialize_checksum()` later on, and to reserve the first 32 bits. |
| `bool serialize_into(bit_writer& writer) const` | Writes the contents of the buffer into the given `writer`. Essentially copies the entire buffer without modifying it. |