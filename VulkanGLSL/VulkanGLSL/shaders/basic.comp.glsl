#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : enable

/** ======== Preprocessor Test ======== **/
// null directive
#
// preceded by spaces
    #
// followed by spaces and precedint the directive
#    define    MY_VALUE1    1
// Commonly used
#define     MY_VALUE6       6

// ## operator is not supported by `glslangValidator` tool.
#define MY_CONSTANT_CONSTRUCT(a, b)      a ## b

// 'MY_CONSTANT_VALUE' is used at line 26, so here `__LINE__` will substitute 26.
// The complete 'MY_CONSTANT_VALUE' used at line 26 will substitute 100.
// filename-based `__FILE__` required extension: `GL_GOOGLE_cpp_style_line_directive`
#define MY_CONSTANT_VALUE   ((MY_VALUE1 * __LINE__ - MY_VALUE6) * 5 +  __VERSION__ - 460)

layout(local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;

// This constant will be specialized when creating the compute pipeline.
// Its default value is 100.
layout(constant_id = 3) const int spec_constant = MY_CONSTANT_VALUE;

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
const uint g_constant = 2U;

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

