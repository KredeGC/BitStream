<div align="center">
<h1>BitStream</h1>

![Windows supported](https://img.shields.io/badge/Windows-win--10-green?style=flat-square)
![Linux supported](https://img.shields.io/badge/Linux-Ubuntu-green?style=flat-square)
![MacOS untested](https://img.shields.io/badge/MacOS-Untested-red?style=flat-square)

An extensible C++ library for serializing and quantizing types into tightly packed bitstreams.

[![Release](https://img.shields.io/github/v/release/KredeGC/BitStream?display_name=tag&style=flat-square)](https://github.com/KredeGC/BitStream/releases/latest)
[![Size](https://img.shields.io/github/languages/code-size/KredeGC/BitStream?style=flat-square)](https://github.com/KredeGC/BitStream/releases/latest)
[![License](https://img.shields.io/github/license/KredeGC/BitStream?style=flat-square)](https://github.com/KredeGC/BitStream/blob/master/LICENSE)

[![Issues](https://img.shields.io/github/issues/KredeGC/BitStream?style=flat-square)](https://github.com/KredeGC/BitStream/issues)
[![Tests](https://img.shields.io/github/actions/workflow/status/KredeGC/BitStream/main.yml?branch=master&style=flat-square)](https://github.com/KredeGC/BitStream/actions/workflows/main.yml)

</div>

# Compatibility
This library was made with C++17 in mind and is not be compatible with earlier versions.
Many of the features use `if constexpr`, which is only available from 17 and up.
If you really want it to work with earlier versions, you should just be able to replace the newer features with C++14 counterparts.

# Installation
As this is a header-only library, you can simply copy the header files directly into your project.
The header files can either be downloaded from the [releases page](https://github.com/KredeGC/BitStream/releases) or from the [`include/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream) directory on the master branch.
The source and header files inside the `test/` directory are only tests and should not be included into your project, unless you want to test them internally.

# Usage
The library has a global header file ([`bitstream/bitstream.h`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/bitstream.h)) which includes every other header file in the library.

If you only need certain features, you can simply include the ones you need.
The files are stored in the categories:
* `quantization/` - Files relating to quantizing types into fewer bits
* `stream/` - Files relating to reading and writing bits into a stream
* `traits/` - Files relating to various serialization traits, like serializble strings, integrals etc.

# Interface


## bit_writer and bit_reader interface
The `bit_writer` and `bit_reader` hold very little state themselves and rely on an external buffer instead. This means that nothing is allocated, and therefore no cleanup code is required. It also means that copying is not possible, since the streams don't have shared state. Move is still possible, and invalidates the object that was moved from.

As a general rule, most operations will return a boolean, indicating whether the operation was a success or not. To keep the library fast, operations never throw, unless debug-mode is enabled. An operation will usually only be unsuccessful if there's not enough space left in the buffer or if the parameters are out of range.

When calling any operation it is important to check the return value, as it's not wise to continue writing or reading after a failure.

| Method | Description |
| --- | --- |
| `bool align()` | Pads the buffer with up to 8 zeros, so that the next read/write is byte-aligned.<br/>The `bit_reader` will return false if the padded bits are not zeros. |
| `bool pad_to_size(uint32_t num_bytes)` | Pads the buffer up to the given number of bytes.<br/>Returns false if the current size of the buffer is bigger than `num_bytes`<br/>The `bit_reader` will return false if the padded bits are not zeros. |
| `bool serialize<Trait>(Args&&...)` | Writes/reads into the buffer, using the given `Trait`. The various traits to use can be found in the [`traits/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/traits/) directory.<br/>It is also possible to use your own custom traits. |
| `bool serialize_bits(uint32_t& value, uint32_t num_bits)` | Writes/reads the first `num_bits` bits of `value` into the buffer.<br/>Returns false if `num_bits` is less than 1 or greater than 32.<br/>Returns false if reading/writing the given number of bits would overflow the buffer. |
| `bool serialize_bytes(uint8_t* bytes, uint32_t num_bits)` | Writes/reads the first `num_bits` bits of the given byte array, 32 bits at a time.<br/>Returns false if `num_bits` is less than 1.<br/>Returns false if reading/writing the given number of bits would overflow the buffer. |
| `bool serialize_checksum(uint32_t protocol)` | When reading, this method compares the first 32 bits of the buffer with a checksum of the buffer + `protocol`.<br/>When writing, this method writes the first 32 bits as a checksum of the buffer + `protocol`.<br/>Returns false if there's no space for the 32 bits, or if the protocol doesn't match the checksum. |
| `` | |

## bit_writer specific interface
There are some methods that only the `bit_writer` has, since the `bit_reader` would have no use for them.

| Method | Description |
| --- | --- |
| `uint32_t flush()` | Flushes any remaining bits into the buffer. Use this when you no longer intend to write anything to the buffer. |
| `bool prepend_checksum()` | Instructs the writer that you intend to use `serialize_checksum()` later on, and to reserve the first 32 bits. |
| `bool serialize_int(bit_writer& writer)` | Writes the contents of the buffer into the given `writer`. Essentially copies the entire buffer without modifying it. |

# Serialization Examples
The examples below follow the same structure: First writing to a buffer and then reading from it.

Writing the first 5 bits of an int to the buffer, then reading it back:
```cpp
// Create a writer, referencing the buffer and its size
uint8_t buffer[1];
bit_writer writer(buffer, 1);

// Write the value
uint32_t value = 27; // We can choose any value below 2^5. Otherwise we need more bits
writer.serialize_bits(value, 5);

// Flush the writer's remaining state into the buffer
uint32_t num_bytes = writer.flush();

// Create a reader, referencing the buffer and bytes written
bit_reader reader(buffer, num_bytes);

// Read the value back
uint32_t out_value; // We don't have to initialize it yet
reader.serialize_bits(out_value, 5); // out_value should now have a value of 27
```

Writing a signed int to the buffer, within a range:
```cpp
// Create a writer, referencing the buffer and its size
uint8_t buffer[4];
bit_writer writer(buffer, 4);

// Write the value
int32_t value = -45; // We can choose any value within the range below
writer.serialize<int32_t>(value, -90, 40);

// Flush the writer's remaining state into the buffer
uint32_t num_bytes = writer.flush();

// Create a reader, referencing the buffer and bytes written
bit_reader reader(buffer, num_bytes);

// Read the value back
int32_t out_value; // We don't have to initialize it yet
reader.serialize<int32_t>(out_value, -90, 40); // out_value should now have a value of -45
```

Writing a c-style string into the buffer:
```cpp
// Create a writer, referencing the buffer and its size
uint8_t buffer[32];
bit_writer writer(buffer, 4);

// Write the value
const char* value = "Hello world!";
writer.serialize<int32_t>(value, 32U); // The second argument is the maximum size we expect the string to be

// Flush the writer's remaining state into the buffer
uint32_t num_bytes = writer.flush();

// Create a reader, referencing the buffer and bytes written
bit_reader reader(buffer, num_bytes);

// Read the value back
char out_value[32U]; // Set the size to the max size
reader.serialize<int32_t>(out_value, 32U); // out_value should now contain "Hello world!\0"
```

# Extensibility
The library is made with extensibility in mind.
The `bit_writer` and `bit_reader` use a template trait specialization of the given type to deduce how to serialize and deserialize the object.
The general structure of a trait looks like the following:

```cpp
template<>
struct serialize_traits<TRAIT_TYPE> // The type to use when serializing
{
    // Will be called when writing the object to a stream
    static bool serialize(bit_writer& stream, ...)
    { ... }
    
    // Will be called when reading the object from a stream
    static bool serialize(bit_reader& stream, ...)
    { ... }
};
```

To use this trait to serialize an object you need to explicitly specify it:
```cpp
bool status = writer.serialize<TRAIT_TYPE>(...);
```

The specialization can also be unified, if writing and reading look similar:
```cpp
template<>
struct serialize_traits<TRAIT_TYPE> // The type to use when serializing
{
    // Will be called when writing or reading the object to a stream
    template<typename Stream>
    static bool serialize(Stream& stream, ...)
    { ... }
};
```

The specialization can also be templated to work with a number of types.
It also works with `enable_if`:
```cpp
// This trait will be used by any integral type (signed and unsigned ints)
template<typename T>
struct serialize_traits<T, typename std::enable_if_t<std::is_integral<T>::value>>
{ ... };
```

More concrete examples of traits can be found in the [`traits/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/traits/) directory.

# Credits
* The quantization classes are from [INSERT]