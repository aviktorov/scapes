#pragma once

#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/TypeTraits.h>

#include <scapes/visual/Fwd.h>

namespace scapes::visual::components
{
	// TODO: move to foundation components
	/*
	 */
	struct Transform
	{
		foundation::math::mat4 transform;
	};

	template<>
	struct ::TypeTraits<Transform>
	{
		static constexpr const char *name = "scapes::visual::components::Transform";
	};

	/*
	 */
	struct SkyLight
	{
		IBLTextureHandle ibl_environment;
		MeshHandle mesh;
		ShaderHandle vertex_shader;
		ShaderHandle fragment_shader;
	};

	template<>
	struct ::TypeTraits<SkyLight>
	{
		static constexpr const char *name = "scapes::visual::components::SkyLight";
	};

	/*
	 */
	struct Renderable
	{
		MeshHandle mesh;
		RenderMaterialHandle material;
	};

	template<>
	struct ::TypeTraits<Renderable>
	{
		static constexpr const char *name = "scapes::visual::components::Renderable";
	};
}
