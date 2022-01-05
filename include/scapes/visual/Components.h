#pragma once

#include <scapes/foundation/math/Math.h>
#include <scapes/foundation/TypeTraits.h>

#include <scapes/visual/Fwd.h>

namespace scapes::visual::components
{
	/*
	 */
	struct SkyLight
	{
		IBLTextureHandle ibl_environment;
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
