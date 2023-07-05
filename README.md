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

