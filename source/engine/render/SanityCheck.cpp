#include <render/shaders/Compiler.h>
#include <render/backend/Driver.h>

static_assert(sizeof(render::backend::ShaderType) == sizeof(render::shaders::ShaderType));
static_assert(sizeof(render::backend::ShaderILType) == sizeof(render::shaders::ShaderILType));

static_assert(static_cast<int>(render::backend::ShaderType::MAX) == static_cast<int>(render::shaders::ShaderType::MAX));
static_assert(static_cast<int>(render::backend::ShaderILType::MAX) == static_cast<int>(render::shaders::ShaderILType::MAX));
