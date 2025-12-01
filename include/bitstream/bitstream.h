#pragma once

// Possible defines
// #define BS_DEBUG_BREAK // Stops execution when serialization fails (debug break, trap etc.)
// #define BS_EQUAL_PATH // Stops optimization of the happy path

// Quantization
#include "quantization/bounded_range.h"
#include "quantization/half_precision.h"
#include "quantization/smallest_three.h"

// Stream
#include "stream/bit_measure.h"
#include "stream/bit_reader.h"
#include "stream/bit_writer.h"
#include "stream/byte_buffer.h"
#include "stream/serialize_traits.h"

// Traits
#include "traits/array_traits.h"
#include "traits/bool_trait.h"
#include "traits/checksum_trait.h"
#include "traits/enum_trait.h"
#include "traits/float_trait.h"
#include "traits/integral_traits.h"
#include "traits/quantization_traits.h"
#include "traits/string_traits.h"