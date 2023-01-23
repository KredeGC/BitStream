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
    bitstream::quantization::bounded_range range(0.0f, 1.0f, 1.0f / 128.0f);
    
    // Bounded range
    float float_in = 0.68798f;
    uint32_t quantized = range.quantize(float_in);
    float float_out = range.dequantize(quantized);
    std::cout << range.get_bits_required() << '\n';
    std::cout << float_in << '\n';
    std::cout << quantized << '\n';
    std::cout << float_out << '\n';
    
    // Smallest three
    quaternion quat_in(0.0f, std::sin(2.0f), std::cos(2.0f), 0.0f);
    auto quantized_quat = bitstream::quantization::smallest_three::quantize<quaternion, 11>(quat_in);
    quaternion quat_out = bitstream::quantization::smallest_three::dequantize<quaternion, 11>(quantized_quat);
    std::cout << quat_in.w << ", " << quat_in.x << ", " << quat_in.y << ", " << quat_in.z << '\n';
    std::cout << quat_out.w << ", " << quat_out.x << ", " << quat_out.y << ", " << quat_out.z << '\n';

    // bit_writer
    uint8_t bytes[1024];
    bitstream::stream::bit_writer writer(bytes);

    uint32_t in_value = 42;
    writer.serialize_bits(in_value, 6);
    writer.serialize(range, float_in);

    writer.flush();

    // bit_reader
    bitstream::stream::bit_reader reader(bytes, 1024);

    uint32_t out_value;
    float float_out2;

    reader.serialize_bits(out_value, 6);
    reader.serialize(range, float_out2);
    
    return 0;
}