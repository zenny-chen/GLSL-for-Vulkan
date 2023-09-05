#version 450

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable

layout(local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;
layout(constant_id = 3) const uint total_data_elem_count = 1024U;

layout(buffer_reference, std430, buffer_reference_align = 16) buffer DataBufferRef {
    int data[total_data_elem_count];
};

layout(buffer_reference, std430, buffer_reference_align = 4) buffer AtomicValueRef {
    uint atomValue;
};

layout(std430, set = 0, binding = 0) readonly buffer src {
    DataBufferRef srcWrapperBuffer[];
};

void main(void)
{
    const uint gid = gl_GlobalInvocationID.x;
    DataBufferRef dstRef = srcWrapperBuffer[0];
    DataBufferRef srcRef = srcWrapperBuffer[1];
    // Cast the reference type `DataBufferRef` to `AtomicValueRef`
    AtomicValueRef atomRef = AtomicValueRef(srcWrapperBuffer[2]);

    if (uint64_t(dstRef) == 0 || uint64_t(srcRef) == 0 || uint64_t(atomRef) == 0) return;

    const uint atomValue = atomicAdd(atomRef.atomValue, 1U);
    dstRef.data[gid] = srcRef.data[gid] | (int(atomValue) << 16);
}

