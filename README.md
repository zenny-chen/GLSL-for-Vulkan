# GLSL-for-Vulkan
Introduction to GLSL for Vulkan API

<br />

## 目录

- [概述](#overview)
    - [关于demo](#about_demo) 

<br />

## <a name="overview"></a> 概述

本文将对适用于 [Vulkan API](https://www.vulkan.org/) 的 [GLSL](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf) 进行介绍，包括其语法以及具体使用。本仓库所附含了完整的 demo 代码。

由于 Vulkan API 本身仅支持 [SPIR-V](https://www.khronos.org/spir/) 字节码作为其唯一指定的可编程着色语言，因此我们一般可以通过 [Vulkan SDK](https://www.vulkan.org/tools#download-these-essential-development-tools) 中附带的编译工具（**glslangValidator**）将 GLSL 源代码编译为最终的 **SPV** 字节码文件，作为 Vulkan API 着色器模块的输入。

这里需要说明的是，SPIR-V 是一个比较通用的、利于诸如 GPU 这种流处理器发挥庞大线程并行计算能力的中间语言。因此它并不仅仅针对 Vulkan，而且也支持 [OpenCL](https://www.khronos.org/opencl/)、甚至较新版本的 [OpenGL](https://www.khronos.org/opengl/)。因此，这里有些 SPIR-V 的语法特性或其语言能力（**capability**）并无法支持 Vulkan API，而可能仅针对 OpenCL 或是 OpenGL的。从而即便是用 GLSL，有些语法特性对于 Vulkan API 和 OpenGL 而言也有所差异。

[GL_KHR_vulkan_glsl](https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt) 此扩展描述了 GLSL 针对 Vulkan API 与原来的 OpenGL 之间的一些特性差异。比如，Vulkan API 移除了 OpenGL 中原本所支持的 **`AtomicCounter`**、**`Subroutines`** 等语法特性；而添加了 **push-constant buffers**、**specialization constants** 等语法特性。另外，**GL_KHR_vulkan_glsl** 这一扩展无需（也不能）直接添加在 GLSL 源代码中，它直接被 Vulkan SDK 中的 **glslangValidator** 工具所支持。

同 OpenGL 类似，GLSL 中所用到的部分特性可能不会被某些 GPU 实现所支持，它可能是 GLSL 的扩展。遇到这类特性，我们需要显式使用 **`#extension`** 进行声明，否则该特性可能将无法被正常使用。

下面列出针对 Vulkan API 的一些常用的 GLSL 扩展

- GLSL 扩展描述的官方首页：[GLSL](https://github.com/KhronosGroup/GLSL)
- [GL_KHR_vulkan_glsl](https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt)（其中包含对 **SPIR-V specialization constants** 的介绍：`layout(constant_id = 17) const int arraySize = 12;`；以及 **The built-in vector gl_WorkGroupSize can be specialized using special
    layout local\_size\_{xyz}\_id's applied to the "in" qualifier.**：`layout(local_size_x_id = 18, local_size_z_id = 19) in;`）（此feature无需在GLSL中使用 **`#extension`** 进行显式给出，在使用 **glslangValidator** 或 **glslc** 等工具编译时，会自动外加 **`VULKAN`** 这个宏，这将会默认开启此特征。）
- [GL_ARB_gpu_shader5](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_gpu_shader5.txt)（包含 **precise** 限定符）
- [GL_EXT_scalar_block_layout](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_scalar_block_layout.txt)（此扩展需要支持 [VK_EXT_scalar_block_layout](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_scalar_block_layout.html) 这一Vulkan扩展）
- [GL_EXT_shader_16bit_storage](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_16bit_storage.txt)（此扩展需要支持 [VK_KHR_16bit_storage](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_16bit_storage.html)  这一Vulkan扩展）
- [GL_EXT_shader_explicit_arithmetic_types](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)（此扩展需要支持 [VK_KHR_shader_float16_int8](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_shader_float16_int8.html) 这一Vulkan扩展）
- [GL_EXT_shader_atomic_int64](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_atomic_int64.txt)（此扩展需要 Vulkan 端支持 [VK_KHR_shader_atomic_int64](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_shader_atomic_int64.html)）
- [GL_EXT_shader_image_int64](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_shader_image_atomic_int64.html)（此扩展需要 Vulkan 端支持 [VK_EXT_shader_image_atomic_int64](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_shader_image_atomic_int64.html) 特征）
- [GL_ARB_gpu_shader_int64](https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_gpu_shader_int64.txt)（需要开启 Vulkan 端的 **`VkPhysicalDeviceFeatures::shaderInt64`** 特征）
- [GL_EXT_shader_atomic_float](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GLSL_EXT_shader_atomic_float.txt)（此扩展需要 Vulkan 端支持 [VK_EXT_shader_atomic_float](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_shader_atomic_float.html) 特征）
- [GL_ARB_shader_clock](https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shader_clock.txt)（此扩展需要 Vulkan 端支持 [VK_KHR_shader_clock](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_shader_clock.html) 特征）
- [GL_EXT_shader_realtime_clock](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_realtime_clock.txt)（此扩展需要 Vulkan 端支持 [VK_KHR_shader_clock](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_shader_clock.html) 特征）
- [GL_EXT_demote_to_helper_invocation](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GLSL_EXT_demote_to_helper_invocation.txt)（此扩展需要 Vulkan 端支持 [VK_EXT_shader_demote_to_helper_invocation](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_shader_demote_to_helper_invocation.html) 特征）

而 [Capabilities](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#spirvenv-capabilities) 这一 Vulkan spec 中关于 SPIR-V 能力的描述，详细列出了 Vulkan API 所能使用的 SPIR-V 能力特性。因此换句话说，没有在此表内列出的 SPIR-V 能力并且也没有在其下面 [SPIR-V Extensions](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#spirvenv-extensions) 扩展表中所列出的能力，则是 Vulkan API 所无法支持的。

本博文将主要针对 GLSL 4.6版本，结合 SPIR-V 1.3版本起进行对照描述，方便读者理解。而出于方便，GLSL 中也是主要针对其 **Compute Shader** 进行讲解，由于 **Compute Shader** 可支持大部分 GLSL 的语法特性，并且 demo 代码也更简洁一些。

<br />

#### <a name="about_demo"></a> 关于Demo

本仓库中的 demo 代码是基于 Windows 11 系统下，利用 Visual Studio 2022 Community Edition 开发完成。整个项目使用了 Windows 系统版本的 Vulkan SDK 1.3，并且使用了 **C17** 标准的C语言。各位只需安装以上所说的两个工具即可编译运行此代码。


