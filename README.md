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

# Table of Content
* [Compatibility](#compatibility)
* [Installation](#installation)
* [Usage](#usage)
* [Documentation](#documentation)
* [Serializables - serialize_traits](#serializables---serialize_traits)
  * [Booleans - bool](#booleans---bool)
  * [Bounded integers - T](#bounded-integers---t)
  * [Compile-time bounded integers - bounded_int\<T, T Min, T Max\>](#compile-time-bounded-integers---bounded_intt-t-min-t-max)
  * [C-style strings - const char*](#c-style-strings---const-char)
  * [Modern strings - std::basic_string\<T\>](#modern-strings---stdbasic_stringt)
  * [Double-precision float - double](#double-precision-float---double)
  * [Single-precision float - float](#single-precision-float---float)
  * [Half-precision float - half_precision](#half-precision-float---half_precision)
  * [Bounded float - bounded_range](#bounded-float---bounded_range)
  * [Quaternion - smallest_three\<Q, BitsPerElement\>](#quaternion---smallest_threeq-bitsperelement)
* [Serialization Examples](#serialization-examples)
* [Extensibility](#extensibility)
  * [Adding new serializables types](#adding-new-serializables-types)
  * [Unified serialization](#unified-serialization)
  * [Partial trait specializations](#partial-trait-specializations)
  * [Trait deduction](#trait-deduction)
* [Building and running tests](#building-and-running-tests)
* [3rd party](#3rd-party)
* [License](#license)

# Compatibility
This library was made with C++17 in mind and is not compatible with earlier versions.
Many of the features use `if constexpr`, which is only available from 17 and up.
If you really want it to work with earlier versions, you should just be able to replace the newer features with C++1x counterparts.

# Installation
As this is a header-only library, you can simply copy the header files directly into your project and include them where relevant.
The header files can either be downloaded from the [releases page](https://github.com/KredeGC/BitStream/releases) or from the [`include/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream) directory on the master branch.

The source and header files inside the `src/` directory are only tests and should not be included into your project, unless you wish to test the library as part of your pipeline.

# Usage
The library has a global header file ([`bitstream/bitstream.h`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/bitstream.h)) which includes every other header file in the library.

If you only need certain features you can instead opt to just include the files you need.
The files are stored in categories:
* [`quantization/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/quantization/) - Files relating to quantizing floats and quaternions into fewer bits
* [`stream/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/stream/) - Files relating to streams that read and write bits
* [`traits/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/traits/) - Files relating to various serialization traits, like serializble strings, integrals etc.

An important aspect of the serialiaztion is performance, since the library is meant to be used in a tight loop, like with networking.
This is why most operations don't use exceptions, but instead return true or false depending on whether the operation was a success.
It's important to check these return values after every operation, especially when reading from an unknown source.
You can check it manually or use the `BS_ASSERT(x)` macro for this, if you want your function to return false on failure.

It is also possible to dynamically put a break point or trap when a bitstream would have otherwise returned false. This can be great for debugging custom serialization code, but should generally be left out of production code. Simply `#define BS_DEBUG_BREAK` before including any of the library header files if you want to break when an operation fails.

For more concrete examples of usage, see the [Serialization Examples](#serialization-examples) below.
If you need to add your own serializable types you should also look at the [Extensibility](#extensibility) section.
You can also look at the unit tests to get a better idea about what you can expect from the library.

# Documentation
Refer to [the documentation](https://kredegc.github.io/BitStream/namespaces.html) for more information about what different classes provide.

# Serializables - serialize_traits
Below is a noncomprehensive list of serializable traits.
A big feature of the library is extensibility, which is why you can add your own types as you please, or choose not to include specific types if you don't need them.

## Booleans - bool
A trait that covers a single bool.<br/>
Takes the bool by reference and serializes it as a single bit.<br/>

The call signature can be seen below:
```cpp
bool serialize<bool>(bool& value);
```
As well as a short example of its usage:
```cpp
bool in_value = true;
bool out_value;
bool status_write = writer.serialize<bool>(in_value);
bool status_read = reader.serialize<bool>(out_value);
```

## Bounded integers - T
A trait that covers all signed and unsigned integers.<br/>
Takes the integer by reference and a lower and upper bound.<br/>
The upper and lower bounds will default to T's upper and lower bounds if left unspecified, effectively making the object unbounded.

The call signature can be seen below:
```cpp
bool serialize<T>(T& value, T min = numeric_limits<T>::min(), T max = numeric_limits<T>::max());
```
As well as a short example of its usage:
```cpp
int16_t in_value = 1027;
int16_t out_value;
bool status_write = writer.serialize<int16_t>(in_value, -512, 2098);
bool status_read = reader.serialize<int16_t>(out_value, -512, 2098);
```

## Compile-time bounded integers - bounded_int\<T, T Min, T Max\>
A trait that covers all signed and unsigned integers within a `bounded_int` wrapper.<br/>
Takes the integer by reference and a lower and upper bound as template parameters.<br/>
This is preferable if you know the bounds at compile time as it skips having to calculate the number of bits required.<br/>
The upper and lower bounds will default to T's upper and lower bounds if left unspecified, effectively making the object unbounded.

The call signature can be seen below:
```cpp
bool serialize<bounded_int<T, Min, Max>>(T& value);
```
As well as a short example of its usage:
```cpp
int16_t in_value = 1027;
int16_t out_value;
bool status_write = writer.serialize<bounded_int<int16_t, -512, 2098>>(in_value);
bool status_read = reader.serialize<bounded_int<int16_t, -512, 2098>>(out_value);
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
const char* in_value = "Hello world!";
char out_value[32]{ 0 };
bool status_write = writer.serialize<const char*>(in_value, 32);
bool status_read = reader.serialize<const char*>(out_value, 32);
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
std::string in_value = "Hello world!";
std::string out_value;
bool status_write = writer.serialize<std::string>(in_value, 32);
bool status_read = reader.serialize<std::string>(out_value, 32);
```

## Double-precision float - double
A trait that covers an entire double, with no quantization.<br/>
Takes a reference to the double.

The call signature can be seen below:
```cpp
bool serialize<double>(double& value);
```
As well as a short example of its usage:
```cpp
double in_value = 0.12345678652;
double out_value;
bool status_write = writer.serialize<double>(in_value);
bool status_read = reader.serialize<double>(out_value);
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
float in_value = 0.12345678f;
float out_value;
bool status_write = writer.serialize<float>(in_value);
bool status_read = reader.serialize<float>(out_value);
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
float in_value = 0.12345678f;
float out_value;
bool status_write = writer.serialize<half_precision>(in_value);
bool status_read = reader.serialize<half_precision>(out_value);
```

## Bounded float - bounded_range
A trait that covers a bounded float.<br/>
Takes a reference to the bounded_range and a reference to the float.

The call signature can be seen below:
```cpp
bool serialize<bounded_range>(const bounded_range& range, float& value);
```
As well as a short example of its usage:
```cpp
bounded_range range(1.0f, 4.0f, 1.0f / 128.0f);
float in_value = 0.1234f;
float out_value;
bool status_write = writer.serialize<bounded_range>(range, in_value);
bool status_read = reader.serialize<bounded_range>(range, out_value);
```

## Quaternion - smallest_three\<Q, BitsPerElement\>
A trait that covers any quaternion type in any order, as long as it's consistent.<br/>
Quantizes the quaternion using the given BitsPerElement.<br/>
Takes a reference to the quaternion.

The call signature can be seen below:
```cpp
bool serialize<smallest_three<Q, BitsPerElement>>(Q& value);
```
As well as a short example of its usage:
```cpp
struct quaternion
{
    // smallest_three supports any combination of w, x, y and z, as long as it's consistent
    float values[4];

    // The constructor order must be the same as the operator[]
    float operator[](size_t index) const
    {
        return values[index];
    }
};
quaternion in_value{ 1.0f, 0.0f, 0.0f, 0.0f };
quaternion out_value;
bool status_write = writer.serialize<smallest_three<quaternion, 12>>(in_value);
bool status_read = reader.serialize<smallest_three<quaternion, 12>>(out_value);
```

# Serialization Examples
The examples below follow the same structure: First writing to a buffer and then reading from it. Each example is littered with comments about the procedure, as well as what outcome is expected.

Writing the first 5 bits of an int to the buffer, then reading it back:
```cpp
// Create a writer, referencing the buffer and its size
alignas(uint32_t) uint8_t buffer[4]; // Buffer must be a multiple of 4 bytes / 32 bits and 4-byte-aligned
bit_writer writer(buffer, 4);

// Write the value
uint32_t value = 27; // We can choose any value below 2^5. Otherwise we need more than 5 bits
writer.serialize_bits(value, 5);

// Flush the writer's remaining state into the buffer
uint32_t num_bits = writer.flush();

// Create a reader, referencing the buffer and bits written
bit_reader reader(buffer, num_bits);

// Read the value back
uint32_t out_value; // We don't have to initialize it yet
reader.serialize_bits(out_value, 5); // out_value should now have a value of 27
```

Writing a signed int to the buffer, within a range:
```cpp
// Create a writer, referencing the buffer and its size
byte_buffer<4> buffer; // byte_bufer is just a wrapper for a 4-byte aligned buffer
bit_writer writer(buffer);

// Write the value
int32_t value = -45; // We can choose any value within the range below
writer.serialize<int32_t>(value, -90, 40); // A lower and upper bound which the value will be quantized between

// Flush the writer's remaining state into the buffer
uint32_t num_bits = writer.flush();

// Create a reader, referencing the buffer and bits written
bit_reader reader(buffer, num_bits);

// Read the value back
int32_t out_value; // We don't have to initialize it yet
reader.serialize<int32_t>(out_value, -90, 40); // out_value should now have a value of -45
```

Writing a c-style string into the buffer:
```cpp
// Create a writer, referencing the buffer and its size
byte_buffer<32> buffer;
bit_writer writer(buffer);

// Write the value
const char* value = "Hello world!";
writer.serialize<const char*>(value, 32U); // The second argument is the maximum size we expect the string to be

// Flush the writer's remaining state into the buffer
uint32_t num_bits = writer.flush();

// Create a reader, referencing the buffer and bits written
bit_reader reader(buffer, num_bits);

// Read the value back
char out_value[32]; // Set the size to the max size
reader.serialize<const char*>(out_value, 32U); // out_value should now contain "Hello world!\0"
```

Writing a std::string into the buffer:
```cpp
// Create a writer, referencing the buffer and its size
byte_buffer<32> buffer;
bit_writer writer(buffer);

// Write the value
std::string value = "Hello world!";
writer.serialize<std::string>(value, 32U); // The second argument is the maximum size we expect the string to be

// Flush the writer's remaining state into the buffer
uint32_t num_bits = writer.flush();

// Create a reader, referencing the buffer and bits written
bit_reader reader(buffer, num_bits);

// Read the value back
std::string out_value; // The string will be resized if the output doesn't fit
reader.serialize<std::string>(out_value, 32U); // out_value should now contain "Hello world!"
```

Writing a float into the buffer with a bounded range and precision:
```cpp
// Create a writer, referencing the buffer and its size
byte_buffer<4> buffer;
bit_writer writer(buffer);

// Write the value
bounded_range range(1.0f, 4.0f, 1.0f / 128.0f); // Min, Max, Precision
float value = 1.2345678f;
writer.serialize<bounded_range>(range, value);

// Flush the writer's remaining state into the buffer
uint32_t num_bits = writer.flush();

// Create a reader, referencing the buffer and bits written
bit_reader reader(buffer, num_bits);

// Read the value back
float out_value;
reader.serialize<bounded_range>(range, out_value); // out_value should now be a value close to 1.2345678f
```

These examples can also be seen in [`src/test/examples_test.cpp`](https://github.com/KredeGC/BitStream/tree/master/src/test/examples_test.cpp).

# Extensibility
The library is made with extensibility in mind.
The `bit_writer` and `bit_reader` use a template trait specialization of the given type to deduce how to serialize and deserialize the object.
The only requirements of the trait is that it has (or can deduce) 2 static functions which take a `bit_writer&` and a `bit_reader&` respectively as their first argument.
The 2 functions must also return a bool indicating whether the serialization was a success or not, but can otherwise take any number of additional arguments.

## Adding new serializables types
The general structure of a trait looks like the following:

```cpp
template<>
struct serialize_traits<TRAIT_TYPE> // The type to use when referencing this specific trait
{
    // Will be called when writing the object to a stream
    static bool serialize(bit_writer& stream, ...)
    { ... }
    
    // Will be called when reading the object from a stream
    static bool serialize(bit_reader& stream, ...)
    { ... }
};
```

As with any functions, you are free to overload them if you want to serialize an object differently, depending on any parameters you pass.
As long as their list of parameters starts with `bit_writer&` and `bit_reader&` respectively they will be able to be called.

## Unified serialization
The serialization can also be unified with templating, if writing and reading look similar.
If some parts of the serialization process don't match entirely you can query the `Stream::reading` or `Stream::writing` and branch depending on the value.
An example of this can be seen below:
```cpp
template<>
struct serialize_traits<TRAIT_TYPE> // The type to use when serializing
{
    // Will be called when writing or reading the object to a stream
    template<typename Stream>
    static bool serialize(Stream& stream, ...)
    {
        // Some code that looks the same for writing and reading
        
        if constexpr (Stream::writing) {
            // Code that should only be run when writing
        }
        
        // A variable that differs if the stream is writing or reading
        int value = Stream::reading ? 0 : 1;
        
        ...
    }
};
```

## Partial trait specializations
The specialization can also be templated to work with a number of types.
It also works with `enable_if` as the second argument:
```cpp
// This trait will be used by any non-const integral pointer type (char*, uint16_t* etc.)
template<typename T>
struct serialize_traits<T*, typename std::enable_if_t<std::is_integral_v<T> && !std::is_const_v<T>>>
{ ... };
// An example which will use the above trait
bool status = writer.serialize<int16_t*>(...);
// An example which won't use it (and won't compile)
bool status = writer.serialize<const int16_t*>(...);
```

Note that `TRAIT_TYPE` does not necessarily have to be part of the serialize function definitions.
It can just be used to specify which trait to use when serializing, if it cannot be deduced from the arguments.<br/>
Below is an example where we serialize an object by explicitly defining the trait type:
```cpp
bool status = writer.serialize<TRAIT_TYPE>(...);
```

## Trait deduction
When calling the `serialize` function on a `bit_writer` or `bit_reader`, the trait can sometimes be deduced instead of being explicitly declared.
This can only be done if the type of the second argument in the `static bool serialize(...)` function is (roughly) the same as the trait type.
An example of the structure for an implicit trait can be seen below:
```cpp
template<>
struct serialize_traits<TRAIT_TYPE> // The type to use when referencing this specific trait
{
    // The second argument is the same as TRAIT_TYPE (const and lvalue references are removed when deducing)
    static bool serialize(bit_writer& stream, const TRAIT_TYPE&, ...)
    { ... }
    
    // The second argument is the same as TRAIT_TYPE (lvalue is removed)
    static bool serialize(bit_reader& stream, TRAIT_TYPE&, ...)
    { ... }
};
```

The above trait could then be used when implicitly serializing an object of type `TRAIT_TYPE`:
```cpp
TRAIT_TYPE value;
bool status = writer.serialize(value, ...);
```

It doesn't work on all types, and there is some guesswork involved relating to const qualifiers.
E.g. a trait of type `char` is treated the same as `const char&` and thus the call would be ambiguous if both had a trait specialization.
In case of ambiguity you will still be able to declare the trait explicitly when calling the `serialize` function.

More concrete examples of traits can be found in the [`traits/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/traits/) directory.

# Building and running tests
The tests require premake5 as build system.
Generating project files can be done by running:
```bash
# Linux
premake5 gmake2 --toolset=gcc
# Windows
premake5 vs2019 --toolset=msc
```

Afterwards the tests can be built using the command below:
```bash
premake5 build --config=(release | debug)
```

You can also run the tests using the command below, or simply run the binary located in `bin/{{config}}-{{platform}}-{{architecture}}`:
```bash
premake5 test --config=(release | debug)
```

# 3rd party
The library has no dependencies, but does build upon some code from the [NetStack](https://github.com/nxrighthere/NetStack) library by Stanislav Denisov, which is free to use, as per their [MIT license](https://github.com/nxrighthere/NetStack/blob/master/LICENSE).
The code in question is about quantizing floats and quaternions, which has simply been translated from C# into C++ for the purposes of this library.

If you do not wish to use float, half or quaternion quantization, you can simply remove the [`quantization/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/quantization/) directory from the library, in which case you will not need to include or adhere to the license for NetStack.

# License
The library is licensed under the [BSD-3-Clause license](https://github.com/KredeGC/BitStream/blob/master/LICENSE) and is subject to the terms and conditions in that license.
In addition to this, everything in the [`quantization/`](https://github.com/KredeGC/BitStream/tree/master/include/bitstream/quantization/) directory is subject to the [MIT license](https://github.com/nxrighthere/NetStack/blob/master/LICENSE) from [NetStack](https://github.com/nxrighthere/NetStack).