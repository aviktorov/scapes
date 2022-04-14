#include <scapes/visual/shaders/Compiler.h>
#include <scapes/visual/hardware/Device.h>

static_assert(sizeof(scapes::visual::hardware::ShaderType) == sizeof(scapes::visual::shaders::ShaderType));
static_assert(sizeof(scapes::visual::hardware::ShaderILType) == sizeof(scapes::visual::shaders::ShaderILType));

static_assert(static_cast<int>(scapes::visual::hardware::ShaderType::MAX) == static_cast<int>(scapes::visual::shaders::ShaderType::MAX));
static_assert(static_cast<int>(scapes::visual::hardware::ShaderILType::MAX) == static_cast<int>(scapes::visual::shaders::ShaderILType::MAX));
