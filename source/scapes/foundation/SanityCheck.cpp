#include <scapes/foundation/shaders/Compiler.h>
#include <scapes/foundation/render/Device.h>

static_assert(sizeof(scapes::foundation::render::ShaderType) == sizeof(scapes::foundation::shaders::ShaderType));
static_assert(sizeof(scapes::foundation::render::ShaderILType) == sizeof(scapes::foundation::shaders::ShaderILType));

static_assert(static_cast<int>(scapes::foundation::render::ShaderType::MAX) == static_cast<int>(scapes::foundation::shaders::ShaderType::MAX));
static_assert(static_cast<int>(scapes::foundation::render::ShaderILType::MAX) == static_cast<int>(scapes::foundation::shaders::ShaderILType::MAX));
