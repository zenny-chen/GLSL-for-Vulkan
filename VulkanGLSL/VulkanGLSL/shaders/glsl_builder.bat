%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o basic.spv  basic.comp.glsl
%VK_SDK_PATH%/Bin/glslangValidator  -V100  --target-env spirv1.3  -Os  -o advanced.spv  advanced.comp.glsl
:: Generate spvasm disassembled sources
%VK_SDK_PATH%/Bin/spirv-dis  -o basic.spvasm  basic.spv
%VK_SDK_PATH%/Bin/spirv-dis  -o advanced.spvasm  advanced.spv

