<div align="center">
<h1>BitStream</h1>

![Windows supported](https://img.shields.io/badge/Windows-Win--10-green?style=flat-square)
![Linux supported](https://img.shields.io/badge/Linux-Ubuntu-green?style=flat-square)
![MacOS untested](https://img.shields.io/badge/MacOS-Untested-red?style=flat-square)

An extensible C++ library for serializing and quantizing types into tightly packed bitstreams.<br/>
Based on [Glenn Fiedler's articles](https://gafferongames.com/post/reading_and_writing_packets/) about packet serialization.

[![Release](https://img.shields.io/github/v/release/KredeGC/BitStream?display_name=tag&style=flat-square)](https://github.com/KredeGC/BitStream/releases/latest)
[![Size](https://img.shields.io/github/languages/code-size/KredeGC/BitStream?style=flat-square)](https://github.com/KredeGC/BitStream/releases/latest)
[![License](https://img.shields.io/github/license/KredeGC/BitStream?style=flat-square)](https://github.com/KredeGC/BitStream/blob/master/LICENSE)

[![Issues](https://img.shields.io/github/issues/KredeGC/BitStream?style=flat-square)](https://github.com/KredeGC/BitStream/issues)
[![Tests](https://img.shields.io/github/actions/workflow/status/KredeGC/BitStream/main.yml?branch=master&style=flat-square)](https://github.com/KredeGC/BitStream/actions/workflows/main.yml)
</div>

# Compatibility
This library was made with C++17 in mind and is not compatible with earlier versions.
Many of the features use `if constexpr`, which is only available from 17 and up.
If you really want it to work with earlier versions, you should just be able to replace the newer features with C++1x counterparts.

# Installation
As this is a header-only library, you can simply copy the header files directly into your project and include them where relevant.
The header files can either be downloaded from the [releases page](https://github.com/KredeGC/BitStream/releases) or from the [`include/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream) directory on the master branch.
The source and header files inside the `test/` directory are only tests and should not be included into your project, unless you wish to test the library as part of your pipeline.

# Usage
The library has a global header file ([`bitstream/bitstream.h`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/bitstream.h)) which includes every other header file in the library.

If you only need certain features, you can simply include the files you need.
The files are stored in categories:
* [`quantization/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/quantization/) - Files relating to quantizing floats and quaternions into fewer bits
* [`stream/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/stream/) - Files relating to streams that read and write bits
* [`traits/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/traits/) - Files relating to various serialization traits, like serializble strings, integrals etc.

It is also possible to dynamically put a break point or trap when a bitstream would have otherwise returned false. This can be great for debugging custom serialization code, but should generally be left out of production code. Simply `#define BS_DEBUG_BREAK` before including any of the library header files if you want to break when an operation fails.

For more examples of usage, see the [Serialization Examples](#serialization-examples) below.
You can also look at the unit tests to get a better idea about what you can expect from the library.

# Interface
Refer to [INTERFACE.md](INTERFACE.md) for more detailed information about what can be accessed in the library.

# Serializables - serialize_traits
Below is a noncomprehensive list of serializable traits.
A big feature of the library is extensibility, which is why you can add your own types as you please, or choose not to include specific types if you don't need them.

## Bounded integers - T
A trait that covers all signed and unsigned integers.<br/>
Takes the integer by reference and a lower and upper bound.<br/>
The upper and lower bounds will default to T's upper and lower bound if left unspecified, effectively making the object unbounded.

The call signature can be seen below:
```cpp
bool serialize<T>(T& value, T min = numeric_limits<T>::min(), T max = numeric_limits<T>::max());
```
As well as a short example of its usage:
```cpp
int16_t value = 1027;
bool status = stream.serialize<int16_t>(value, -512, 2098);
```

## Compile-time bounded integers - bounded_int\<T, T Min, T Max\>
A trait that covers all signed and unsigned integers within a `bounded_int` wrapper.<br/>
Takes the integer by reference and a lower and upper bound as template parameters.<br/>
This is preferable if you know the bounds at compile time.

The call signature can be seen below:
```cpp
bool serialize<bounded_int<T, Min, Max>>(T& value);
```
As well as a short example of its usage:
```cpp
int16_t value = 1027;
bool status = stream.serialize<bounded_int<int16_t, -512, 2098>>(value);
```

## C-style strings - const char*
A trait that only covers c-style strings.<br/>
Takes the pointer and a maximum expected string length.

The call signature can be seen below:
```cpp
bool serialize<const char*>(const char* value, uint32_t max_size);
```
As well as a short example of its usage:
```cpp
const char* value = "Hello world!";
bool status = stream.serialize<const char*>(value, 32);
```

## Modern strings - std::basic_string\<T\>
A trait that covers any combination of basic_string, including strings with different allocators.<br/>
Takes a reference to the string and a maximum expected string length.

The, somewhat bloated, call signature can be seen below:
```cpp
bool serialize<std::basic_string<T, Traits, Alloc>>(std::basic_string<T, Traits, Alloc>& value, uint32_t max_size);
// For std::string this would look like:
bool serialize<std::string>(std::string& value, uint32_t max_size);
```
As well as a short example of its usage:
```cpp
std::string value = "Hello world!";
bool status = stream.serialize<std::string>(value, 32);
```

## Single-precision float - float
A trait that covers an entire float, with no quantization.<br/>
Takes a reference to the float.

The call signature can be seen below:
```cpp
bool serialize<float>(float& value);
```
As well as a short example of its usage:
```cpp
float value = 0.12345678f;
bool status = stream.serialize<float>(value);
```

## Half-precision float - half_precision
A trait that covers a float which has been quantized to 16 bits.<br/>
Takes a reference to the float.

The call signature can be seen below:
```cpp
bool serialize<half_precision>(float& value);
```
As well as a short example of its usage:
```cpp
float value = 0.12345678f;
bool status = stream.serialize<half_precision>(value);
```

## Bounded float - bounded_range
A trait that covers a quantized float.<br/>
Takes a reference to the bounded_range and a reference to the float.

The call signature can be seen below:
```cpp
bool serialize<bounded_range>(bounded_range& range, float& value);
```
As well as a short example of its usage:
```cpp
bounded_range range(1.0f, 4.0f, 1.0f / 128.0f);
float value = 0.1234f;
bool status = stream.serialize<bounded_range>(range, value);
```

## Quaternion - smallest_three\<Q, BitsPerElement\>
A trait that covers any quaternion type in any order, as long as it's consistent.<br/>
Takes a reference to the quaternion.

The call signature can be seen below:
```cpp
bool serialize<smallest_three<Q, BitsPerElement>>(Q& value);
```
As well as a short example of its usage:
```cpp
struct quaternion
{
    float w;
    float x;
    float y;
    float z;
    
    float operator[](size_t index) const
    {
        return reinterpret_cast<const float*>(this)[index];
    }
};
quaternion value = { 1.0f, 0.0f, 0.0f, 0.0f };
bool status = stream.serialize<smallest_three<quaternion, 12>>(value);
```

# Serialization Examples
The examples below follow the same structure: First writing to a buffer and then reading from it. Each example is littered with comments about the procedure, as well as what outcome is expected.

Writing the first 5 bits of an int to the buffer, then reading it back:
```cpp
// Create a writer, referencing the buffer and its size
uint8_t buffer[4]; // Buffer must be a multiple of 4 bytes / 32 bits
bit_writer writer(buffer, 4);

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

// Create a reader by moving and invalidating the writer
bit_reader reader(std::move(writer));

// Read the value back
int32_t out_value; // We don't have to initialize it yet
reader.serialize<int32_t>(out_value, -90, 40); // out_value should now have a value of -45
```

Writing a c-style string into the buffer:
```cpp
// Create a writer, referencing the buffer and its size
uint8_t buffer[32];
bit_writer writer(buffer, 32);

// Write the value
const char* value = "Hello world!";
writer.serialize<const char*>(value, 32U); // The second argument is the maximum size we expect the string to be

// Flush the writer's remaining state into the buffer
uint32_t num_bytes = writer.flush();

// Create a reader by moving and invalidating the writer
bit_reader reader(std::move(writer));

// Read the value back
char out_value[32]; // Set the size to the max size
reader.serialize<const char*>(out_value, 32U); // out_value should now contain "Hello world!\0"
```

Writing a float into the buffer with a bounded range and precision:
```cpp
// Create a writer, referencing the buffer and its size
uint8_t buffer[4];
bit_writer writer(buffer, 4);

// Write the value
bounded_range range(1.0f, 4.0f, 1.0f / 128.0f); // Min, Max, Precision
float value = 1.2345678f;
writer.serialize<bounded_range>(range, value);

// Flush the writer's remaining state into the buffer
uint32_t num_bytes = writer.flush();

// Create a reader by moving and invalidating the writer
bit_reader reader(std::move(writer));

// Read the value back
float out_value;
reader.serialize<bounded_range>(range, out_value); // out_value should now be a value close to 1.2345678f
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

Note that `TRAIT_TYPE` does not necessarily have to be part of the function definitions. It is purely used to specify which trait to use when serializing, since it cannot be deduced from the arguments.<br/>
To use the trait above to serialize an object you need to explicitly specify it:
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
// This trait will be used by any integral pointer type (char*, uint16_t* etc.)
template<typename T>
struct serialize_traits<T*, typename std::enable_if_t<std::is_integral_v<T>>>
{ ... };
```

More concrete examples of traits can be found in the [`traits/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/traits/) directory.

# 3rd party
The library has no dependencies, but does build upon some code from the [NetStack](https://github.com/nxrighthere/NetStack) library by Stanislav Denisov, which is free to use, as per their [license](https://github.com/nxrighthere/NetStack/blob/master/LICENSE). The code in question is about quantizing floats and quaternions, which has simply been translated from C# into C++ for the purposes of this library.

# License
The library is licensed under the [BSD-3-Clause license](https://github.com/KredeGC/BitStream/blob/master/LICENSE) and is subject to the terms and conditions in that license as well as the [NetStack license](https://github.com/nxrighthere/NetStack/blob/master/LICENSE).