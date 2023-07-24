# GLSL-for-Vulkan
Introduction to GLSL for Vulkan API

<br />

## 目录

- [概述](#overview)
    - [关于demo](#about_demo)
    - [语言体系概述](#language_system_overview)
- [着色概述](#shading_overview)
    - [顶点处理器](#vertex_processor)
    - [细分曲面控制处理器](#tessellation_control_processor)
    - [细分曲面计算处理器](#tessellation_evaluation_processor)
    - [几何处理器](#geometry_processor)
    - [片段处理器](#fragment_processor)
    - [计算处理器](#compute_processor)
- [基本语法](#basic)
    - [预处理器（preprocessor） ](#preprocessor)
    - [注释（Comments）](#comments)
    - [定义](#definitions)
        - [静态使用](#static_use)
        - [动态均匀表达式与均匀控制流](#dynamic_uniform_expressions)
- [变量与类型](#variables_and_types)
- [基本类型](#basic_types)
    - [透明类型(Transparent Types)](#transparent_types)
    - [浮点隐含类型（Floating-Point Opaque Types）](#floating-point_opaque_types)
    - [无符号的整数隐含类型（Unsigned Integer Opaque Types）](#unsigned_integer_opaque_types)
    - [采样器隐含类型（Sampler Opaque Types）](#sampler_opaque_types)
    - [基于Vulkan API的GLSL新增一些隐含类型的具体操作](#vulkan_glsl_opaque_types_operations)
    - [用于 image 类型的格式限定符列表](#image_format_qualifiers)
    - [Void 类型](#type_void)
    - [布尔类型（Booleans）](#type_bool)
    - [整数（Integers）](#type_integers)
    - [浮点类型（Floats）](#type_float)
    - [向量（Vectors）](#type_vector)
    - [矩阵（Matrices）](#type_matrices)
    - [隐含类型（Opaque Types）](#opaque_types)
        - [与纹理结合的采样器（Texture-Combined Samplers）](#texture-combined_samplers)
        - [图像（Images）](#images)
        - [原子计数器（Atomic Counters）](#atomic_counters)
        - [纹理、采样器、以及 samplerShadow 类型（Texture, sampler, and samplerShadow Types）](#texture_sampler_and_samplerShadow_types)
        - [子遍输入（Subpass Inputs）](#subpass_inputs)
    - [结构体（Structures）](#structures)
    - [数组（Arrays）](#arrays)
    - [隐式转换（Implicit Conversions）](#implicit_conversions)
    - [初始化器（Initializers）](#initializers)
- [作用域（Scoping）](#scoping)
- [存储限定符（Storage Qualifiers）](#storage_qualifiers)

<br />

## <a name="overview"></a> 概述

本文将对适用于 [Vulkan API](https://www.vulkan.org/) 的 [GLSL](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf) 进行介绍，包括其语法以及具体使用。本仓库所附含了完整的 demo 代码。

由于 Vulkan API 本身仅支持 [SPIR-V](https://www.khronos.org/spir/) 字节码作为其唯一指定的可编程着色语言，因此我们一般可以通过 [Vulkan SDK](https://www.vulkan.org/tools#download-these-essential-development-tools) 中附带的编译工具（**glslangValidator**）将 GLSL 源代码编译为最终的 **SPV** 字节码文件，作为 Vulkan API 着色器模块的输入。

这里需要说明的是，SPIR-V 是一个比较通用的、利于诸如 GPU 这种流处理器发挥庞大线程并行计算能力的中间语言。因此它并不仅仅针对 Vulkan，而且也支持 [OpenCL](https://www.khronos.org/opencl/)、甚至较新版本的 [OpenGL](https://www.khronos.org/opengl/)。因此，这里有些 SPIR-V 的语法特性或其语言能力（**capability**）并无法支持 Vulkan API，而可能仅针对 OpenCL 或是 OpenGL的。从而即便是用 GLSL，有些语法特性对于 Vulkan API 和 OpenGL 而言也有所差异。本文也会涉及一些对 SPIR-V 的介绍。

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

<br />

#### <a name="language_system_overview"></a> 语言体系概述

上述已经提到，本文将针对 *The OpenGL Shading Language, version 4.60* 进行描述。用 GLSL 所编写的一个独立编译单元称为一个 **着色器**（***shader***）。而一个完整的 *程序* 由一组着色器进行编译，然后被连接（link）在一起，以完整地创建图形API流水线的一个或多个可编程阶段（**programmable stages**）。一单个可编程阶段的所有着色器必须在同一个程序内。一整套可编程阶段可以被放入到一单个程序，或是可以将这些阶段跨多个程序进行划分。

GLSL 从整个语法体系上来看就像是 [C++](https://en.cppreference.com/w/cpp/language/history) 98/03 的子集。它是强类型且类型安全的编译型语言。其中支持一些传统C++的语法特性，比如：**构造器（Constructors）**，**函数重载（function overloading）**，数组类型被封装为一个结构体类型，且能用 **`.`** 语法来访问 `length()` 方法等。而C语言所能支持的 **预处理器（Preprocessor）**、**代码注释（Comments）** 等均能支持。而 GLSL 还有其自身的语法，比如 **`layout`** 限定符、函数参数限定符、额外的存储限定符等等。当然，GLSL 也有许多C语言语法特性是不支持的。比如，不支持枚举类型（但 **`enum`** 作为保留关键字），不支持联合体（ **`union`**），不支持 **`sizeof`** 操作符（但 **`sizeof`** 作为保留关键字），不支持函数递归调用等。

<br />

## <a name="shading_overview"></a> 着色概述

OpenGL着色语言实际上分为几种紧密相关的语言。这些语言用于创建包含在API的处理流水线中的每个可编程处理器的着色器。当前，这些处理器有：**顶点（vertex）**、**细分曲面控制（tessellation control）**、**细分曲面计算（tessellation evaluation）**、**几何（geometry）**、**片段（fragment）**，以及 **计算（compute）** 处理器。

GLSL的基本语法对以上所有可编程处理器均通用，而只有少部分的 **`layout`** 属性、内建函数等只能用于一些特定的处理器着色器。

<br />

#### <a name="vertex_processor"></a> 顶点处理器

*顶点处理器*（*vertex processor*）是一个可编程单元，对正来临的顶点及其相关联的数据进行操作。在此处理器上所运行的用OpenGL着色语言编写的编译单元被称为 *顶点着色器*（*vertex shader*）。当一组顶点着色器被成功编译并被连接之后，它们最后产生一个 *顶点着色器可执行程序*（*vertex shader executable*）运行在顶点处理器上。

顶点处理器一次处理一个顶点。不过它不会取代一次需要了解多个顶点的图形操作。

<br />

#### <a name="tessellation_control_processor"></a> 细分曲面控制处理器

*细分曲面控制处理器*（*tessellation control processor*）是一个可编程单元，它处理正在来临的顶点及其相关联的数据的一个 patch，然后发射一个新的输出 patch。运行在此处理器上的用OpenGL着色器语言编写的编译单元被称为 *细分曲面控制着色器*（*tessellation control shaders*）。当一组细分曲面控制着色器被成功编译并连接之后，它们将产生一个 *细分曲面控制着色器可执行程序*（*tessellation control shader executable*）运行在细分曲面控制处理器上。

细分曲面控制着色器为输出 patch 的每个顶点进行调用。每个调用（invocation，即线程）可以读输入或输出 patch 的任一顶点的属性，但只能写对应输出 patch 顶点的逐顶点属性。着色器调用（线程）所有一起产生一组输出 patch 的逐 patch 属性。

当所有的细分曲面控制着色器调用（线程）完成之后，输出顶点与逐 patch 属性进行组装，以形成一个要被后续流水线阶段所使用的 patch。

细分曲面控制着色器调用（线程）通常几乎是独立运行的，具有未定义的相对执行次序。然而，内建函数 **`barrier()`** 通过同步调用可以被用于控制执行次序，有效地将细分曲面控制着色器执行划分为一组执行阶段（phases）。如果一个调用（线程）从一个逐顶点或逐 patch 的属性读，而该属性在此执行阶段（phase）期间内的任一点又被另一个调用（线程）写，或者是两个调用（线程）企图对同一逐 patch 输出32位分量、在一单个执行阶段（phase）去写不同的值，那么细分曲面控制着色器将会得到未定义的结果。

<br />

#### <a name="tessellation_evaluation_processor"></a> 细分曲面计算处理器

*细分曲面计算处理器*（*tessellation evaluation processor*）是一个可编程单元，通过使用正来临的顶点和它们所关联的数据，用于计算由细分曲面图元生成器所生成的一个顶点的位置和其他属性。运行在此处理器上的用OpenGL着色语言所写的编译单元被称为 *细分曲面计算着色器*（*tessellation evaluation shaders*）。当一组细分曲面计算着色器被成功编译并连接时，它们将产生一个 *细分曲面计算着色器可执行程序*（*tessellation evaluaton shader executable*），运行在细分曲面计算处理器上。

细分曲面计算可执行程序的每个调用（线程）计算由细分曲面图元生成器所生成的一单个顶点的位置和属性。可执行程序可以读输入 patch 中的任一顶点的属性，加上细分曲面坐标，细分曲面坐标是在被细分图元中顶点的相对位置。可执行程序写顶点的位置和其他属性。

<br />

#### <a name="geometry_processor"></a> 几何处理器

*几何处理器*（*geometry processor*）是一个可编程单元，对正来临的、顶点处理之后的已被组装好的一个图元的顶点数据进行操作，并输出形成输出图元的一串顶点。用OpenGL着色语言编写以运行此处理器的编译单元称为 *几何着色器*（*geometry shaders*）。当一组几何着色器被成功编译并连接时，它们生成一个 *几何着色器可执行程序*（*geometry shader executable*）运行在此几何处理器上。

几何处理器上的几何着色器可执行程序的一单个调用（线程）将对带有一组固定数量顶点的所声明的输入图元进行操作。此单个调用（线程）可以发射一组可变数量的顶点，它们被组装成一个所声明的输出图元类型的图元，并传递给后续流水线阶段。

<br />

#### <a name="fragment_processor"></a> 片段处理器

*片段处理器*（*fragment processor*）是一个可编程单元，对片段值以及与它们相关联的数据进行操作。用OpenGL着色语言所编写的、运行在此处理器上的编译单元称为 *片段着色器*（*fragment shader*）。当一组片段着色器被成功编译并连接时，它们产生一个 *片段着色器可执行程序*（*fragment shader executable*）运行在此片段处理器上。

一个片段着色器不能改变一个片段的 `(x, y)` 位置。而且也不允许访问邻近片段。由片段着色器所计算的值最终用于更新帧缓存存储器或纹理存储器，依赖于当前API的状态以及API命令，它们引发片段的生成。

<br />

#### <a name="compute_processor"></a> 计算处理器

*计算处理器*（*compute processor*）是一个可编程单元，对来自其他着色器处理器进行独立地操作。用OpenGL着色语言所编写的、运行在此处理器上的编译单元称为 *计算着色器*（*compute shader*）。当一组计算着色器被成功编译并连接时，它们产生一个 *计算着色器可执行程序*（*compute shader executable*）运行在计算处理器上。

一个计算着色器可以访问许多与片段着色器等其他着色器处理器一样的的资源，诸如纹理（textures）、缓存（buffers）、图像（image）变量，以及原子计数器（atomic counters）（基于 Vulkan 的 GLSL 以及 SPIR-V 则不支持原子计数器）。但它不具有固定功能输出。它并非图形流水线的一部分，并且其可见的副作用是通过对 images、storage buffers 以及 atomic counters（基于 Vulkan 的 GLSL 以及 SPIR-V 不支持此特征） 的改变而体现的。

一个计算着色器对一组工作项（work items）进行操作，该组工作项被称为一个 *workgroup*。一个 workgroup 是一堆执行同样代码的 shader invocations（线程）的合集，潜在地是并行执行的。一个 workgroup 中的一个 invocation（线程）可以与同一 workgroup 的其他成员（线程）共享数据，通过共享的（**`shared`**）变量，并发射存储器和控制流的栅栏（barriers）以与同一 workgroup 的其他成员（线程）进行同步。 

<br />

## <a name="basic"></a> 基本语法

之前已经提到过，GLSL 基于 C++98/03 的基础上做了一些扩充。当然，它仅仅支持C++的部分语法特性，像C++中的类、模板等高级语法特性，GLSL均不支持。不过大部分基础语法还是与C++共通的。比如其合法的标识符字符集包括：

- 字母 a到z，A到Z，以及下划线（_）。
- 数字 0到9。
- 操作符：句点（.），加号（+），减号（-），斜杠（/），星号（\*），百分号（%），尖括号（< 和 >），方括号（\[ 和 \]），圆括号（( 和 )），花括号（{ 和 }），插入符号（^），垂直竖条（|），与符号（&），波浪号（~），等号（=），惊叹号（!），冒号（:），分号（;），逗号（,）以及问号（?）。

如果GLSL源文件中出现除以上列出来的符号（token），那么会引发编译时错误。

此外，GLSL不包含双字符（digraphs），也不包含三字符（trigraphs）。另外，除了用作行接续字符之外，反斜杠（**`\`**）没有转义序列或其他用途。

GLSL没有字符或字符串数据类型，因此不包含受单引号或双引号包围的字符。

<br />

#### <a name="preprocessor"></a> 预处理器（preprocessor）

GLSL的预处理器与 C++98 所能支持的预处理器一样，而且尤其自己的扩展。不过上述已经提到，由于GLSL不支持字符类型，所以也不支持预处理器的 **`#`** 操作符，但原始的GLSL能支持 **`##`** 操作符做符号拼接，但 **glslangValidator** 工具则不支持此语法特性。

具体来说，GLSL支持以下列出的预处理指示符（preprocessor directives）：**`#`**（空预处理指示符），**`#define`**，**`#undef`**，**`#if`**，**`#ifdef`**，**`#ifndef`**，**`else`**，**`#elif`**，**`#endif`**，**`#error`**，**`#pragma`**，**`#extension`**，**`#version`**，**`#line`**。此外，在预处理器之中还能使用这些操作符：**`defined`**，**`##`**（**glslangValidator** 工具不支持此特性）。

每个 **井号**（**number sign**，**`#`**）可以被放在一行的开头，当然其前面也可以插入空格或水平制表符。它后面也可以放空格和水平制表符，而且可以放在指示符之前。每个指示符以一个换行符（new-line）结束。预处理并不改变一段源文件字符串的换行个数以及相对位置。预处理在换行被行连接字符移除之后发生。

一行中的井号（**`#`**）被其自己忽略。任何没有在上述所列出的指示符都将引发编译时错误。

以下都是合法的预处理器
```c
// null directive
#
// preceded by spaces
    #
// followed by spaces and preceding the directive
#    define    MY_VALUE1    1

// Commonly used
#define     MY_VALUE0       0

#define     MY_CONSTANT_VALUE   ((MY_VALUE1 + MY_VALUE0) * 100)
```

**`#define`** 和 **`#undef`** 的功能性与C++中的预处理器的标准定义得一样，用于带有或不带有宏参数的宏定义。

GLSL还可以使用这些预定义宏：**`__LINE__`**，**`__FILE__`**（基于Vulkan的GLSL默认不支持），**`__VERSION__`**。

**`__LINE__`** 将被替换为一个十进制整数常量，表示当前源文件中的行号。

对于基于OpenGL的GLSL，**`__FILE__`** 将被替换为一个十进制整数常量，用于说明当前正在处理哪个源代码字符串号（即GLSL源代码数组索引）。由于OpenGL中往往是在运行时（on the fly）去编译GLSL源代码字符串的。比如以下OpenGL API：
```cpp
void glShaderSource(GLuint shader,
                    GLsizei count,
                    const GLchar **string,
                    const GLint *length);
```
这里的参数 string 存放在一个或多个GLSL源代码字符串；length 则存放每个源代码字符串的长度。

而对于Vulkan而言，它接受的是 SPIR-V，而不是 GLSL。所以用于Vulkan的GLSL需要经过离线（offline）编译为spv文件，然后将此spv文件的内容送给Vulkan API生成着色器对象。因此对于Vulkan而言，一般来说是不需要使用 **`__FILE__`** 这个预定义宏。然而，Google有个扩展可使得 **`__FILE__`** 能得以使用——`GL_GOOGLE_cpp_style_line_directive`，不过当前能支持此扩展的 GLSL 实现估计并不多。

**`__VERSION__`** 将被替换为一个十进制整数，反映当前所使用的OpenGL着色语言的版本号（由 **`#version`** 声明）。如果GLSL源文件一开头声明了 **`#version 460`**，那 **`__VERSION__`** 这个值将会是460。

为了方便区分，GLSL对所有包含双下划线（**`__`**）的宏名进行保留，由底层软件层所使用。在一个着色器中去定义或取消定义这么一个宏名，其本身并不会导致一个错误，但可能会引发一些不可预期的行为，倘若后续还要使用源于与该宏同名的多个定义的话。所有以 **GL_** 打头的宏名（**GL** 后面跟着一条下划线）也是被保留的，而定义或取消定义这么一个宏名则将导致一个编译错误。

GLSL实现必须能支持最多长达 **1024** 个字符的宏名。同时也允许实现对于超出1024个字符的宏名生成一个错误，当然也允许实现能支持超过1024个字符的宏名。

**`#if`**、**`#ifdef`**、**`#ifndef`**、**`#else`**、**`#elif`**、**`#endif`** 所定义的操作如同C++标准中的预处理器一样，除了以下例外：

- 跟在 **`#if`** 和 **`#elif`** 后面的表达式被进一步限定为对字面量整数常量操作的表达式，外加由 **`defined`** 操作符所使用的标识符。
- 字符常量不被支持。

在预处理器中可用的操作符如下所示：

**优先级** | **操作符类别** | **操作符** | **结合性**
---- | ---- | ---- | ----
1（最高） | 圆括号分组 | **`()`** | NA
2 | 单目操作符 | **`defined`** <br /> **+** &ensp; &ensp; **-** &ensp; &ensp; **~** &ensp; &ensp; **!** | 从右到左
3 | 乘法操作符 | **\*** &ensp; &ensp; **/** &ensp; &ensp; **%** | 从左到右
4 | 加法操作符 | **+** &ensp; &ensp; **-** | 从左到右
5 | 按位移位 | **<<** &ensp; &ensp; **>>** | 从左到右
6 | 关系操作符 | **<** &ensp; &ensp; **>** &ensp; &ensp; **<=** &ensp; &ensp; **>=** | 从左到右
7 | 等号操作符 | **==** &ensp; &ensp; **!=** | 从左到右
8 | 按位与操作符 | **&** | 从左到右
9 | 按位异或操作符 | **^** | 从左到右
10 | 按位或操作符 | **\|** | 从左到右
11 | 逻辑与操作符 | **&&** | 从左到右
12（最低） | 逻辑或操作符 | **\|\|** | 从左到右

**`defined`** 操作符可被用于以下两种方式的其中一个：
```c
defined identifier
defined ( identifier )
```

对于基于OpenGL的GLSL来说，一个宏内的两个符号（token）可以使用符号连结操作符（**`##`**）拼接在也一起，就如C++预处理器的标准一样。但当前基于 **glslangValidator** 工具的GLSL则尚不支持此语法特性。

将操作符应用到预处理器中整数字面量的语义（比如上面提到的 **`defined`** 操作符，还有尚未被 **glslangValidator** 工具 所支持的 **`##`** 操作符等）匹配于C++预处理器中的那些标准，但未必适用于OpenGL着色语言（即GLSL的正式编译部分的源代码）。

预处理器表达式将根据主机端处理器的行为进行计算，而不是由着色器所指定的目标处理器。

**`#error`** 会使得GLSL实现将一个编译时的诊断消息放进着色器对象的信息日志中。该消息将是跟在 **`#error`** 指示符之后的符号，直到碰到一个换行符。GLSL实现必须将一个 **`#error`** 指示符的存在对待为一个编译时错误。

**`#pragma`** 允许依赖于实现的编译器控制。跟在 **`#pragma`** 后面的符号不受预处理宏扩展的影响。如果GLSL实现无法识别跟在 **`#pragma`** 后面的符号，那么实现将忽略此 pragma。以下列出了GLSL作为该语言的一部分所定义的 pragma。
```c
#pragma STDGL
```
**STDGL** 这一 pragma 被用于保留GLSL未来版本的使用。当前的GLSL实现不该使用首个符号为 **STDGL** 的 pragma。

```c
#pragma optimize(on)
#pragma optimize(off)
```
可以被用于开启或关闭优化，关闭优化能方便我们在开发过程中进行调式。这对 pragma 只能用于函数定义之外。默认情况下，优化对所有着色器都被自动打开的。而我们在使用基于 Vulkan 的GLSL时，可以对 **glslangValidator** 工具传递 **-Od** 命令行选项来禁用编译器的优化；而传递 **-Os** 来开启对 SPIR-V 的优化，并且最小化所生成的spv文件的尺寸。

调试 pragma：
```c
#pragma debug(on)
#pragma debug(off)
```
可以被用于允许对一个着色器的编译和注解带有调试信息，使得该着色器可以连上调试器来运行使用。而我们在使用基于 Vulkan 的GLSL时，可以对 **glslangValidator** 工具传递 **-g** 命令行选项以生成带有调试信息的spv文件。默认情况下，调试是被自动关闭的。

着色器应该声明它们所写的GLSL语言版本。一个着色器所使用的语言版本通过 **`#version`** 指示符进行指定：
```glsl
#version  number  profile_opt
```
而且大部分GLSL实现都要求将 **`#version`** 的声明放在GLSL源文件的一开始。这里，*number* 必须是GLSL语言的一个版本，遵循上述提到的 **`__VERSION__`** 相同的约定。比如，`#version 460` 指示符对于任一使用 4.60 GLSL版本的着色器而言是必须的。如果某个GLSL编译器不支持某个用 *number* 表示的GLSL语言版本，那么将引发一个编译器错误。对于版本1.10的GLSL，则不要求着色器必须包含此指示符，而不包含 **`#version`** 指示符的着色器将被对待为目标版本为1.10。指定了 `#version 100` 的着色器将被对待为目标版本为1.00的 OpenGL ES 着色语言。指定了 `#version 300` 的着色器将被对待为目标版本为3.00的 OpenGL ES 着色语言。指定了 `#version 310` 的着色器将被对待为目标版本为3.10的 OpenGL ES 着色语言。

如果提供了可选的 *profile* 实参，那么它必须是一个 OpenGL profile 的名字（因此对于基于 Vulkan 的GLSL而言，我们不需要指定此 *profile* 实参。）。当前有以下三种选择：
```glsl
core
compatibility
es
```
一个 *profile* 实参只能对于1.50版本或更高版本的GLSL进行使用。如果不提供 profile 实参，并且指定的版本为1.50或更高，那么默认为 **core**。如果指定了版本为300或310，那么 profile 实参 **必须不能** 是可选的，且必须指定为 **es**，否则将会引发编译时错误。**es** profile 的语言规格说明在《The OpenGL ES Shading Language specification》中描述。

声明了不同的 **core** 或 **compatibility** profile 的着色器可以被一起连接。然而，**es** profile 的着色器则无法用非 **es** profile 的着色器一起连接，而与同样带有 **es** profile 但具有不同版本的着色器进行连接也无法被一起连接，否则会产生一个连接时错误。

被指定为属于特定于 **compatibility** profile 的特征在 **core** profile 中是不可用的。当用于生成 SPIR-V 时，**compatibility** profile 的特征是不可用的。

对于GLSL实现所支持的每种 profile，都有一个内建的宏定义。所有实现提供了以下宏定义：
```c
#define GL_core_profile 1
```

提供了 **compatibility** profile 的GLSL实现提供了以下宏：
```c
#define GL_compatibility_profile 1
```

提供了 **es** profile 的GLSL实现提供了以下宏：
```c
#define GL_es_profile 1
```

**`#version`** 指示符必须发生在一个着色器中的任何事之前，除了注释和空白符。

默认情况下，GLSL语言的编译器对于并不遵循此规范的着色器必须发射编译时的词法和语法错误。任一扩展行为必须先被允许。用于控制编译器关于扩展行为的指示符用 **`#extension`** 指示符进行声明。

```glsl
#extension extension_name : behavior
#extension all : behavior
```

这里，*extension_name* 是一个扩展的名字。符号 **all** 意思是当前行为应用当前编译器所支持的所有扩展。*behavior* 可以是以下选项之一：

**行为** | **效果**
---- | ----
**require** | 由扩展 *extension_name* 所指定的行为。<br /> 如果 *extension_name* 扩展不受支持，或是指定了 **all**，那么将对 **`#extension`** 给出一个编译时错误。
**enable** | 由扩展 *extension_name* 所指定的行为。<br /> 如果 *extension_name* 扩展不受支持，那么将对 **`#extension`** 发出警告。<br /> 如果指定了 **all**，则给出一个编译时错误。
**warn** | 由扩展 *extension_name* 所指定的行为，除了对任一该扩展的可探测到的使用发出警告之外，除非这种使用受其他被允许或所要求的扩展支持。<br /> 如果指定了 **all**，那么对任一所使用的扩展的所有可探测到的使用都会发出警告。<br /> 如果扩展 *extension_name* 没被支持，则对 **`#extension`** 发出警告。
**disable** | 就好比扩展 *extension_name* 不是此语言定义的一部分的行为（包括发出错误和警告）。<br /> 如果指定了 **all**，那么行为必须恢复回正被编译的语言的非扩展核心版本。<br /> 如果扩展 *extension_name* 没被支持，则对 **`#extension`** 发出警告。

**extension** 指示符是一个简单的、底层的机制，用于设置每个扩展的行为。它并不定义诸如哪些结合是合适的等等，那些一定在其他地方定义。指示符的次序关系到每个扩展行为的设置：发生在后面的指示符覆盖更早看到的。**all** 变量对所有扩展设置了行为，覆盖所有先前发射的 **`extension`** 指示符，但仅针对 **warn** 和 **disable** 的 *行为*。

编译器的初始状态就好比用了以下指示符：
```glsl
#extension all : disable
```

即告诉编译器所报告的所有错误和警告必须根据此规格说明执行，而忽略任何扩展。

每个扩展可定义它所允许的作用域的粒度。如果没有任何说明，则粒度是整个着色器（也就是一单个编译单元），并且扩展指示符必须发生在任一非预处理符号之前。若有必要，连接器可以迫使大于一单个编译单元的粒度，这种情况下，每个所涉及到的着色器将不得不包含所需要的扩展指示符。

包含 **`#extension`** 和 **`#version`** 指示符的行不做宏扩展。

**`#line`** 在宏替换之后，必须具有以下形式之一：

```c
#line line
#line line source-string-number
```

这里，*line* 和 *source-string-number* 是常量整数表达式。如果这些常量表达式不是整数字面量，那么行为是未定义的。在处理了此指示符之后（包括其换行），实现行为则表现为好似它正在行号 *line* 以及源代码字符串号 *source-string-number* 处进行编译。后续的源代码字符串将按顺序进行编号，直到另一个 **`#line`** 指示符覆盖该编号。

当着色器针对 OpenGL SPIR-V 进行编译时，可用以下预定义宏：
```c
#define GL_SPIRV 100
```

当目标为 Vulkan 时，可用以下预定义宏：
```c
#define VULKAN 100
```

<br />

#### <a name="comments"></a> 注释（Comments）

GLSL中注释用 **`/*`** 和 **`*/`** 作为界限范围，或是用 **`//`** 加一个换行。起始注释定界符（**`/*`** 或 **`//`**）并不在一个注释内部被识别为注释定界符，因此注释是无法被嵌套的。一个 **`/*`** 包含了其终结界定符 **`*/`**。然而，一个 **`//`** 注释不包含（或消除）其终结的换行。

在注释内可以使用任一字节值，除了一个值为0的字节不能使用外。对于注释内容不会给出任何错误，并且对注释内容也无需进行校验。

通过行接续字符（**`\`**）逻辑上在注释被处理之前发生。也就是说，以行接续字符（**`\`**）结尾的一个单行注释包含了该注释中的下一行。
```c
// a single-line comment containing the next line \
a = b; // this is still in the first comment
```

<br />

#### <a name="definitions"></a> 定义

以下所描述的语言规则依赖于接下来的定义。

<br />

##### <a name="static_use"></a> 静态使用

一个着色器包含了对一个变量 x 的 *静态使用*（*static use*），如果在预处理之后，该着色器包含了一条语句将访问 x 的任一部分，无论控制流是否会致使该语句被执行。这么一个变量被引用为 *静态使用的*。如果该访问时一次写，那么 x 被进一步称为 *静态赋值的*。

<br />

##### <a name="dynamic_uniform_expressions"></a> 动态均匀表达式与均匀控制流

某些操作要求一个表达式需要是 *动态均匀的*（*dynamically uniform*），或者说，它应该在 *均匀控制流*（*uniform control flow*）内。这些要求被定义在以下定义集中。

一个 *调用*（*invocation*）（即，GPU中的线程）是一个特定流水线阶段中 *main()* 的一单个执行，仅仅对该阶段的着色器内所显式暴露的数据量进行操作。（任何对额外数据实例的隐式操作将由额外的调用（即线程）构成。）比如，在计算执行模型中，一单个调用（即线程）仅对一单个工作项（work item）进行操作；在一个顶点执行模型中，一单个调用（即线程）仅对一单个顶点进行操作。

一个 *调用组*（*invocation group*）是个完整的调用（线程）集。它们一起处理一个特定的工作组（workgroup）或是图形操作，这里一个“图形操作”的作用范围是依赖于实现的，但至少与一单个三角形或 patch 一样大，并且至少与一单个由客户端API所定义的渲染命令一样大。

在一单个调用（线程）内，一单条着色器语句可以被执行多次，若给出那条指令的多个 *动态实例*（*dynamic instances*）的话。当该指令在一个循环中执行、或是在一个函数中，该函数在多个点被调用，或是以上这些的多种组合时，这种情况就会发生。不同的循环迭代以及不同的动态函数调用点链产生了这么一条指令的不同动态实例。动态实例通过一个调用（即线程）内的它们（指这些动态实例）的控制流路径进行区别，而不是通过哪些调用（即线程）执行它。也就是说， *main()* 中的不同调用（即线程）执行一条指令相同的动态实例，当它们跟着同一条控制流路径走的时候。

SPIR-V 关于控制流指令的介绍可参考这里：[3.42.17. Control-Flow Instructions](https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#_control_flow_instructions)。

一个表达式对一个正在消费它的动态实例而言是 *动态均匀的*（*dynamically uniform*），当该表达式的值对于执行那个动态实例的（在当前调用组中的）所有的调用（即线程）都相同时。

*均匀控制流*（*Uniform control flow*）（或称为“收拢的”（**converged**）控制流）当调用组中所有的调用（即线程）执行相同控制流路径（并因而执行相同指令序列的动态实例）时发生。均匀控制流在 *main()* 入口处为初始状态，然后一直持续，直到一个带条件的分支为不同的调用（即线程）采用不同的控制路径（非均匀，或“岔开的”（**divergent**）的控制流）。这种分岔可以重新聚拢，将所有调用（即线程）再一次执行相同的控制流路径，而这重新建立了均匀控制流存在。如果控制流对于一个选择或循环的入口是均匀的，并且在调用组中的所有调用（即线程）后续离开那个选择或循环，那么控制流重新聚拢为均匀的。

比如：
```cpp
void main(void)
{
    float a = ...; // this is uniform control flow
    if (a < b) { // this expression is true for some fragments, not all
        ...; // non-uniform control flow
    }
    else {
        ...; // non-uniform control flow
    }
    ...; // uniform control flow again
}
```
注意，常量表达式是平凡动态均匀的。它遵循的是，基于常量表达式的典型的循环计数器也是动态均匀的。

<br />

## <a name="variables_and_types"></a> 变量与类型

所有变量和函数必须在被使用前进行声明。变量和函数名为标识符。

GLSL没有默认类型。所有变量和函数声明必须具有一个声明的类型，以及可选地具有限定符。一个变量通过指定其类型，后面跟着一个或多个用逗号分隔的名字进行声明。在许多情况下，一个变量可以通过使用赋值操作符（**=**）进行初始化，作为其声明的一部分。

用户自定义类型可以使用 **`struct`** 来定义，以聚合一组已存在的类型列表到一个名字中。

OpenGL着色语言是类型安全的。在类型之间存在一些隐式转换。这在何时发生、如何发生的确切描述在“[隐式转换](#implicit_conversions)”这小节中描写，并且在此规格说明的其他部分进行引用。

<br />

## <a name="basic_types"></a> 基本类型

**定义**：一个 *基本类型*（*basic type*）是一个由本语言的一个关键字所定义的类型。

OpenGL着色语言支持以下基本数据类型，如以下分组列出。

<br />

#### <a name="transparent_types"></a> 透明类型(Transparent Types)

**类型** | **含义** | 对应的 SPIR-V 类型 | SPIR-V 类型的描述
---- | ---- | ---- | ----
**`void`** | 用于不返回一个值的函数 | **`OpTypeVoid`** | 声明了 **`void`** 类型
**`bool`** | 一个条件类型，取值为 **`true`** 或 **`false`** | **`OpTypeBool`** | 声明了布尔类型。此类型的值只能取为 **`true`** 或 **`false`**。为这些值所定义的比特模式没有物理大小。如果它们被存储（与 **`OpVariable`** 连同使用），那么它们只能用逻辑寻址操作进行使用，而不能是物理的，并且只能与非外部可见的着色器存储类：**`Workgroup`**、**`CrossWorkgroup`**、**`Private`**、**`Function`**、**`Input`**、**`Output`**。
**`int`** | 一个带符号的整数 | %int = **`OpTypeInt`** 32 1 | **`OpTypeInt`** 声明了一个新的整数类型。<br /> *Width* 指定了该类型有多少比特的位宽。*Width* 是一个无符号32位整数。一个带符号的整数值是2的补码。<br /> *Signedness* 指定了是否有带符号的语义要保留或生效。<br /> **0** 指定了无符号数，或者说是无符号语义。<br /> **1** 指定了带符号语义。<br /> 在所有情况下，一条指令的操作类型取自于该指令的操作码，而不是操作数的符号性。
**`uint`** | 一个无符号整数 | %uint = **`OpTypeInt`** 32 0 | **`OpTypeInt`** 声明了一个整数类型。<br /> *Width* 指定了该类型有多少比特的位宽。*Width* 是一个无符号32位整数。一个带符号的整数值是2的补码。<br /> *Signedness* 指定了是否有带符号的语义要保留或生效。<br /> **0** 指定了无符号数，或者说是无符号语义。<br /> **1** 指定了带符号语义。<br /> 在所有情况下，一条指令的操作类型取自于该指令的操作码，而不是操作数的符号性。
**`float`** | 一个单精度浮点标量 | %float = **`OpTypeFloat`** 32 | **`OpTypeFloat`** 声明了一个新的浮点类型。<br /> *Width* 指定了该类型有多少比特的位宽。*Width* 是一个无符号32位整数。一个浮点值的比特模式由IEEE754标准所描述。
**`double`** | 一个双精度浮点标量 | %double = **`OpTypeFloat`** 64 | **`OpTypeFloat`** 声明了一个新的浮点类型。<br /> *Width* 指定了该类型有多少比特的位宽。*Width* 是一个无符号32位整数。一个浮点值的比特模式由IEEE754标准所描述。
**`vec2`** | 一个2分量的单精度浮点向量 | %v2float = **`OpTypeVector`** %float 2 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`vec3`** | 一个3分量的单精度浮点向量 | %v3float = **`OpTypeVector`** %float 3 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`vec4`** | 一个4分量的单精度浮点向量 | %v4float = **`OpTypeVector`** %float 4 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`dvec2`** | 一个2分量的双精度浮点向量 | %v2double = **`OpTypeVector`** %double 2 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`dvec3`** | 一个3分量的双精度浮点向量 | %v3double = **`OpTypeVector`** %double 3 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`dvec4`** | 一个4分量的双精度浮点向量 | %v4double = **`OpTypeVector`** %double 4 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`bvec2`** | 一个2分量的布尔向量 | N/A （可能替代：%v2uint = **`OpTypeVector`** %uint 2） | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`bvec3`** | 一个3分量的布尔向量 | N/A （可能替代：%v3uint = **`OpTypeVector`** %uint 3） | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`bvec4`** | 一个4分量的布尔向量 | N/A （可能替代：%v4int = **`OpTypeVector`** %int 4） | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`ivec2`** | 一个2分量的带符号整数向量 | %v2int = **`OpTypeVector`** %int 2 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`ivec3`** | 一个3分量的带符号整数向量 | %v3int = **`OpTypeVector`** %int 3 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`ivec4`** | 一个4分量的带符号整数向量 | %v4int = **`OpTypeVector`** %int 4 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`uvec2`** | 一个2分量的无符号整数向量 | %v2uint = **`OpTypeVector`** %uint 2 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`uvec3`** | 一个3分量的无符号整数向量 | %v3uint = **`OpTypeVector`** %uint 3 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`uvec4`** | 一个4分量的无符号整数向量 | %v4uint = OpTypeVector %uint 4 | **`OpTypeVector`** 声明了一个新的向量类型。*Component Type* 是结果类型的每个分量的类型。它必须是一个标量类型。<br /> *Component Count* 是结果类型中的分量的个数。*Component Count* 是一个无符号32位整数。它必须至少是2。<br /> 分量是从0开始，被依次编号的。
**`mat2`** | 一个 2 × 2 的单精度浮点矩阵 | %mat2v2float = **`OpTypeMatrix`** %v2float 2 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat3`** | 一个 3 × 3 的单精度浮点矩阵 | %mat3v3float = **`OpTypeMatrix`** %v3float 3 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat4`** | 一个 4 × 4 的单精度浮点矩阵 | %mat4v4float = **`OpTypeMatrix`** %v4float 4 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat2x2`** | 与 **`mat2`** 相同 | 与 **`mat2`** 相同 | 与 **`mat2`** 相同
**`mat2x3`** | 一个具有2列3行的单精度浮点矩阵 | %mat2v3float = **`OpTypeMatrix`** %v3float 2 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat2x4`** | 一个具有2列4行的单精度浮点矩阵 | %mat2v4float = **`OpTypeMatrix`** %v4float 2 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat3x2`** | 一个具有3列2行的单精度浮点矩阵 | %v3uint = **`OpTypeVector`** %uint 3 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat3x3`** | 与 **`mat3x3`** 相同 | 与 **`mat3x3`** 相同 | 与 **`mat3x3`** 相同
**`mat3x4`** | 一个具有3列4行的单精度浮点矩阵 | %mat3v4float = **`OpTypeMatrix`** %v4float 3 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat4x2`** | 一个具有4列2行的单精度浮点矩阵 | %mat4v2float = **`OpTypeMatrix`** %v2float 4 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat4x3`** | 一个具有4列3行的单精度浮点矩阵 | %mat4v3float = **`OpTypeMatrix`** %v3float 4 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`mat4x4`** | 与 **`mat4`** 相同 | 与 **`mat4`** 相同 | 与 **`mat4`** 相同
**`dmat2`** | 一个 2 × 2 的双精度浮点矩阵 | %mat2v2double = **`OpTypeMatrix`** %v2double 2 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat3`** | 一个 3 × 3 的双精度浮点矩阵 | %mat3v3double = **`OpTypeMatrix`** %v3double 3 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat4`** | 一个 4 × 4 的双精度浮点矩阵 | %mat4v4double = **`OpTypeMatrix`** %v4double 4 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat2x2`** | 与 **`dmat2`** 相同 | 与 **`dmat2`** 相同 | 与 **`dmat2`** 相同
**`dmat2x3`** | 一个具有2列3行的双精度矩阵 | %mat2v3double = **`OpTypeMatrix`** %v3double 2 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat2x4`** | 一个具有2列4行的双精度浮点矩阵 | %mat2v4double = **`OpTypeMatrix`** %v4double 2 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat3x2`** | 一个具有3列2行的双精度浮点矩阵 | %mat3v2double = **`OpTypeMatrix`** %v2double 3 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat3x3`** | 与 **`dmat3`** 相同 | 与 **`dmat3`** 相同 | 与 **`dmat3`** 相同
**`dmat3x4`** | 一个具有3列4行的双精度浮点矩阵 | %mat3v4double = **`OpTypeMatrix`** %v4double 3 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat4x2`** | 一个具有4列2行的双精度浮点矩阵 | %mat4v2double = **`OpTypeMatrix`** %v2double 4 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat4x3`** | 一个具有4列3行的双精度浮点矩阵 | %mat4v3double = **`OpTypeMatrix`** %v3double 4 | **`OpTypeMatrix`** 声明了一个新的矩阵类型。<br /> *Column Type* 是该矩阵中每一列的类型。它必须是向量类型。<br /> *Column Count* 是新矩阵类型中的列的个数。*Column Count* 是一个无符号32位整数。它必须至少是2。
**`dmat4x4`** | 与 **`dmat4`** 相同 | 与 **`dmat4`** 相同 | 与 **`dmat4`** 相同

注意以下表所说的“访问一个纹理”的地方，**`sampler*`** 隐含（**opaque**）类型访问一个指定类型的纹理，**`image*`** 隐含类型访问一个指定类型的图像。

<br />

#### <a name="floating-point_opaque_types"></a> 浮点隐含类型（Floating-Point Opaque Types）

**类型** | **含义** | 对应的 SPIR-V 类型 | SPIR-V 类型的描述
---- | ---- | ---- | ----
**`texture1D`** | 访问一个1D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float 1D 0 0 0 1 Unknown | **`OpTypeImage`** 声明了一个图像类型。比如，由 **`OpTypeSampledImage`** 消费。该类型是隐含的：此类型的值不具有所定义的物理大小或是比特模式。<br /> *Sampled Type* 是从该图像类型所采样或读取到的结果分量的类型。它必须是一个标量数值类型或是 **`OpTypeVoid`**。<br /> *Dim* 是图像的维度。<br /> 以下所有字面量都是整数，每个字面量作为一个操作数。<br /> *Depth* 用于判定该图像是否为一个深度图像。（注意，是否实际完成了深度比较是采样操作码的一个属性，而不是此类型声明的属性。）<br /> **0** 指示不是一个深度图像 <br /> **1** 指示是一个深度图像 <br /> **2** 意味着没有指示说明它是否为一个深度图像。<br /> *Arrayed* 必须是以下所指示的值之一：<br /> **0** 指示为非阵列内容 <br /> **1** 指示为阵列内容 <br /> *MS* 必须是以下所指示的值之一：<br /> **0** 指示单一样本内容 <br /> **1** 指示多重采样内容 <br /> *Sampled* 指示此图像是否与一个采样器结合进行访问，并且必须是以下值之一：<br /> **0** 指示这只能在运行时可知，而无法在编译时判定 <br /> **1** 指示一个图像与采样操作兼容 <br /> **2** 指示一个图像与读/写操作兼容（一个 storage 或是子遍（subpass）数据图像）。 <br /> *Image Format* 是图像格式，并且可以是 **Unknown**，由客户端API进行指定。<br /> 如果 *Dim* 是 **`SubpassData`**，那么 *Sampled* 必须是2，*Image Format* 必须是 **Unknown**，并且执行模式必须是 **片段**（**Fragment**）。
**`sampler1D`** | 访问一个1D纹理的一个句柄 | %20 = **`OpTypeImage`** %float 1D 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | **`OpTypeSampledImage`** 声明了一个被采样的图像类型、**`OpSampledImage`** 的 *Result Type*、或是外部绑定的采样器与图像。该类型是隐含的（opaque）：该类型的值没有所定义的物理大小或是比特模式。<br /> *Image Type* 必须是一个 **`OpTypeImage`**。它是在绑定的采样器与图像类型中的图像类型。它不允许含有一个 **`SubpassData`** 的 *Dim*。此外在1.6版本起，它不允许含有一个 **`Buffer`** 的 *Dim*。
**`image1D`** | 访问一个1D纹理的一个句柄 | **`OpTypeImage`** %float 1D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`sampler1DShadow`** | 访问一个带有比较的1D深度纹理 | %33 = **`OpTypeImage`** %float 1D 1 0 0 1 Unknown <br /> %34 = **`OpTypeSampledImage`** %33 | 见 **`OpTypeSampledImage`**
**`texture1DArray`** | 访问一个1D阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float 1D 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`sampler1DArray`** | 访问一个1D阵列纹理的一个句柄 | %33 = **`OpTypeImage`** %float 1D 0 1 0 1 Unknown <br /> %34 = **`OpTypeSampledImage`** %33 | 见 **`OpTypeSampledImage`**
**`image1DArray`** | 访问一个1D阵列纹理的一个句柄 | %41 = **`OpTypeImage`** %float 1D 0 1 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`sampler1DArrayShadow`** | 访问一个1D阵列纹理的一个句柄 | %36 = **`OpTypeImage`** %float 1D 1 1 0 1 Unknown <br /> %37 = **`OpTypeSampledImage`** %36 | 见 **`OpTypeSampledImage`**
**`texture2D`** | 访问一个2D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float 2D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`sampler2D`** | 访问一个2D纹理的一个句柄 | %20 = **`OpTypeImage`** %float 2D 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`image2D`** | 访问一个2D纹理的一个句柄 | %41 = **`OpTypeImage`** %float 2D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`sampler2DShadow`** | 访问一个带有比较的2D深度纹理的一个句柄 | %36 = **`OpTypeImage`** %float 2D 1 0 0 1 Unknown <br /> %37 = **`OpTypeSampledImage`** %36 | 见 **`OpTypeSampledImage`**
**`texture2DArray`** | 访问一个2D阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float 2D 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`sampler2DArray`** | 访问一个2D阵列纹理的一个句柄 | %20 = **`OpTypeImage`** %float 2D 0 1 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`image2DArray`** | 访问一个2D阵列纹理的一个句柄 | %42 = **`OpTypeImage`** %float 2D 0 1 0 2 Rgba8 | 见 **`OpTypeImage`**
**`sampler2DArrayShadow`** | 访问一个带有比较的2D阵列深度纹理的一个句柄 | %37 = **`OpTypeImage`** %float 2D 1 1 0 1 Unknown <br /> %38 = **`OpTypeSampledImage`** %37 | 见 **`OpTypeSampledImage`**
**`texture2DMS`** | 访问一个2D多重采样纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float 2D 0 0 1 1 Unknown | 见 **`OpTypeImage`**
**`sampler2DMS`** | 访问一个2D多重采样纹理的一个句柄 | %20 = **`OpTypeImage`** %float 2D 0 0 1 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`image2DMS`** | 访问一个2D多重采样纹理的一个句柄 | %44 = **`OpTypeImage`** %float 2D 0 0 1 2 <*Image Format*> | 见 **`OpTypeImage`**
**`texture2DMSArray`** | 访问一个2D多重采样阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float 2D 0 1 1 1 Unknown | 见 **`OpTypeImage`**
**`sampler2DMSArray`** | 访问一个2D多重采样阵列纹理的一个句柄 | %20 = **`OpTypeImage`** %float 2D 0 1 1 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`image2DMSArray`** | 访问一个2D多重采样阵列纹理的一个句柄 | %45 = **`OpTypeImage`** %float 2D 0 1 1 2 <*Image Format*> | 见 **`OpTypeImage`**
**`texture2DRect`** | 访问一个矩形纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float Rect 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`sampler2DRect`** | 访问一个矩形纹理的一个句柄 | %20 = **`OpTypeImage`** %float Rect 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`image2DRect`** | 访问一个矩形纹理的一个句柄 | %43 = **`OpTypeImage`** %float Rect 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`sampler2DRectShadow`** | 访问一个带有比较的矩形纹理 | %37 = **`OpTypeImage`** %float Rect 1 0 0 1 Unknown <br /> %38 = **`OpTypeSampledImage`** %37 | 见 **`OpTypeSampledImage`**
**`texture3D`** | 访问一个3D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float 3D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`sampler3D`** | 访问一个3D纹理的一个句柄 | %20 = **`OpTypeImage`** %float 3D 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`image3D`** | 访问一个3D纹理的一个句柄 | %42 = **`OpTypeImage`** %float 3D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`textureCube`** | 访问一个立方体贴图纹理（cube mapped texture）的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float Cube 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`samplerCube`** | 访问一个立方体贴图纹理（cube mapped texture）的一个句柄 | %20 = **`OpTypeImage`** %float Cube 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`imageCube`** | 访问一个立方体贴图纹理（cube mapped texture）的一个句柄 | %48 = **`OpTypeImage`** %float Cube 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`samplerCubeShadow`** | 访问一个带有比较的立方体贴图的深度纹理 | %37 = **`OpTypeImage`** %float Cube 1 0 0 1 Unknown <br /> %38 = **`OpTypeSampledImage`** %37 | 见 **`OpTypeSampledImage`**
**`textureCubeArray`** | 访问一个立方体贴图阵列纹理（cube map array texture）的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %float Cube 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`samplerCubeArray`** | 访问一个立方体贴图阵列纹理（cube map array texture）的一个句柄 | %20 = **`OpTypeImage`** %float Cube 0 1 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`imageCubeArray`** | 访问一个立方体贴图阵列纹理（cube map array texture）的一个句柄 | %42 = **`OpTypeImage`** %float Cube 0 1 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`samplerCubeArrayShadow`** | 访问一个带有比较的立方体贴图阵列深度纹理（cube map array depth texture）的一个句柄 | %37 = **`OpTypeImage`** %float Cube 1 1 0 1 Unknown <br /> %38 = **`OpTypeSampledImage`** %37 | 见 **`OpTypeSampledImage`**
**`textureBuffer`** | 访问一个缓存纹理的一个句柄（仅支持基于Vulkan的GLSL） | **`OpTypeImage`** %float Buffer 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`samplerBuffer`** | 访问一个缓存纹理的一个句柄 | %20 = **`OpTypeImage`** %float Buffer 0 0 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`imageBuffer`** | 访问一个缓存纹理的一个句柄 | %34 = **`OpTypeImage`** %float Buffer 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`subpassInput`** | 访问一个浮点子遍（subpass）输入的一个句柄（仅支持Vulkan中的片段着色器） | %13 = **`OpTypeImage`** %float SubpassData 0 0 0 2 Unknown | 见 **`OpTypeImage`**
**`subpassInputMS`** | 访问一个多重采样的浮点子遍（subpass）输入的一个句柄（仅支持Vulkan中的片段着色器） | %13 = **`OpTypeImage`** %float SubpassData 0 0 1 2 Unknown | 见 **`OpTypeImage`**

<br />

#### <a name="signed_integer_opaque_types"></a> 带符号的整数隐含类型（Signed Integer Opaque Types）

**类型** | **含义** | 对应的 SPIR-V 类型 | SPIR-V 类型的描述
---- | ---- | ---- | ----
**`itexture1D`** | 访问一个带符号整数1D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %21 = **`OpTypeImage`** %int 1D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`isampler1D`** | 访问一个带符号整数1D纹理的一个句柄 | %21 = **`OpTypeImage`** %int 1D 0 0 0 1 Unknown <br /> %29 = **`OpTypeSampledImage`** %21 | 见 **`OpTypeSampledImage`**
**`iimage1D`** | 访问一个带符号整数1D纹理的一个句柄 | %39 = **`OpTypeImage`** %int 1D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itexture1DArray`** | 访问一个带符号整数1D阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int 1D 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`isampler1DArray`** | 访问一个带符号整数1D阵列纹理的一个句柄 | %20 = **`OpTypeImage`** %int 1D 0 1 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimage1DArray`** | 访问一个带符号整数1D阵列纹理的一个句柄 | %43 = **`OpTypeImage`** %int 1D 0 1 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itexture2D`** | 访问一个带符号整数2D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int 2D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`isampler2D`** | 访问一个带符号整数2D纹理的一个句柄 | %20 = **`OpTypeImage`** %int 2D 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimage2D`** | 访问一个带符号整数2D纹理的一个句柄 | %43 = **`OpTypeImage`** %int 2D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itexture2DArray`** | 访问一个带符号整数2D阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int 2D 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`isampler2DArray`** | 访问一个带符号整数2D阵列纹理的一个句柄 | %20 = **`OpTypeImage`** %int 2D 0 1 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimage2DArray`** | 访问一个带符号整数2D阵列纹理的一个句柄 | %44 = **`OpTypeImage`** %int 2D 0 1 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itexture2DMS`** | 访问一个带符号整数2D多重采样纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int 2D 0 0 1 1 Unknown | 见 **`OpTypeImage`**
**`isampler2DMS`** | 访问一个带符号整数2D多重采样纹理的一个句柄 | %20 = **`OpTypeImage`** %int 2D 0 0 1 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimage2DMS`** | 访问一个带符号整数2D多重采样纹理的一个句柄 | %43 = **`OpTypeImage`** %int 2D 0 0 1 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itexture2DMSArray`** | 访问一个带符号整数2D多重采样阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int 2D 0 1 1 1 Unknown | 见 **`OpTypeImage`**
**`isampler2DMSArray`** | 访问一个带符号整数2D多重采样阵列纹理的一个句柄 | %20 = **`OpTypeImage`** %int 2D 0 1 1 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimage2DMSArray`** | 访问一个带符号整数2D多重采样阵列纹理的一个句柄 | %44 = **`OpTypeImage`** %int 2D 0 1 1 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itexture2DRect`** | 访问一个带符号整数2D矩形纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int Rect 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`isampler2DRect`** | 访问一个带符号整数2D矩形纹理的一个句柄 | %20 = **`OpTypeImage`** %int Rect 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimage2DRect`** | 访问一个带符号整数2D矩形纹理的一个句柄 | %42 = OpTypeImage %int Rect 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itexture3D`** | 访问一个带符号整数3D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int 3D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`isampler3D`** | 访问一个带符号整数3D纹理的一个句柄 | %20 = **`OpTypeImage`** %int 3D 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimage3D`** | 访问一个带符号整数3D纹理的一个句柄 | %44 = **`OpTypeImage`** %int 3D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itextureCube`** | 访问一个带符号整数立方体贴图的纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int Cube 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`isamplerCube`** | 访问一个带符号整数立方体贴图的纹理的一个句柄 | %20 = **`OpTypeImage`** %int Cube 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimageCube`** | 访问一个带符号整数立方体贴图的纹理的一个句柄 | %43 = **`OpTypeImage`** %int Cube 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itextureCubeArray`** | 访问一个带符号整数立方体贴图的阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int Cube 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`isamplerCubeArray`** | 访问一个带符号整数立方体贴图的阵列纹理的一个句柄 | %20 = **`OpTypeImage`** %int Cube 0 1 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimageCubeArray`** | 访问一个带符号整数立方体贴图的阵列纹理的一个句柄 | %44 = **`OpTypeImage`** %int Cube 0 1 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`itextureBuffer`** | 访问一个带符号整数缓存纹理的一个句柄（仅支持基于Vulkan的GLSL） | %20 = **`OpTypeImage`** %int Buffer 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`isamplerBuffer`** | 访问一个带符号整数缓存纹理的一个句柄 | %20 = **`OpTypeImage`** %int Buffer 0 0 0 1 Unknown <br /> %28 = **`OpTypeSampledImage`** %20 | 见 **`OpTypeSampledImage`**
**`iimageBuffer`** | 访问一个带符号整数缓存纹理的一个句柄 | %39 = **`OpTypeImage`** %int Buffer 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`isubpassInput`** | 访问一个带符号整数子遍（subpass）输入的一个句柄（仅支持Vulkan中的片段着色器） | %14 = **`OpTypeImage`** %int SubpassData 0 0 0 2 Unknown | 见 **`OpTypeImage`**
**`isubpassInputMS`** | 访问一个多重采样的带符号整数子遍（subpass）输入的一个句柄（仅支持Vulkan中的片段着色器） | %14 = **`OpTypeImage`** %int SubpassData 0 0 1 2 Unknown | 见 **`OpTypeImage`**

<br />

#### <a name="unsigned_integer_opaque_types"></a> 无符号的整数隐含类型（Unsigned Integer Opaque Types）

**类型** | **含义** | 对应的 SPIR-V 类型 | SPIR-V 类型的描述
---- | ---- | ---- | ----
**`utexture1D`** | 访问一个无符号整数1D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint 1D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`usampler1D`** | 访问一个无符号整数1D纹理的一个句柄 | %19 = **`OpTypeImage`** %uint 1D 0 0 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimage1D`** | 访问一个无符号整数1D纹理的一个句柄 | %40 = **`OpTypeImage`** %uint 1D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utexture1DArray`** | 访问一个无符号整数1D阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint 1D 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`usampler1DArray`** | 访问一个无符号整数1D阵列纹理的一个句柄 | %19 = **`OpTypeImage`** %uint 1D 0 1 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimage1DArray`** | 访问一个无符号整数1D阵列纹理的一个句柄 | %43 = **`OpTypeImage`** %uint 1D 0 1 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utexture2D`** | 访问一个无符号整数2D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint 2D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`usampler2D`** | 访问一个无符号整数2D纹理的一个句柄 | %19 = **`OpTypeImage`** %uint 2D 0 0 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimage2D`** | 访问一个无符号整数2D纹理的一个句柄 | %43 = **`OpTypeImage`** %uint 2D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utexture2DArray`** | 访问一个无符号整数2D阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint 2D 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`usampler2DArray`** | 访问一个无符号整数2D阵列纹理的一个句柄 | %19 = **`OpTypeImage`** %uint 2D 0 1 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimage2DArray`** | 访问一个无符号整数2D阵列纹理的一个句柄 | %44 = **`OpTypeImage`** %uint 2D 0 1 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utexture2DMS`** | 访问一个无符号整数2D多重采样纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint 2D 0 0 1 1 Unknown | 见 **`OpTypeImage`**
**`usampler2DMS`** | 访问一个无符号整数2D多重采样纹理的一个句柄 | %19 = **`OpTypeImage`** %uint 2D 0 0 1 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimage2DMS`** | 访问一个无符号整数2D多重采样纹理的一个句柄 | %43 = **`OpTypeImage`** %uint 2D 0 0 1 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utexture2DMSArray`** | 访问一个无符号整数2D多重采样阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint 2D 0 1 1 1 Unknown | 见 **`OpTypeImage`**
**`usampler2DMSArray`** | 访问一个无符号整数2D多重采样阵列纹理的一个句柄 | %19 = **`OpTypeImage`** %uint 2D 0 1 1 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeImage`**
**`uimage2DMSArray`** | 访问一个无符号整数2D多重采样阵列纹理的一个句柄 | %44 = **`OpTypeImage`** %uint 2D 0 1 1 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utexture2DRect`** | 访问一个无符号整数矩形纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint Rect 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`usampler2DRect`** | 访问一个无符号整数矩形纹理的一个句柄 | %19 = **`OpTypeImage`** %uint Rect 0 0 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimage2DRect`** | 访问一个无符号整数矩形纹理的一个句柄 | %42 = **`OpTypeImage`** %uint Rect 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utexture3D`** | 访问一个无符号整数3D纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint 3D 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`usampler3D`** | 访问一个无符号整数3D纹理的一个句柄 | %19 = **`OpTypeImage`** %uint 3D 0 0 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimage3D`** | 访问一个无符号整数3D纹理的一个句柄 | %44 = **`OpTypeImage`** %uint 3D 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utextureCube`** | 访问一个无符号整数立方体贴图的纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint Cube 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`usamplerCube`** | 访问一个无符号整数立方体贴图的纹理的一个句柄 | %19 = **`OpTypeImage`** %uint Cube 0 0 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimageCube`** | 访问一个无符号整数立方体贴图的纹理的一个句柄 | %42 = **`OpTypeImage`** %uint Cube 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`utextureCubeArray`** | 访问一个无符号整数立方体贴图阵列纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint Cube 0 1 0 1 Unknown | 见 **`OpTypeImage`**
**`usamplerCubeArray`** | 访问一个无符号整数立方体贴图阵列纹理的一个句柄 | %19 = **`OpTypeImage`** %uint Cube 0 1 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimageCubeArray`** | 访问一个无符号整数立方体贴图阵列纹理的一个句柄 | %43 = **`OpTypeImage`** %uint Cube 0 1 0 2 Rgba8ui | 见 **`OpTypeImage`**
**`utextureBuffer`** | 访问一个无符号整数缓存纹理的一个句柄（仅支持基于Vulkan的GLSL） | %19 = **`OpTypeImage`** %uint Buffer 0 0 0 1 Unknown | 见 **`OpTypeImage`**
**`usamplerBuffer`** | 访问一个无符号整数缓存纹理的一个句柄 | %19 = **`OpTypeImage`** %uint Buffer 0 0 0 1 Unknown <br /> %27 = **`OpTypeSampledImage`** %19 | 见 **`OpTypeSampledImage`**
**`uimageBuffer`** | 访问一个无符号整数缓存纹理的一个句柄 | %39 = **`OpTypeImage`** %uint Buffer 0 0 0 2 <*Image Format*> | 见 **`OpTypeImage`**
**`atomic_uint`** | 访问一个无符号整数原子计数器的一个句柄（仅支持基于OpenGL的GLSL） |  |  
**`usubpassInput`** | 访问一个无符号整数的子遍（subpass）输入的一个句柄（仅支持Vulkan中的片段着色器） | %14 = **`OpTypeImage`** %uint SubpassData 0 0 0 2 Unknown | 见 **`OpTypeImage`**
**`usubpassInputMS`** | 访问一个多重采样的无符号整数子遍（subpass）输入的一个句柄（仅支持Vulkan中的片段着色器） | %14 = **`OpTypeImage`** %uint SubpassData 0 0 1 2 Unknown | 见 **`OpTypeImage`**

<br />

#### <a name="sampler_opaque_types"></a> 采样器隐含类型（Sampler Opaque Types）

**类型** | **含义** | 对应的 SPIR-V 类型 | SPIR-V 类型的描述
---- | ---- | ---- | ----
**`sampler`** | 访问描述如何采样一个纹理状态的一个句柄（仅支持基于Vulkan的GLSL） | **`OpTypeSampler`** | 声明了采样器类型，由 **`OpSampledImage`** 消费。此类型是隐含的（opaque）：此类型的值不具有所定义的物理大小，也没有比特模式。
**`samplerShadow`** | 访问描述如何采样一个带有比较的深度纹理的一个句柄（仅支持基于Vulkan的GLSL） | **`OpTypeSampler`** | 声明了采样器类型，由 **`OpSampledImage`** 消费。此类型是隐含的（opaque）：此类型的值不具有所定义的物理大小，也没有比特模式。

<br />

#### <a name="vulkan_glsl_opaque_types_operations"></a> 基于Vulkan API的GLSL新增一些隐含类型的具体操作

上表中列出了仅针对 Vulkan API 所支持的新增 GLSL 语法特性的隐含类型，比如：**`texture1D`**、**`textureCube`**、**`textureBuffer`**、**`sampler`**、**`samplerShadow`** 等。因此，这里对于 GLSL 的内建函数 **`texelFetch`** 也增加了直接对上述纹理类型的直接操作（通过开启 **GL_EXT_samplerless_texture_functions** 这一扩展）；当然也可以通过具体 sampler 的构造，通过传统的 **`texelFetch`** 或 **`texture`** 内建函数对上述纹理类型进行操作。比如以下代码：

```glsl
#version 460
#extension GL_EXT_samplerless_texture_functions : enable

layout(set = 0, binding = 2) uniform texture1D myTex1D;
layout(set = 0, binding = 3) uniform textureCube myTexCube;
layout(set = 0, binding = 4) uniform textureBuffer myTexBuffer;
layout(set = 0, binding = 5) uniform sampler mySampler;

void main(void)
{
    // 通过 texelFetch 直接访问 myTex1D 的 texel
    vec4 value = texelFetch(myTex1D, 0, 0);

    // 通过构造 sampler1D 来访问 myTex1D 的 texel
    value += texelFetch(sampler1D(myTex1D, mySampler), 0, 0);

    // texelFetch 没有 textureCube 类型的变种，因此只能使用 texture 函数进行采样
    // 这里需要通过构造 samplerCube 用 texture 内建函数来访问 myTexCube 的 texel
    value += texture(samplerCube(myTexCube, mySampler), vec3(0.0f, 0.0f, 0.0f));

    // 通过 texelFetch 直接访问 myTexBuffer 的数据
    value += texelFetch(myTexBuffer, 0);

    // 通过构造 samplerBuffer 来访问 myTexBuffer 的数据
    value += texelFetch(samplerBuffer(myTexBuffer, mySampler), 0);
}
```

<br />

#### <a name="image_format_qualifiers"></a> 用于 image 类型的格式限定符列表

以下列表列出了用于修饰 **image** 隐含类型的由GLSL所支持的全部格式限定符。

GLSL image 类型格式 | **`layout`** image 类型 | 对应的 SPIR-V 格式 <br /> （用于 **`OpTypeImage`** 中的 *Image Format*） | SPIR-V 中所需要开启的能力
---- | ---- | ---- | ----
rgba32f | 浮点 **`layout`** **image** 类型 | Rgba32f | Shader
rgba16f | 浮点 **`layout`** **image** 类型 | Rgba16f | Shader
rg32f | 浮点 **`layout`** **image** 类型 | Rg32f | StorageImageExtendedFormats
rg16f | 浮点 **`layout`** **image** 类型 | Rg16f | StorageImageExtendedFormats
r11f_g11f_b10f | 浮点 **`layout`** **image** 类型 | R11fG11fB10f | StorageImageExtendedFormats
r32f | 浮点 **`layout`** **image** 类型 | R32f | Shader
r16f | 浮点 **`layout`** **image** 类型 | R16f | StorageImageExtendedFormats
rgba16 | 浮点 **`layout`** **image** 类型 | Rgba16 | StorageImageExtendedFormats
rgb10_a2 | 浮点 **`layout`** **image** 类型 | Rgb10A2 | StorageImageExtendedFormats
rgba8 | 浮点 **`layout`** **image** 类型 | Rgba8 | Shader
rg16 | 浮点 **`layout`** **image** 类型 | Rg16 | StorageImageExtendedFormats
rg8 | 浮点 **`layout`** **image** 类型 | Rg8 | StorageImageExtendedFormats
r16 | 浮点 **`layout`** **image** 类型 | R16 | StorageImageExtendedFormats
r8 | 浮点 **`layout`** **image** 类型 | R8 | StorageImageExtendedFormats
rgba16_snorm | 浮点 **`layout`** **image** 类型 | Rgba16Snorm | StorageImageExtendedFormats
rgba8_snorm | 浮点 **`layout`** **image** 类型 | Rgba8Snorm | Shader
rg16_snorm | 浮点 **`layout`** **image** 类型 | Rg16Snorm | StorageImageExtendedFormats
rg8_snorm | 浮点 **`layout`** **image** 类型 | Rg8Snorm | StorageImageExtendedFormats
r16_snorm | 浮点 **`layout`** **image** 类型 | R16Snorm | StorageImageExtendedFormats
r8_snorm | 浮点 **`layout`** **image** 类型 | R8Snorm | StorageImageExtendedFormats
rgba32i | 带符号整数 **`layout`** **image** 类型 | Rgba32i | Shader
rgba16i | 带符号整数 **`layout`** **image** 类型 | Rgba16i | Shader
rgba8i | 带符号整数 **`layout`** **image** 类型 | Rgba8i | Shader
rg32i | 带符号整数 **`layout`** **image** 类型 | Rg32i | StorageImageExtendedFormats
rg16i | 带符号整数 **`layout`** **image** 类型 | Rg16i | StorageImageExtendedFormats
rg8i | 带符号整数 **`layout`** **image** 类型 | Rg8i | StorageImageExtendedFormats
r32i | 带符号整数 **`layout`** **image** 类型 | R32i | Shader
r16i | 带符号整数 **`layout`** **image** 类型 | R16i | StorageImageExtendedFormats
r8i | 带符号整数 **`layout`** **image** 类型 | R8i | StorageImageExtendedFormats
rgba32ui | 无符号整数 **`layout`** **image** 类型 | Rgba32ui | Shader
rgba16ui | 无符号整数 **`layout`** **image** 类型 | Rgba16ui | Shader
rgb10_a2ui | 无符号整数 **`layout`** **image** 类型 | Rgb10a2ui | StorageImageExtendedFormats
rgba8ui | 无符号整数 **`layout`** **image** 类型 | Rgba8ui | Shader
rg32ui | 无符号整数 **`layout`** **image** 类型 | Rg32ui | StorageImageExtendedFormats
rg16ui | 无符号整数 **`layout`** **image** 类型 | Rg16ui | StorageImageExtendedFormats
rg8ui | 无符号整数 **`layout`** **image** 类型 | Rg8ui | StorageImageExtendedFormats
r32ui | 无符号整数 **`layout`** **image** 类型 | R32ui | Shader
r16ui | 无符号整数 **`layout`** **image** 类型 | R16ui | StorageImageExtendedFormats
r8ui | 无符号整数 **`layout`** **image** 类型 | R8ui | StorageImageExtendedFormats

【注：上述列表中关于浮点 **`layout`** **image** 类型，如果该类型后面含有 ***f*** 字母后缀，那说明该类型数据在主机端和GPU设备端均是以浮点数的形式给出的。如果该类型后面没有后缀 ***f***，且没有后缀 ***_snorm***，那说明该类型在主机端是用8位或16位整数存储的，而在GPU设备端会被实现转换为 [0.0, 1.0] 区间范围内的规格化浮点数，其实也就相当于是 **unorm** 类型，即无符号规格化为单精度浮点数类型。而带有 ***s_norm*** 后缀的，则表示该类型数据在主机端是用8位或16位存储的，而在GPU设备端则会被实现转换为 [-1.0, 1.0] 区间范围内的浮点数，这也就意味着它是带符号规格化为单精度浮点数类型。】

<br />

此外，一个着色器可以使用数组和结构体来聚合这些基本类型，用以构建更复杂的类型。

**GLSL没有指针类型**。但基于 Vulkan API 的GLSL通过开启 **[GL_EXT_buffer_reference](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GLSL_EXT_buffer_reference.txt)** 这一扩展，能在GLSL中使用对 **`buffer`** 块的引用。

本规格说明中，一个 *聚合*（*aggregate*）将意指一个结构体或一个数组。（矩阵(**matrix**)和向量（**vector**）其本身不是聚合。）聚合、矩阵和向量统称为 *组合*（*composites*）。

<br />

#### <a name="type_void"></a> `void` 类型

不返回一个值的函数必须被声明为 **`void`**。GLSL不存在默认的函数返回类型。关键字 **`void`** 不能在其他声明中使用（除非用于空的、正式的或实际形参列表，比如：`void main(void) { }`。），否则会引发编译时错误。

<br />

#### <a name="type_bool"></a> 布尔类型（Booleans）

**定义**：一个 *布尔类型*（*boolean type*）是任一布尔标量或向量类型（**`bool`**，**`bvec2`**，**`bvec3`**，**`bvec4`**）。

为了使得代码的条件执行更易于表达，**`bool`** 类型受到支持。当然也不能期待硬件能直接对此类型的变量予以支持。它是一个真实的布尔类型，仅保持着要么“真”、要么“假”这两种值的其中之一。两个关键字 **`true`** 和 **`false`** 可以被用作为布尔常量字面量。布尔变量可以如以下例子那样进行声明并可选地进行初始化：

```glsl
bool success;    // 将 "success" 声明为一个布尔变量
bool done = false;    // 声明并初始化 "done"
```

对于带条件的跳转所使用的表达式（**`if`**，**`for`**，**`?:`**，**`while`**，**`do-while`**）必须被计算为 **`bool`** 类型。

<br />

#### <a name="type_integers"></a> 整数（Integers）

**定义**：

- 一个 *整数类型*（*integral type*）是任一带符号的或无符号的，标量或向量整数类型。它排除了数组和结构体。
- 一个 *标量整数类型*（*scalar integral type*）是一个标量带符号或无符号的整数类型。
- 一个 *向量整数类型*（*vector integral type*）是一个带符号或无符号的整数向量。

带符号或无符号的整数变量被完全支持。在本文中，术语 *整数*（*integer*）旨在普遍包括带符号及无符号整数，即对带符号与无符号整数的统称。

对于OpenGL，无符号整数完全具有32位精度。当目标API为Vulkan时，**`highp`** 无符号整数完全具有32位精度。

对于OpenGL，带符号整数具有32位，其中包含了一位符号位，并以2的补码形式给出。当目标API为Vulkan时，**`highp`** 带符号整数具有32位，包含了一位符号位，并以2的补码形式给出。

当目标API为Vulkan时，**`mediump`** 和 **`lowp`** 整数由 SPIR-V 中的 **`RelaxedPrecision`** 装饰定义。

此外，导致上溢或下溢的减法和乘法将导致正确结果R的低32位。这里的R被计算时具有足够的精度以避免上溢或下溢。导致上溢的除法将产生一个未定义的值。

以下例子声明了整数，并可选地使用整数表达式进行初始化：

```glsl
int i, j = 42; // 默认的整数字面量类型是 int
uint k = 3u; // "u" 建立了该类型为 uint
```

整数常量字面量可以用十进制（比如：**`100`**），八进制（比如：**`0144`**），或是十六进制（比如：**`0x64`** 或 **`0X64`**）来表达。

在一个整数常量的数字之间不允许包含任何空白字符，这也包括具有在前导 **0** 或是前导 **0x** 或 **0X** 之后，或是在后缀 **u** 或 **U** 之前。当进行符号化（tokenizing）时，匹配上述最大的符号将在一个新的符号被启动前进行识别。当存在后缀 **u** 或 **U** 时，字面量具有类型 **`uint`**，否则该类型为 **int**。一个前导的单目减号（**-**）被解释为一个算术单目取非（arithmetic unary negation），即取其相反数，而不作为该常量的一部分。因而，字面量其本身总是用非负数语法进行表达的，尽管它们可能会产生一个负数值。

如果一个整数字面量的比特模式不能适用于32位（即超出了32位），则会引发编译时错误。字面量的比特模式总是不受修改地进行使用。因此，对于一个带符号的字面量，其比特模式包含了一个符号位，将该符号位置1来创建一个负数值。

比如：

```cpp
1 // OK。带符号整数，值为1
1u // OK。无符号整数，值为1
-1 // OK。对带符号整数使用了单目减号，结果是一个带符号整数，值为-1
-1u // OK。单目减号应用于无符号整数。结果是一个无符号整数，值为0xffffffff
0xA0000000 // OK。32位带符号十六进制数
0xABcdEF00u // OK。32位无符号十六进制数
0xffffffff // OK。带符号整数，值为-1
0x80000000 // OK。计算值为-2147483648
0xffffffffu // OK。无符号整数，值为0xffffffff
0xfffffffff // 错误：需要多于32 bits
3000000000 // OK。一个带符号十进制数字面量，取32位。该值计算为-1294967296
2147483648 // OK。计算为-2147483648（该字面量将其符号位置1）
5000000000 // 错误：需要多于32位
```

GLSL可通过开启 [GL_ARB_gpu_shader_int64](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_gpu_shader_int64.txt) 或更为推荐的 [GL_EXT_shader_explicit_arithmetic_types_int64](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展来使用64位整数。若当前实现支持此扩展，那么可以使用新增的64位整数类型：**`int64_t`**、**`uint64_t`**、**`i64vec2`**、**`u64vec2`**、**`i64vec3`**、**`u64vec3`**、**`i64vec4`**、**`u64vec4`**。此外，对于带符号的64位整数字面量采用 **l** 或 **L** 后缀；而对于无符号64位整数字面量则采用 **ul** 或 **UL** 后缀。

GLSL可通过开启 [GL_EXT_shader_explicit_arithmetic_types_int16](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展来使用16位整数。若当前实现支持此扩展，那么可以使用新增的16位整数类型：**`int16_t`**、**`uint16_t`**、**`i16vec2`**、**`u16vec2`**、**`i16vec3`**、**`u16vec3`**、**`i16vec4`**、**`u16vec4`**。此外，对于带符号的16位整数字面量采用 **s** 或 **S** 后缀；而对于无符号16位整数字面量则采用 **us** 或 **US** 后缀。

GLSL可通过开启 [GL_EXT_shader_explicit_arithmetic_types_int8](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展来使用8位整数。若当前实现支持此扩展，那么可以使用新增的8位整数类型：**`int8_t`**、**`uint8_t`**、**`i8vec2`**、**`u8vec2`**、**`i8vec3`**、**`u8vec3`**、**`i8vec4`**、**`u8vec4`**。8位整数字面量没有特定的后缀。

GLSL可通过开启 [GL_EXT_shader_explicit_arithmetic_types_int32](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展来使用新增的32位整数类型，这些类型与原有的32位整数类型对应等价。从而增加了以下关键字：**`int32_t`**（与 **`int`** 等价）、**`uint32_t`**（与 **uint** 等价）、**`i32vec2`**（与 **`ivec2`** 等价）、**`u32vec2`**（与 **`uvec2`** 等价）、**`i32vec3`**（与 **`u32vec3`** 等价）、**`i32vec4`**（与 **`ivec4`** 等价）、**`u32vec4`**（与 **`uvec4`** 等价）。

<br />

#### <a name="type_float"></a> 浮点类型（Floats）

单精度和双精度浮点变量在各种标量计算中进行使用。一般来说，术语 *浮点*（*floating-point*）既引用了单精度又引用了双精度浮点。以下例子定义了浮点变量：

```cpp
float a, b = 1.5; // 单精度浮点
double c, d = 2.0LF; // 双精度浮点
```

作为某一处理单元的一个输入值，一个单精度或双精度浮点变量被期待匹配IEEE754浮点定义，用于精度和动态范围。在一个着色器内的浮点变量也根据IEEE754规范来编码单精度浮点值。

（逻辑上，而不必在物理上），当用逻辑上的IEEE754进行编码时，操作（加法、乘法等）不必用IEEE754所要求的进行执行。

如果浮点字面量存在指数部分，那么十进制浮点（**.**）可以不需要，比如：**`3E5`**、**`2e8`**。在一个浮点常量内不能在任何地方出现空白符，包括在一个后缀之前。当符号化（tokenizing）时，在一个新的符号开启前，将会识别匹配上述定义的最大符号。当存在 **lf** 或 **LF** 后缀时，该字面量具有类型 **double**。否则的话，该字面量具有类型 **float**。一个前导单目减号（**-**）被解释为一个单目操作符，且不作为浮点常量的一部分。

GLSL可通过开启 [GL_EXT_shader_explicit_arithmetic_types_float16](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展来使用新增的16位浮点类型。若当前实现支持此扩展，那么可以使用此新增的16位浮点类型：**`float16_t`**、**`f16vec2`**、**`f16vec3`**、**`f16vec4`**、**`f16mat2`**、**`f16mat3`**、**`f16mat4`**、**`f16mat2x2`**、**`f16mat2x3`**、**`f16mat2x4`**、**`f16mat3x2`**、**`f16mat3x3`**、**`f16mat3x4`**、**`f16mat4x2`**、**`f16mat4x3`**、**`f16mat4x4`**。此外，对于16位浮点字面量采用 **`hf`** 或 **`HF`** 后缀。具有类型 **`float16_t`** 的变量表示完全使用了16个比特的浮点数，并且使用 IEEE 754-2008 中所描述的浮点表示进行存储。

GLSL可通过开启 [GL_EXT_shader_explicit_arithmetic_types_float32](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展来使用新增的32位浮点类型。若当前实现支持此扩展，那么可以使用此新增的32位浮点类型：**`float32_t`**（与 **`float`** 等价）、**`f32vec2`**（与 **`vec2`** 等价）、**`f32vec3`**（与 **`vec3`** 等价）、**`f32vec4`**（与 **`vec4`** 等价）、**`f32mat2`**（与 **`mat2`** 等价）、**`f32mat3`**（与 **`mat3`** 等价）、**`f32mat4`**（与 **`mat4`** 等价）、**`f32mat2x2`**（与 **`mat2x2`** 等价）、**`f32mat2x3`**（与 **`mat2x3`** 等价）、**`f32mat2x4`**（与 **`mat2x4`** 等价）、**`f32mat3x2`**（与 **`mat3x2`** 等价）、**`f32mat3x3`**（与 **`mat3x3`** 等价）、**`f32mat3x4`**（与 **`mat3x4`** 等价）、**`f32mat4x2`**（与 **`mat4x2`** 等价）、**`f32mat4x3`**（与 **`mat4x3`** 等价）、**`f32mat4x4`**（与 **`mat4x4`** 等价）。此外，此扩展对单精度浮点字面量引入了后缀 **f** 或 **F**，使得带有此后缀的浮点字面量类型为32位单精度浮点类型。然而，对于缺省后缀的浮点字面量也仍然表示为32位单精度浮点类型，比如 **`0.5f`**、**`0.5F`** 和 **`0.5`** 均默认表示为 **`float`** 或 **`float32_t`** 类型。**`float32_t`** 类型与 **`float`** 类型的变量是等价的，并都表示了 IEEE 754-2008 中所描述的32位浮点表示。

GLSL可通过开启 [GL_EXT_shader_explicit_arithmetic_types_float64](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展来使用新增的64位浮点类型。若当前实现支持此扩展，那么可以使用此新增的64位浮点类型：**`float64_t`**（与 **`double`** 等价）、**`f64vec2`**（与 **`dvec2`** 等价）、**`f64vec3`**（与 **`dvec3`** 等价）、**`f64vec4`**（与 **`dvec4`** 等价）、**`f64mat2`**（与 **`dmat2`** 等价）、**`f64mat3`**（与 **`dmat3`** 等价）、**`f64mat4`**（与 **`dmat4`** 等价）、**`f64mat2x2`**（与 **`dmat2x2`** 等价）、**`f64mat2x3`**（与 **`dmat2x3`** 等价）、**`f64mat2x4`**（与 **`dmat2x4`** 等价）、**`f64mat3x2`**（与 **`dmat3x2`** 等价）、 **`f64mat3x3`**（与 **`dmat3x3`** 等价）、**`f64mat3x4`**（与 **`dmat3x4`** 等价）、**`f64mat4x2`**（与 **`dmat4x2`** 等价）、**`f64mat4x3`**（与 **`dmat4x3`** 等价）、**`f64mat4x4`**（与 **`dmat4x4`** 等价）。**`float64_t`** 类型与 **`double`** 类型的变量是等价的，并都表示了 IEEE 754-2008 中所描述的64位浮点表示。

如果：

- [GL_EXT_shader_explicit_arithmetic_types_int8](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)
- [GL_EXT_shader_explicit_arithmetic_types_int16](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)
- [GL_EXT_shader_explicit_arithmetic_types_int32](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)
- [GL_EXT_shader_explicit_arithmetic_types_int64](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)
- [GL_EXT_shader_explicit_arithmetic_types_float16](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)
- [GL_EXT_shader_explicit_arithmetic_types_float32](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)
- [GL_EXT_shader_explicit_arithmetic_types_float64](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt)

上述任一扩展被允许，那么 [GL_EXT_shader_explicit_arithmetic_types](https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt) 扩展将被隐式开启。

<br />

#### <a name="type_vector"></a> 向量（Vectors）

OpenGL着色语言包含了用于浮点值、整数值以及布尔值这些类型的泛型的 2、3 和 4 分量向量的数据类型。浮点向量变量可以被用于存放颜色、法线、位置、纹理坐标、纹理查找结果等等。布尔向量可以被用于对数值向量的按各个分量进行比较。以下是向量声明的一些例子：

```glsl
vec2 texcoord1, texcoord2;
vec3 position;
vec4 myRGBA;
ivec2 textureLookup;
bvec3 less;
```

向量的初始化用构造器来完成。见 “[向量与矩阵构造器](#vector_and_matrix_constructors)”。

<br />

#### <a name="type_matrices"></a> 矩阵（Matrices）

OpenGL着色语言具有浮点数值的 2 × 2、2 × 3、2 × 4、3 × 2、3 × 3、3 × 4、4 × 2、4 × 3，以及 4 × 4 矩阵。以 **mat** 开头的矩阵类型具有单精度分量，而以 **dmat** 开头的矩阵类型具有双精度分量。此类型中的第一个数是列的个数（或者说是列向量的个数），而第二个数则是行的个数（或者说是每一列向量的分量个数）。如果该类型仅有一个数，那么此矩阵为一个方阵。矩阵声明的例子如下：

```glsl
mat2 mat2D;
mat3 optMatrix;
mat4 view, projection;
mat4x4 view; // 另一种方式来声明 mat4 变量
mat3x2 m; // 具有 3 列 2 行的一个矩阵，或者可视为具有 3 个列向量的矩阵，每个列向量含有 2 个分量
dmat4 highPrecisionMVP;
dmat2x4 dm;
```

矩阵值的初始化用构造器来完成。见 “[向量与矩阵构造器](#vector_and_matrix_constructors)”。

<br />

#### <a name="opaque_types"></a> 隐含类型（Opaque Types）

**定义**：一个 *隐含类型*（*opaque type*）是一种类型，此类型的内部结构从语言中受到隐藏。

在以下部分所列出的隐含类型声明了对其他对象有效的隐含句柄的变量。这些对象通过内建函数进行访问，而不是直接通过对所声明变量的读或写。它们只能作为函数形参或是用 **`uniform`** 限定符修饰的变量进行声明（见 “[Uniform 变量](#uniform_variables)”）。仅有的采用存储器限定符的隐含类型是图像类型。除了可使用数组索引、结构体成员选择、以及圆括号之外，隐含变量不允许在表达式中作为操作数；如果这么使用，则会产生一个编译时错误。

隐含变量不能被对待为左值；因而不能被用作为 **`out`** 或 **`inout`** 函数形参，它们也不能被赋值。任何做了这种使用均将导致一个编译时错误。然而，它们可以作为带有匹配类型及存储器限定符的 **`in`** 形参进行传递。它们不能用一个初始化器进行声明。

因为一单个隐含类型的声明有效地声明了两个对象，隐含句柄其本身以及它所引用的对象，所以对于一个存储（storage）限定符和一个存储器（memory）限定符都留有空间。存储限定符将限定隐含句柄，而存储器限定符将限定它所引用的对象。

<br />

##### <a name="texture-combined_samplers"></a> 与纹理结合的采样器（Texture-Combined Samplers）

与纹理结合的采样器类型（比如 **`sampler2D`**）是在基本类型表中所描述的采样器类型，作为访问纹理的句柄。（它们不包括 **`sampler`** 以及 **`samplerShadow`**。）对于每种纹理目标、以及对于每种浮点、带符号整数、还有无符号整数数据类型，都有不同的与纹理结合的采样器类型。纹理访问通过内建的纹理函数（在“[纹理函数](#texture_functions)”中描述）完成，并且与纹理所结合的采样器被用于指定要访问哪个纹理以及它是如何进行滤波的。

与纹理相结合的采样器类型是隐含类型，其行为和声明如上面针对隐含类型所描述的那样。当在一个着色器内聚合进数组中时，它们只能用一个动态均匀整数表达式进行索引，否则结果是未定义的。

<br />

##### <a name="images"></a> 图像（Images）

图像类型是隐含类型，其行为和声明如上述针对隐含类型所描述的那样。它们可以被进一步用存储器限定符进行限定。当在一个着色器内聚合进数组中时，图像只能用一个动态均匀的整数表达式进行索引，否则结果是未定义的。

图像变量是对一维、二维、或三维图像的句柄，对应于绑定在一个图像单元的一单个细节度的纹理图像的全部或一部分。对于每种纹理目标，以及对于每种浮点、带符号整数、还有无符号整数数据类型，都有不同的图像变量类型。图像访问应该使用匹配于纹理目标的一种图像类型，其细节度绑定于图像单元中，或是对于3D或阵列图像的非分层绑定，应该使用匹配于该层图像维度的图像类型。（即，一个3D、2D阵列、立方体、或立方体阵列的一个图层，应该使用 **`image2D`**，一个1D阵列的一个图层应该使用 **`image1D`**，而对于2D多重采样阵列的一个图层应该使用 **`image2DMS`**。）如果图像目标类型不匹配于此方式下所绑定的图像，如果数据类型不匹配于所绑定的图像，或如果格式 **`layout`** 限定符不匹配于在 [Image Load Store](https://www.khronos.org/opengl/wiki/Image_Load_Store) 中所描述的图像单元格式，那么图像访问的结果是未定义的，但不能导致程序终结。

图象变量被用于在“[图像函数（Image Functions”）](#image_functions)”中所描述的图像加载、存储、以及原子函数，以指定要访问的一个图像。

<br />

##### <a name="atomic_counters"></a> 原子计数器（Atomic Counters）

原子计数器仅支持基于OpenGL的GLSL，而不支持基于 Vulkan API 的GLSL。对于 Vulkan API 而言，可直接使用各类原子数据类型。

原子计数器类型（比如 **`atomic_uint`**）是对计数器的隐含句柄，其声明和行为如上面针对隐含类型所描述的那样。它们所声明的变量指定了要访问哪个计数器，当使用在“[原子计数器函数（Atomic Counter Functions）](#atomic_counter_functions)”中所描述的内建原子计数器函数时。它们被绑定到在“[原子计数器布局限定符（Atomic Counter Layout Qualifiers）](#atomic_counter_layout_qualifiers)”中所描述的缓存。

在一个着色器内被聚合到数组中的原子计数器只能用动态均匀整数表达式进行索引，否则结果是未定义的。

结构体成员不能被声明为原子计数器类型。

当目标API为Vulkan时，原子计数器类型不可用。

<br />

<a name="texture_sampler_and_samplerShadow_types"></a>
##### 纹理、采样器、以及 samplerShadow 类型（Texture, sampler, and samplerShadow Types）

纹理（比如 **`texture2D`**）、**`sampler`**、和 **`samplerShadow`** 类型均为隐含类型，其声明与行为如上面针对隐含类型所描述的那样。这些类型只有当目标API为Vulkan时才可用。当在一个着色器内聚合进数组时，这些类型只能使用一个动态均匀表达式进行索引，否则纹理查找将会导致未定义的值。纹理变量是对一维、二维及三维纹理、立方体贴图等等的句柄，这些列在基本类型表中。对于每种纹理目标，以及对于每种浮点、带符号整数、以及无符号整数数据类型，都有不同的纹理类型。纹理可以用 **`sampler`** 或 **`samplerShadow`** 类型的一个变量进行结合，以创建一个纹理绑定的采样器类型（比如：**`sampler2D`** 或 **`sampler2DShadow`**）。这通过一个构造器来实现，比如：`sampler2D(texture2D, sampler)`，`sampler2DShadow(texture2D, sampler)`，`sampler2DShadow(texture2D, samplerShadow)`，或是 `sampler2D(texture2D, samplerShadow)`，这些细节在 [构造器（Constructors）](#constructors) 这一章中描述。

<br />

<a name="subpass_inputs"></a>
##### 子遍输入（Subpass Inputs）

子遍输入类型只有当目标API为Vulkan时才可用。

子遍输入类型（比如 **`subpassInput`**）是隐含类型，其声明和行为如上面针对隐含类型所描述的那样。当在一个着色器内被聚合进数组时，它们只能用一个动态均匀整数表达式进行索引，否则结果是未定义的。

子遍输入类型是对二维单个样本的或多重采样的图像的句柄，对于每种浮点、带符号整数、以及无符号整数数据类型具有不同的类型。

子遍类型只在片段着色器中可用。如果在其他任一流水线阶段中使用它们，则会产生一个编译时错误。

<br />

<a name="structures"></a>
#### 结构体（Structures）

用户自定义类型可以通过将其他已定义的类型聚合到使用 **`struct`** 关键字的一个结构体进行创建。比如：

```glsl
struct light {
    float intensity;
    vec3 position;
} lightVar;
```

在此例子中，*light* 变为新类型的名字，而 *lightVar* 变为类型 *light* 的一个变量。如果要声明新类型的变量，则直接使用其名（而不需要使用 **`struct`** 关键字）。

```cpp
light lightVar2;
```

结构体必须至少要有一个成员声明。这意味着，像 `struct S { };` 这种结构体类型的定义是非法的。成员声明符可以包含精度限定符，但要是使用任何其他限定符则会产生一个编译时错误。GLSL的结构体不支持位域（Bit Fields）。成员类型必须是已被定义的（不能包含先前引用）。如果一个成员声明包含了一个初始化器，则会产生一个编译时错误。成员声明符可包含数组，但这些数组必须具有一个指定的大小，并且该大小必须是和一个常量整数表达式，且值要大于零（见“[常量表达式（Constant Expressions）](#constant_expressions)”）。结构体的每一层具有其自己的名字空间，用于在成员声明符中所给定的名字；这些名字只需要在该名字空间内是唯一的。

匿名结构体不被支持。嵌套的结构体定义也不被支持。这些都会导致编译时错误。

```glsl
struct S { float f; };    // 允许：S 被定义为一个结构体类型
struct T {
    S; // 错误：不允许匿名结构体
    struct { int i; };    // 错误：不允许嵌套的结构体
    S s; // 允许：嵌套了一个具有名字的结构体变量作为成员
    highp float f;    // 允许：使用精度限定符修饰结构体成员。且这里 T 的成员 f 与上述结构体 S 中的成员 f 是有所区别的。
    const int c;    // 错误：不能使用除精度限定符以外的限定符来修饰结构体成员，包括这里的存储限定符
    int array[8];    // 允许：具有8个元素的 int 类型数组作为结构体成员
    int unknown[];    // 错误：作为结构体成员，必须指明数组的大小，且大小必须大于零
};
```

结构体可以在声明时使用构造器进行初始化，在“[结构体构造器（Structure Constructors）](#structure_constructors)”中讨论。

任何对一个类型或限定符使用的限制也应用于包含一个该类型或限定符成员的任一结构体。这也递归地应用于以那些结构体类型所定义的结构体成员。

<br />

<a name="arrays"></a>
#### 数组（Arrays）

相同类型的变量可以通过声明一个名字，后面跟着方括号（**[ ]**），方括号内可以放一个可选的大小，而被聚合到数组中。当一个数组大小在一个声明中指定时，它必须是一个大于零的整数常量表达式（见“[常量表达式（Constant Expressions）](#constant_expressions)”）。除了一个着色器存储块（shader storage block，见“[Interface Blocks](#interface_blocks)”）中的最后所声明的成员，一个数组的大小必须被显式声明（即，显式地指定数组的大小）。任一数组的大小必须在将它作为一个实参传递给一个函数之前被声明。任何对这些规则的违背都会产生编译时错误。声明一个不具有大小的一个数组是合法的，但后面要重新将它声明为同名同类型的数组，且指定一个大小，否则只能用常量表达式（隐式大小）对它进行索引（此时GLSL实现能在编译时隐式确定该数组的大小）。然而，除非做了说明，否则 interface blocks 不能被重新声明；一个用户声明的 block 中的一个未指定大小的数组成员不能通过一个 block 的重新声明来指定其大小。对一个数组用某一确定的大小进行声明之后，然后（在同一着色器内）用一个大于或等于其所声明大小的整数常量表达式对该数组进行索引，那么会产生一个编译时错误。要用一个大小小于等于当前着色器中索引该数组的早先所使用的任一索引来重新声明一个未指定大小的数组，那么会产生一个编译时错误。如果用一个负数的常量表达式取索引一个数组，也会产生一个编译时错误。声明为一个函数声明中正式形参的数组必须指定一个大小。如果用值大于等于数组所指定大小、或是用小于0的一个非常量表达式来索引一个数组，则会导致未定义行为。数组只有一单个维度（在“[ ]”内的一单个条目），然而，可以声明数组的数组。所有类型（基本类型、结构体、数组）都能作为数组的元素来形成一个数组。

所有数组本质上是同构的（homogeneous）；所有所包含的元素都具有相同类型和大小，不过有一个例外。该例外是，一个着色器存储块可以含有一个未指定大小的数组作为其最后一个成员（该数组大小在运行时确定）；一个数组可以从这么一个着色器存储块来构成，即便该存储块对于它们最后的成员具有不同的长度。

下面给出一些例子：

```glsl
float frequencies[3];
uniform vec4 lightPosition[4];
light lights[];
const int numLights = 2;
light lights[numLights];

// 一个着色器存储块（shader storage block），在“缓存变量（Buffer Variables）”这一节中介绍 
layout(std430, set = 0, binding = 0) buffer b {
    float u[];  // 产生错误，除非 u 具有连接时的静态大小
    vec4 v[];  // OK， v 将被动态分配大小，而非静态
} name[3];  // 当此 block 作为一个数组时，所有的 u 将具有相同大小，但所有的 v 则不必要，如果其大小是动态分配的话

// 定义函数 foo，其第一个形参类型是含有8个元素个数的 int 类型数组
void foo(in int array[8])
{

}

void main()
{
    int unsized_array[];        // 这里没有指定 unsized_array 数组的大小
    unsized_array[0] = 10;      // 但如果用整数常量表达式对它进行索引，则是合法的
    unsized_array[7] = 20;      // unsized_array 的大小为8，因为对它进行索引的最大整数是7，从而GLSL实现能在编译时确定其大小

    int sized_array[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    // 用显式指定8个元素大小的数组 sized_array 作为实参传递给函数 foo 是合法的。
    // 但用未指定大小的 unsized_array 作为 foo 函数的实参则是非法的。
    foo(sized_array);
}
````

一个数组类型可以通过非数组类型后面跟一个数组指定符来构成。这么一个数组指定符的所有维度必须包含一个大小。

```cpp
float[5]  // 大小为 [5]，类型为 float 的一个数组
float[2][3]  // 大小为 [2][3]，类型为 float 的一个数组，而不是大小为 [3]，类型为 float[2] 的数组
```

GLSL中数组类型可以被用于任何地方，跟其他类型一样，包括作为一个函数的返回值。比如：

```cpp
float[5] foo(in int arr[5])
{
    const float dstArray[5] = { float(arr[0]), float(arr[1]), float(arr[2]), float(arr[3]), float(arr[4]) };
    return dstArray;
}
```

数组可以用构造器来构造：

```cpp
float[5](3.4, 4.2, 5.0, 5.2, 1.1)
```

也能作为函数的匿名形参：

```cpp
void foo(float[5])
```

还有如Java、C\# 这些编程语言那样来声明一个数组变量或是作为函数形参：

```cpp
void foo(int[5] arr)
{
    const int[5] dstArr = int[5](arr[0], arr[1], arr[2], arr[3], arr[4]);
}
```

数组可以用数组构造器来构成其初始化器：

```cpp
float a[5] = float[5](3.4, 4.2, 5.0, 5.2, 1.1);
float[5] b = float[](3.4, 4.2, 5.0, 5.2, 1.1);  // 同上一样
```

一个数组的数组可如下声明：

```cpp
vec4 a[3][2];  // 大小为3的数组，其每个元素为大小为2的 vec4 的数组
```

上述代码声明了大小为3的一个一维数组，其每个元素为大小为2的、类型为 **`vec4`** 的一维数组。以下声明做了同样的事情：

```cpp
vec4[2] a1[3];
vec4[3][2] a2;
```

当在透明存储器（比如像在一个 uniform block 中）中时，其布局是最里面的（在最右边所声明的）维度迭代比外面的维度迭代要快。也就是说，像上述代码，存储器中的次序将会是：

> 低地址 : a[0][0] : a[0][1] : a[1][0] : a[1][1] : a[2][0] : a[2][1] : 高地址

以下代码中由构造器所需要的 *a* 的类型以及匿名函数形参的类型都是 **`vec4[3][2]`**：

```glsl
vec4 b[2] = vec4[2](vec4(0.0), vec4(0.1));
vec4[3][2] a = vec4[3][2](b, b, b);  // 构造器
void foo(vec4[3][2]);  // 带有未命名函数形参的函数原型
```

可选地，初始化器列表语法可以用于初始化一个数组的数组：

```glsl
vec4 a[3][2] = { vec4[2](vec4(0.0), vec4(1.0)),
                 vec4[2](vec4(0.0), vec4(1.0)),
                 vec4[2](vec4(0.0), vec4(1.0)) };
```

未指定大小的数组可以通过显式地使用一个初始化器，在声明时来确定其大小：

```cpp
float a[5];
float b[] = a;  // b 显式地指定了其大小为5
float b[5] = a;  // 与上面相同
float b[] = float[](1,2,3,4,5); // 同样显式地指定了数组 b 的大小为5
```

然而，要对一个未指定大小的数组赋值将会产生一个编译时错误。注意，这是一种很少见的情况，即初始化器和赋值出现了具有不同的语义。对于数组的数组，任一未指定大小的维度通过初始化器进行显式地指定大小：

```glsl
// OK，大小为 a[3][2]
vec4 a[][] = { vec4[2](vec4(0.0), vec4(1.0)),
               vec4[2](vec4(0.0), vec4(1.0)),
               vec4[2](vec4(0.0), vec4(1.0)) };
```

数组知道它们所包含的元素个数。这可以通过使用 **`length()`** 方法获得：

```cpp
float a[5];
a.length();  // 返回5
```

**`length()`** 方法返回了 **`int`** 类型。如果一个数组已经被显式地指明大小，那么由 **`length()`** 方法所返回的值是一个常量表达式。如果一个数组没有被显式地指明大小，并且它是一个 shader storage block 所声明的最后的成员，那么返回值将不是一个常量表达式，并且将在运行时基于那个 block 所提供存储的缓存对象的大小进行确定。这种数组是运行时确定大小的。对于运行时确定大小的数组，由 **`length()`** 方法所返回的值将是未定义的，如果该数组被包含在 shader storage blocks 的一个数组中，且用一个小于零或大于等于该数组中 blocks 个数的非常量表达式进行索引。

**`length()`** 方法不能对这么一个数组进行调用，该数组没在运行时指明大小，并且也尚未被显式地指明大小；这将会导致一个编译时错误。

当 **`length()`** 方法返回一个编译时常量时，应用 **`length()`** 方法的表达式不能包含任何副作用，诸如对此表达式内对左值的写，或是其本身具有副作用的函数调用。只有 **`length()`** 方法所返回的编译时常量其本身需要被计算。包括任一编译时错误报告的行为和结果是未定义的，如果该表达式包含了其他副作用。

```cpp
float a;
int b = 0;
const int s = float[2](a = 3.0, ++b).length();  // 非法的副作用
```

**`length()`** 方法对于数组的数组也同样能工作良好：

```glsl
vec4 a[3][2];
int len = a.length() // this is 3
a[x].length() // this is 2
```

当 **`length()`** 方法返回了一个编译时常量时，在方括号中的表达式（比如上述代码中第三行里的 *x*）将被解析并接受对数组索引所要求的规则，但该数组将不会被解引用（dereferenced）。从而，此行为一定是良好定义的，即便该表达式的值（比如上面的 *x*）超出了数组的边界，只要该表达式不包含任何副作用。

当 **`length()`** 方法返回一个运行时值（而不是一个编译时常量）时，该数组将被解引用。比如，如果 *x* 不是一个编译时常量，且超出了数组的边界，那么将会导致一个未定义的值。更通常来说，所有所涉及到的表达式都被完整计算并执行。

```glsl
struct B
{
    int a[8];
};

layout(std430, set = 0, binding = 2) buffer DataBuffer {
    B s[];
};

void main(void)
{
    B b[3];
    int x = 0;

    // 对于含有一个成员数组 a 的一个编译时大小的数组 b：
    b[x + 3].a.length();  // b 不会被解引用，x + 3 会被计算并受检查
    b[++x].a.length();  // 不允许；结果是未定义的
    // 对于一个 shader storage object 的一个数组 s（其长度在运行时指定），
    // 数组 s 包含了一个成员数组 a：
    s[++x].a.length();  // s 被解引用；++x 需要是一个有效的索引值
}
```

对于隐式指定大小或运行时指定大小的数组，只有最外边的维度才能缺省对大小的指定。包含一个未知数组大小的一个类型不能用于构成一个数组，直到它得到了一个显式的大小，除非是对于 shader storage blocks，只有其最后的成员是未指定大小的数组。

在一个 shader storage block 中，最后一个成员可以不显式指定大小来声明。在此情况下，有效的数组大小从存储在 interface block 中的数据大小在运行时推导出。这样的运行时确定大小的数组可以用通用整数表达式进行索引。然而，将它们作为实参传递给一个函数、或是用一个负数常量表达式对它们索引，将会引发编译时错误。

<br />

<a name="implicit_conversions"></a>
#### 隐式转换（Implicit Conversions）

在某些情况下，一个表达式及其类型将会被隐式转换为一个不同的类型。下表展示了所有GLSL所允许的隐式转换，另外，涉及到8位、16位、**`int32_t`** 类型所表示的32位整数、64位整数以及16位、**`float64_t`** 类型所表示的64位浮点数时，将需要开启对应特定的 **GL_EXT_shader_explicit_arithmetic_types** 扩展。

**下表展示了GLSL所允许的整数之间的隐式转换**

表达式类型 | 可以被隐式转换到的类型
---- | ----
int8_t | uint8_t, int16_t, uint16_t, uint32_t, int64_t, uint64_t, ***int***
i8vec2 | u8vec2, i16vec2, u16vec2, u32vec2, i64vec2, u64vec2, ***ivec2***
i8vec3 | u8vec3, i16vec3, u16vec3, u32vec3, i64vec3, u64vec3, ***ivec3***
i8vec4 | u8vec4, i16vec4, u16vec4, u32vec4, i64vec4, u64vec4, ***ivec4***
int16_t | uint16_t, uint32_t, int64_t, uint64_t, ***int***
i16vec2 | u16vec2, u32vec2, i64vec2, u64vec2, ***ivec2***
i16vec3 | u16vec3, u32vec3, i64vec3, u64vec3, ***ivec3***
i16vec4 | u16vec4, u32vec4, i64vec4, u64vec4, ***ivec4***
uint8_t | int16_t, uint16_t, uint32_t, int64_t, uint64_t, ***int***
u8vec2 | i16vec2, u16vec2, u32vec2, i64vec2, u64vec2, ***ivec2***
u8vec3 | i16vec3, u16vec3, u32vec3, i64vec3, u64vec3, ***ivec3***
u8vec4 | i16vec4, u16vec4, u32vec4, i64vec4, u64vec4, ***ivec4***
uint16_t | uint32_t, int64_t, uint64_t, ***int***
u16vec2 | u32vec2, i64vec2, u64vec2, ***ivec2***
u16vec3 | u32vec3, i64vec3, u64vec3, ***ivec3***
u16vec4 | u32vec4, i64vec4, u64vec4, ***ivec4***
int32_t | uint32_t, int64_t, uint64_t
i32vec2 | u32vec2, i64vec2, u64vec2
i32vec3 | u32vec3, i64vec3, u64vec3
i32vec4 | u32vec4, i64vec4, u64vec4
uint32_t | int64_t, uint64_t
u32vec2 | i64vec2, u64vec2
u32vec3 | i64vec3, u64vec3
u32vec4 | i64vec4, u64vec4
int64_t | uint64_t
i64vec2 | u64vec2
i64vec3 | u64vec3
i64vec4 | u64vec4 

**下表展示了GLSL所允许的浮点数之间的隐式转换**

表达式类型 | 可以被隐式转换到的类型
---- | ----
float16_t  | float32_t, ***double***
float32_t | ***double***
f16vec2    | f32vec2, ***dvec2***
f32vec2 | ***dvec2***
f16vec3    | f32vec3, ***dvec3***
f32vec3 | ***dvec3***
f16vec4    | f32vec4, ***dvec4***
f32vec4 | ***dvec4***
f16mat2    | f32mat2, ***dmat2***
f32mat2 | ***dmat2***
f16mat3    | f32mat3, ***dmat3***
f32mat3 | ***dmat3***
f16mat4    | f32mat4, ***dmat4***
f32mat4 | ***dmat4**
f16mat2x3  | f32mat2x3, ***dmat2x3***
f32mat2x3 | ***dmat2x3***
f16mat2x4  | f32mat2x4, ***dmat2x4***
f32mat2x4 | ***dmat2x4***
f16mat3x2  | f32mat3x2, ***dmat3x2***
f32mat3x2 | ***dmat3x2***
f16mat3x4  | f32mat3x4, ***dmat3x4***
f32mat3x4 | ***dmat3x4***
f16mat4x2  | f32mat4x2, ***dmat4x2***
f32mat4x2 | ***dmat4x2***
f16mat4x3  | f32mat4x3, ***dmat4x3***
f32mat4x3 | ***dmat4x3***

**下表展示了GLSL所允许的整数到浮点数的隐式转换**

表达式类型 | 可以被隐式转换到的类型
---- | ----
int8_t     | float16_t, float32_t, float64_t
i8vec2     | f16vec2, f32vec2, f64vec2
i8vec3     | f16vec3, f32vec3, f64vec3
i8vec4     | f16vec4, f32vec4, f64vecv4
int16_t    | float16_t, float32_t, float64_t
i16vec2    | f16vec2, f32vec2, f64vec2
i16vec3    | f16vec3, f32vec3, f64vec3
i16vec4    | f16vec4, f32vec4, f64vec4
uint8_t    | float16_t, float32_t, float64_t
u8vec2     | f16vec2, f32vec2, f64vec2
u8vec3     | f16vec3, f32vec3, f64vec3
u8vec4     | f16vec4, f32vec4, f64vec4
uint16_t   | float16_t, float32_t, float64_t
u16vec2    | f16vec2, f32vec2, f64vec2
u16vec3    | f16vec3, f32vec3, f64vec3
u16vec4    | f16vec4, f32vec4, f64vec4
int32_t    | float32_t, float64_t
i32vec2    | f32vec2, f64vec2
i32vec3    | f32vec3, f64vec3
i32vec4    | f32vec4, f64vec4
uint32_t   | float32_t, float64_t
u32vec2    | f32vec2, f64vec2
u32vec3    | f32vec3, f64vec3
u32vec4    | f32vec4, f64vec4
int64_t    | float64_t
i64vec2    | f64vec2
i64vec3    | f64vec3
i64vec4    | f64vec4
uint64_t   | float64_t 
u64vec2    | f64vec2
u64vec3    | f64vec3
u64vec4    | f64vec4

上述三个表中，用 ***加粗斜体*** 标识出来的类型表示类型 **晋升**（**promotion**）。当一种类型向另一种目标类型进行隐式转换时，倘若源类型能晋升到目标类型同样的位宽，那么会先发生类型晋升，再做对应的类型转换。比如，当我们需要将一个 **`int8_t`** 类型的整数变量转换为 **`uint`** 类型时，原始整数会先从 **`int8_t`** 类型晋升到 **`int`** 类型，随后再从 **int** 类型隐式转换为 **uint** 类型。我们看以下例子：

```cpp
const int8_t byteValue = int8_t(100);
const uint uintValue = byteValue;
```

对于上述第二条语句，将会产生如下式样的 SPIR-V 目标代码：

```llvm
%48 = OpSConvert %int %46
%49 = OpBitcast %uint %48
```

从上述 SPIR-V 汇编代码中可见，这里是先用 **`OpSConvert`** 指令将加载的 **`int8_t`** 类型的整数变量先转换为 **`int`** 类型。随后再使用 **`OpBitcast`** 指令将晋升后的 **`int`** 类型的整数转换为 **`uint`** 类型。

而如果要转换到的目标类型不支持晋升的话，则直接进行隐式转换。比如以下例子：

```cpp
const int8_t byteValue = int8_t(100);
const uint16_t ushortValue = byteValue;
```

由于这里 **`int8_t`** 类型的整数在隐式转换为 **`uint16_t`** 的过程中，其位宽没有达到 **`int`** 的宽度（即32位），因此直接做类型转换。其过程如以下 SPIR-V 所示：

```llvm
%40 = OpBitcast %uchar %38
%41 = OpUConvert %ushort %40
```

我们可以看到，这里是先用 **`OpBitcast`** 指令将原始变量（byteValue）转换为 **`uint8_t`** 类型（即这里的 `%uchar`），随后再将该类型通过 **`OpUConvert`** 指令再转换为 **`uint16_t`** 类型。

对于相同等级（即相同位宽）的整数类型，GLSL不提供从无符号整数转换到带符号整数类型的隐式转换；不提供从浮点数到整数类型的转换；不提供从高精度到低精度的隐式转换；不提供从标量到向量的隐式转换；不提供从向量到标量的隐式转换；不提供行数和列数不匹配的矩阵类型之间的隐式转换。数组类型之间或结构体类型之间也不提供隐式转换。比如，一个 **`int`** 元素类型的数组不能被隐式转换为 **`float`** 元素类型的数组。

当完成一个隐式转换时，这并不是对表达式比特模式的重新解释，而是将其值转换到新类型的等价值。比如，整数值-5可以被隐式转换为浮点值-5.0。

当把整数值转换为浮点值时，结果是尽可能保持精确的。如果要被转换的值在该值能表示的范围内，那么将选择下一个更低或更高可表示的值，由以下两点之一确定：

- 通过主机端的API所请求的舍入模式进行判定；或者
- 如果没有请求舍入模式，那么由实现定义。

比起一个浮点尾数具有更多比特精度的整数值，当要转换到浮点数时将会丢失精度。如果正被转换的值超出了值可被表示的范围，那么行为是未定义的。

当对双目操作符执行隐式转换时，可能会有多个数据类型这两个操作数可以被转换为。比如，当我们用一个 **`int`** 值去加一个 **`uint`** 值时，这两个值均可能会被隐式转换为 **`uint`**、**`float`**、以及 **`double`**。在这些情况下，转换如以下定义的方式发生：（注：在本段中，向量和矩阵类型被引用为从带有相同位宽与位解释的标量类型所派生的类型）

- 如果有任一操作数具有 **`double`** 类型、或是从 **`double`** 所派生出的类型，那么另一个操作数应该被转换为 **`double`** 或其派生类型。
- 否则，如果有任一操作数具有 **`float`** 类型、或是从 **`float`** 所派生出的类型，那么另一个操作数应该被转换为 **`float`** 类型或其派生类型。
- 否则，如果有任一操作数具有 **`float16_t`** 类型、或是从 **`float16_t`** 所派生出的类型，那么另一个操作数应该被转换为 **`float16_t`** 类型或其派生类型。
- 否则，如果两个操作数都具有整数类型，那么要将以下规则应用到这俩操作数：
    - 如果两个操作数都具有相同类型，则不需要进一步的转换。
    - 如果两个操作数都具有带符号整数类型或都具有无符号整数类型，那么具有更小整数转换等级的类型的操作数将被转换为具有更高整数转换等级的类型。
    - 否则，如果带有无符号整数类型的操作数具有比另一个操作数的类型更高的转换等级，那么带有带符号整数类型的操作数将被转换为带有无符号整数类型的操作数类型。
    - 否则，如果带有带符号整数类型的操作数可以表示带有无符号整数类型操作数类型的所有值，那么带有无符号整数类型操作数将被转换为带有带符号整数类型的操作数类型。
    - 否则，两个操作数都将被转换为无符号整数类型，该类型对应于带符号整数类型的操作数类型。

（注：跟C++不同，如果两个操作数都具整数类型，那么在考虑任何其他转换之前，上述规则不要求执行整数晋升。）

每个整数类型都具有如下定义的整数转换等级：

- 没有两种带符号整数类型具有相同等级。
- 一个标量带符号整数类型的等级将高于任一具有更小位宽的标量带符号整数类型的等级。
- **`int64_t`** 的等级应该高于 **`int`** 的等级，**`int`** 的等级应该高于 **`int16_t`** 的等级，**`int16_t`** 的等级将高于 **`int8_t`** 的等级。
- 任一向量带符号整数类型的等级等于基本标量带符号整数类型的等级。
- 任一标量无符号整数类型的等级将等于对应标量带符号整数类型的等级。
- 任一向量无符号整数类型的等级等于对应的标量无符号整数类型的等级。

<br />

<a name="initializers"></a>
#### 初始化器（Initializers）

在声明时，可以给一个变量提供初始值，用一个等号（**=**）然后在后面跟着一个初始化器进行指定。初始化器要么是一个 *赋值表达式*（*assignment-expression*），要么是一个用花括号包围住的初始化器列表。

*赋值表达式*（*assignment-expression*）是一个正规的表达式，除了在圆括号外的一个逗号（**,**）被解释为该初始化器的结束，而不是作为顺序操作符。正如下面更详细的解释，这允许创建嵌套的初始化器：变量类型与其初始化器必须依据嵌套、在每一层所存在的分量/元素/成员的个数、以及分量/元素/成员的类型都要完全匹配。在全局作用域的一个 *赋值表达式*（*assignment-expression*）可以包含对用户自定义函数的调用。

```cpp
// 声明一个带有3个元素的数组变量 a
int a[3] = {    // 由此左花括号引出初始化器列表
    1,  // 这个逗号表示对该数组第一个元素的初始化器结束

    // 这里，圆括号里面的逗号表示“顺序操作符”，
    // 圆括号外的逗号则表示对该数组第二个元素的初始化器结束
    (1, 2),

    3
};
```

一个初始化器中的一个 *赋值表达式*（*assignment-expression*）必须要么与它所初始化的对象具有相同类型，要么是一个类型，根据“[隐式转换（Implicit Conversions）](#implicit_conversions)”可以被转换为对象类型。由于这些包含了构造器，所以可以通过一个构造器或者一个初始化器列表来初始化一个组合变量；并且一个初始化器中的一个元素可以是一个构造器。

如果一个初始化器是包围在花括号中的初始化器列表，那么正被声明的变量必须是一个向量、矩阵、数组或是一个结构体。

```cpp
int i = { 1 }; // 非法！i 不是一个组合
```

包围在一组匹配集的花括号中的一个初始化器列表应用到一个组合。这可以是正被声明的变量，或是包含在正被声明变量中的一个组合。来自初始化器列表中的独立的初始化器按次序地应用于组合的元素/成员。

如果组合具有一个向量类型，那么来自列表的初始化器从分量0开始，按次序地应用于向量的分量。初始化器的个数必须匹配分量个数。

如果组合具有一个矩阵类型，那么来自列表的初始化器必须是向量初始化器，并且从列0开始，按次序地应用于矩阵的列。初始化器的个数必须匹配列的个数。

如果组合具有一个结构体类型，那么来自列表的初始化器从第一个成员开始，按照该结构体中所声明的次序应用到结构体的成员。初始化器的个数必须匹配该结构体成员的个数。

应用这些规则，以下矩阵声明是等价的：

```glsl
mat2x2 a = mat2( vec2( 1.0, 0.0 ), vec2( 0.0, 1.0 ) );
mat2x2 b = { vec2( 1.0, 0.0 ), vec2( 0.0, 1.0 ) };
mat2x2 c = { { 1.0, 0.0 }, { 0.0, 1.0 } };
```

所有以下声明都会导致一个编译时错误。

```glsl
float a[2] = { 3.4, 4.2, 5.0 }; // 非法
vec2 b = { 1.0, 2.0, 3.0 }; // 非法
mat3x3 c = { vec3(0.0), vec3(1.0), vec3(2.0), vec3(3.0) }; // 非法
mat2x2 d = { 1.0, 0.0, 0.0, 1.0 }; // 非法！不能扁平化嵌套初始化器
struct {
    float a;
    int b;
} e = { 1.2, 2, 3 }; // 非法
```

在所有情况下，应用于一个对象的最里面的初始化器（即，不在花括号中包围的一个初始化器列表）必须具有与正被初始化的对象具有相同类型，或是根据“[隐式转换（Implicit Conversions）](#implicit_conversions)”可以被转换为该对象的类型。在后者情况下，在该赋值被完成前，对该初始化器完成一次隐式转换。

```cpp
struct {
    float a;
    int b;
} e = { 1.2, 2 }; // 合法！所有类型都匹配
struct {
    float a;
    int b;
} e = { 1, 3 }; // 合法！第一个初始化器被隐式转换
```

所有以下声明均将导致一个编译时错误。

```glsl
int a = true; // illegal
vec4 b[2] = { vec4(0.0), 1.0 }; // illegal
mat4x2 c = { vec3(0.0), vec3(1.0) }; // 非法
struct S1 {
    vec4 a;
    vec4 b;
};
struct {
    float s;
    float t;
} d[] = { S1(vec4(0.0), vec4(1.1)) }; // 非法
```

如果一个初始化器（或初始化器列表）提供给了一个未指定大小的数组，那么该数组的大小由在该初始化器内的顶层（非嵌套的）初始化器的个数来确定。所有以下声明创建了显式指定具有五个元素大小的数组：

```cpp
float a[] = float[](3.4, 4.2, 5.0, 5.2, 1.1);
float b[] = { 3.4, 4.2, 5.0, 5.2, 1.1 };
float c[] = a; // 给 c 显式指定了大小5
float d[5] = b; // 与上述一样
```

对于正被初始化的组合，如果一个初始化器列表中有过少或过多初始化器，那么会产生一个编译时错误。也就是，一个数组中的所有元素，一个结构体的所有成员，一个矩阵的所有列，以及一个向量的所有分量，必须恰好存在一个初始化器表达式，而不带有未消费的初始化器。

<br />

<a name="scoping"></a>
## 作用域（Scoping）

一个变量的作用域由它所声明的地方进行确定。如果它被声明在所有函数定义之外，那么它具有全局作用域，而此作用域从它被声明的地方开始，并一直持续到它所声明在的着色器的末尾。如果它被声明在了一个 **`while`** 测试或是一条 **`for`** 语句，那么其作用域是在后续子语句的末尾处。如果它被声明在一条 **`if`** 或 **`else`** 语句中，那么其作用域在那条语句的结尾处。其他情况下，如果它在一条复合语句内被声明为一条语句，那么其作用域是在那条复合语句的结尾处。如果它在一个函数定义中被声明为一个形参，那么其作用域一直到那个函数定义的结尾处。一个函数的形参声明和函数体一起形成嵌套在全局作用域中的一单个作用域。**`if`** 语句的表达式不允许声明新的变量，因而并不形成一个新的作用域。

在一个声明内，一个名字的作用域，如果存在一个初始化器，那么在其初始化器之后立即开始；如果不存在初始化器，那么立即在正被声明的名字之后。以下列出一些例子：

```glsl
int x = 1;
{
    int x = 2, y = x; // y 被初始化为 2
}
struct S
{
    int x;
};
{
    S S = S(0); // 'S' 仅作为一个结构体以及构造器可见
    S; // 'S' is now visible as a variable
}

int x = x; // 错误，倘若 x 在此之前没有被定义。而如果 x 之前的定义在该语句同一个作用域，则会引发一个重声明错误。

int f( /* 嵌套作用域在此开始 */ int k)
{
    int k = k + 3; // 名字 k 的重声明错误
}

int f(int k)
{
    {
        int k = k + 3; // 这里等号右边的 k 是函数 f 的形参, 这里用于初始化等号左边的 k
        int m = k; // 使用新声明的 k, 它隐藏了当前函数 f 的形参 k。
    }
}
```

对于 **`for`** 和 **`while`** 循环，子语句其本身并不对变量名引入一个新的作用域，因此以下代码具有一个重声明的编译时错误：

```cpp
for ( /* 嵌套作用域在此开始 */ int i = 0; i < 10; i++) {
    int i; // 重声明错误
}
```

一个 **`do-while`** 循环体引入了一个新的作用域，仅在 **`do`** 和 **`while`** 之间持续（并不包含 **`while`** 测试表达式），无论该循环体是单条简单语句还是复合语句：

```cpp
int i = 17;
do
    int i = 4; // OK，在嵌套作用域
while (i == 0); // 这里的 i 是17，其作用域在 do-while 体的外部
```

跟在一条 **`switch (...)`** 后面的语句形成一个嵌套的作用域。

所有在一个给定作用域中的变量名、结构体类型名、以及函数名共享同一个名字空间。函数名可以在同一作用域被重声明，且可以使用相同的或不同的形参，而不会引发错误。一个隐式指定大小的数组可以在同一作用域被重声明为具有相同基本类型的一个数组。否则，在一个编译单元内，一个所声明的名字不能在同一作用域被重声明；这么做将导致一个重声明的编译时错误。如果一个嵌套作用域重新声明了在外部作用域所使用的一个名字，那么这个名字将隐藏所有已存在的对那个名字的使用。没有方式能访问一个已被隐藏的名字或是将它不隐藏，而只能通过退出隐藏它的作用域。

内建函数的作用域是在用户声明全局变量所在的全局作用域之外。也就是一个着色器的全局作用域，对用户自定义的函数和全局变量可用，该全局作用域被嵌套在包含着内建函数的作用域内。当一个函数名在一个嵌套作用域中被重声明时，它隐藏了在外层作用域中用那个名字所声明的所有函数。函数声明（函数原型）不能在函数内部发生；它们必须在全局作用域，或者对于内建函数，在全局作用域外，否则会导致一个编译时错误。

共享全局变量是在同一个语言（即，相同的流水线阶段，比如顶点）内、在独立的编译单元（着色器）中，用相同的名字声明的全局变量，当制作一单个程序时，这些共享变量被连接在一起。（构成两个不同着色语言之间接口的全局变量在其他章节中介绍。）共享全局变量共享了相同的名字空间，并且必须用相同的类型来声明。它们将共享相同的存储。

共享的全局数组必须具有相同的基类型以及相同的显式大小。在一个着色器中隐式指定大小的一个数组可以在同样的流水线阶段中的另一个着色器进行显式指定大小。如果在一个流水线阶段中对于那个数组没有着色器具有一个显式大小，那么在那个流水线阶段中所使用的最大隐式大小（所使用的最大索引加1）被使用。没有跨阶段的数组大小指定。对一个隐式指定大小的数组，如果在声明它的流水线阶段内没有对它的静态访问，那么该数组的大小被给定为1。这与当数组在一个 interface block 内所声明的数组、而其他流水线阶段或当前应用程序共享该 interface block，相关（其他未使用的数组可能被优化器消除）。

共享的全局标量必须具有完全相同的类型名以及类型定义。结构体必须具有相同的名字、类型名的顺序和类型定义、以及要考虑为相同类型的成员名。这条规则递归地应用于嵌套或嵌入类型。如果一个共享全局变量具有多个初始化器，那么这些初始化器必须是常量表达式，并且它们必须全都具有相同的值。否则，会产生一个连接时错误。（一个仅含有一个初始化器的共享全局变量不要求其初始化器是一个常量表达式。）

<br />

<a name="storage_qualifiers"></a>
## 存储限定符（Storage Qualifiers）

变量声明在其类型之前可以最多只有一个存储限定符（storage qualifier）进行指定。这些如下表概述

存储限定符 | 含义 | 对应 SPIR-V 的存储类（Storage Class） | 描述
---- | ---- | ---- | ----
<无：默认>（修饰的变量定义在函数内） | 本地读/写存储器，或是一个函数的输入形参 | **`Function`** | 只在当前调用（线程）所声明的函数内可见。<br /> 正规的函数存储器。
<无：默认>（修饰的变量定义在当前编译单元的全局作用域内） | 本地读/写存储器 | **`Private`** | 对当前调用（线程）中的所有函数可见。<br /> 正规的全局存储器。
**`const`** | 一个变量，其值不能被改变 | N/A | 实现可能直接用 **`OpConstant`**、**`OpConstantComposite`** 等常量创建指令（Constant-Creation Instructions）进行优化。
**`in`** | 从先前的流水线阶段输入到一个着色器的连接，用此存储限定符修饰的变量被拷贝进来。 | **`Input`** | 来自流水线的输入。<br /> 对当前调用（线程）中跨所有函数可见。<br /> 用此存储类所声明的变量是只读的，并且不能具有初始化器。
**`out`** | 输出一个着色器的连接，输出到后续的一个流水线阶段，变量被拷贝出去。 | **`Output`** | 输出到流水线。对当前调用（线程）中跨所有函数可见。
**`uniform`**（非 **`uniform`** block） | 值不会跨正被处理的图元进行改变。<br /> **`uniform`** 对象在一个着色器、API、和应用之间形成连接。 | **`UniformConstant`** | 外部共享的，跨所有工作组中的所有调用（线程）中的所有函数可见。<br /> 图形 **`uniform`** 存储器。<br /> OpenCL常量存储器。<br /> 以此存储类所声明的变量是只读的。<br /> 它们可以具有初始化器，正如被客户端API所允许的那样。<br /> 比如：`layout(set = 0, binding = 1) uniform highp sampler2D imageSampler;`
**`uniform`**（**`uniform`** block） | 值不会跨正被处理的图元进行改变。<br /> **`uniform`** 对象在一个着色器、API、和应用之间形成连接。 | **`Uniform`** | 外部共享的，对跨所有工作组中的所有调用（线程）中的所有函数可见。<br /> 图形 **uniform** blocks（在SPIR-V 1.3版本前，还对应 buffer blocks）。<br /> 比如：`layout(set = 0, binding = 2) uniform MyBlock { uvec4 u_data; };`
**`buffer`** | 值被存放在一个缓存对象中，并且可以通过着色器调用（线程）以及API进行读写。 | **`StorageBuffer`** | 外部共享的，对跨所有工作组中的所有调用（线程）中的所有函数可读、可写、可见。 <br /> 图形 storage buffers （buffer blocks）。<br /> 此存储类仅从 SPIR-V 1.3版本起才可用。另外可参阅扩展：[VK_KHR_storage_buffer_storage_class](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_storage_buffer_storage_class.html)，[VK_KHR_variable_pointers](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_variable_pointers.html)
**`shared`** | 仅用于计算着色器；变量存储在一个工作组中跨所有工作项共享。 | **`Workgroup`** | 在一个工作组内跨所有调用（线程）共享。<br /> 跨所有函数可见。<br /> 对应GLSL中的 **`shared`** 存储限定符。<br /> 对应OpenCL **`local`** 存储器。

某些输入和输出限定的变量可以用最多一个额外的辅助存储限定符（auxiliary storage qualifier）进行限定。这些GLSL中的辅助限定符对应于 SPIR-V 中的装饰（**`Decoration`**）。装饰为一个 *<id>* 或是一个结构体的成员添加了额外的信息。

辅助存储限定符 | 含义 | 对应 SPIR-V 装饰 | 描述
---- | ---- | ---- | ----
**`centroid`** | 基于质心的插值 | **`Centroid`** | 必须只能用子一个存储器对象声明或是一个结构体类型的成员上。<br /> 如果与多重采样光栅化一起使用，那么允许为一整个像素给出一单个插值位置。<br /> 该插值位置同时坐落于该像素内以及正被光栅化的图元内。<br /> 仅针对 **`Input`** 和 **`Output`** 有效。
**`sample`** | 逐样本的插值 | **`Sample`** | 必须只能用在存储器对象声明或是一个结构体类型的一个成员上。<br /> 如果与多重采样光栅化一起使用，那么要求逐样本插值。插值位置则是同时坐落于像素和正被光栅化的图元中的样本的位置。仅对 **`Input`** 和 **`Output`** 有效。
**`patch`** | 逐细分曲面 patch 的属性 | **`Patch`** | 必须只能用在一个存储器对象声明或是一个结构体类型的一个成员上。<br /> 指示了一个细分 patch。仅对 **`Input`** 和 **`Output`** 存储类有效。<br /> 对由非细分曲面执行模型所引用的对象或类型进行使用则是无效的。

并非所有限定符的组合都被允许的。辅助存储限定符只能与 **`in`** 或 **`out`** 存储限定符一起使用。后续会有额外的限定符规则。

函数内的局部变量只能使用 **`const`** 存储限定符（或是不使用存储限定符）。

