#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : enable

/** ======== Preprocessor Test ======== **/
// null directive
#
// preceded by spaces
    #
// followed by spaces and preceding the directive
#    define    MY_VALUE1    1
// Commonly used
#define     MY_VALUE2       14

// ATTENTION: ## operator is not supported by `glslangValidator` tool.
#define MY_CONSTANT_CONSTRUCT(a, b)      a ## b

#ifndef VULKAN
#error  VULKAN macro is not pre-defined!
#endif

#if !defined(MY_VALUE1) || !defined(MY_VALUE2)
#error Either MY_VALUE1 or MY_VALUE2 is not defined!
#endif

// 'MY_CONSTANT_VALUE' is used at line 34, so here `__LINE__` will substitute 34.
// The complete 'MY_CONSTANT_VALUE' used at line 34 will substitute 100.
// filename-based `__FILE__` required extension: `GL_GOOGLE_cpp_style_line_directive`
#define MY_CONSTANT_VALUE   ((MY_VALUE1 * __LINE__ - MY_VALUE2) * 5 +  __VERSION__ - 460)

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
const uint g_constants[8] = { 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U };

// Private variable
uint g_gtid = 0;

void main(void)
{
    g_gtid = gl_GlobalInvocationID.x;

    const uint constValue = g_constants[g_gtid & 7];

    const int constantValue = pushConstants.a + int(pushConstants.b) - 
        (pushConstants.secondConstant.x + pushConstants.secondConstant.y + pushConstants.secondConstant.z + pushConstants.secondConstant.w);
    const uint srcValue = srcBuffer[g_gtid] * (constValue - uint(spec_constant)) + constantValue;
    dstBuffer[g_gtid] = srcValue;
}

