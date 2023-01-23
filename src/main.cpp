#include <bitstream/bitstream.h>

#include <iostream>

struct quaternion
{
    float w;
    float x;
    float y;
    float z;
    
    quaternion() = default;
    
    quaternion(float w, float x, float y, float z)
        : w(w), x(x), y(y), z(z) {}
};

int main(int argc, const char** argv)
{
    // Bounded range
    {
        bitstream::quantization::bounded_range range(0.0f, 1.0f, 1.0f / 128.0f);

        float float_in = 0.68798f;
        uint32_t quantized = range.quantize(float_in);
        float float_out = range.dequantize(quantized);
        std::cout << range.get_bits_required() << '\n';
        std::cout << float_in << '\n';
        std::cout << quantized << '\n';
        std::cout << float_out << '\n';
    }
    
    // Smallest three
    {
        quaternion quat_in(0.0f, std::sin(2.0f), std::cos(2.0f), 0.0f);
        auto quantized_quat = bitstream::quantization::smallest_three::quantize<quaternion, 11>(quat_in);
        quaternion quat_out = bitstream::quantization::smallest_three::dequantize<quaternion, 11>(quantized_quat);
        std::cout << quat_in.w << ", " << quat_in.x << ", " << quat_in.y << ", " << quat_in.z << '\n';
        std::cout << quat_out.w << ", " << quat_out.x << ", " << quat_out.y << ", " << quat_out.z << '\n';
    }

    // Writing & reading
    {
        bitstream::quantization::bounded_range range(0.0f, 1.0f, 1.0f / 128.0f);

        // protocol version
        uint32_t protocol_version = 0xDEADC0DE;

        // bit_writer
        uint8_t bytes[1024];
        bitstream::stream::bit_writer writer(bytes, 1024);

        writer.prepend_checksum();

        uint32_t in_u32 = 42;
        float in_float = 0.68798f;
        int16_t in_i16 = -28;

        writer.serialize_bits(in_u32, 6);
        writer.serialize(range, in_float);
        writer.serialize(in_i16, -32, 31);

        uint32_t num_bytes = writer.serialize_checksum(protocol_version);

        // bit_reader
        bitstream::stream::bit_reader reader(bytes, num_bytes);

        bool status = reader.serialize_checksum(protocol_version);

        uint32_t out_u32;
        float out_float;
        int16_t out_i16;

        reader.serialize_bits(out_u32, 6);
        reader.serialize(range, out_float);
        reader.serialize(out_i16, -32, 31);

#ifdef _WIN32
        system("pause");
#endif
    }
    
    return 0;
}