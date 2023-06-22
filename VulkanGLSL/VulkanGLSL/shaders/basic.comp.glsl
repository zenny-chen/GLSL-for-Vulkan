#version 450
#extension GL_EXT_shader_explicit_arithmetic_types : enable

layout(local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;

// This constant will be specialized when creating the compute pipeline.
// Its default value is 100.
layout(constant_id = 3) const int spec_constant = 100;

struct CustomPushConstants
{
    int a;
    uint b;
    uint paddings[2];
    ivec4 secondConstant;
};

layout(push_constant, std430) uniform PushConstants {
    layout(offset = 0) CustomPushConstants pushConstants;
};

// destination StorageBuffer
layout(std430, set = 0, binding = 0) buffer writeonly Dst {
    uint dstBuffer[];
};

// source StorageBuffer
layout(std430, set = 0, binding = 1) buffer readonly Src {
    uint srcBuffer[];
};

// Private constant
const uint g_constant = 2;

// Private variable
uint g_gtid = 0;

void main(void)
{
    g_gtid = gl_GlobalInvocationID.x;

    const int constantValue = pushConstants.a + int(pushConstants.b) - 
        (pushConstants.secondConstant.x + pushConstants.secondConstant.y + pushConstants.secondConstant.z + pushConstants.secondConstant.w);
    const uint srcValue = srcBuffer[g_gtid] * (g_constant - uint(spec_constant)) + constantValue;
    dstBuffer[g_gtid] = srcValue;
}

