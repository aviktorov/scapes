
//////////////////////////////////////////////////////////

RenderGraph *render_graph = new RenderGraph(device, world);
render_graph->registerPass<RenderPassPost>();
render_graph->registerPass<RenderPassGeometry>();
render_graph->registerPass<RenderPassSkyLight>();
render_graph->addParameter(...);
...
render_graph->addTexture(...);
...

RenderPassGeometry *pass_gbuffer = new RenderPassGeometry();
pass_gbuffer->addInput(..);
...
pass_gbuffer->addOutput(...);
pass_gbuffer->setFragmentShader(...);
pass_gbuffer->setVertexShader(...);

render_graph->addPass(pass_gbuffer);

//////////////////////////////////////////////////////////

render_graph->load("graph.json");

//////////////////////////////////////////////////////////

for (int i = 0; i < max_random_ssao_vectors; ++i)
	render_graph->setParameter("SSAO::OffsetVectors", i, ssao_vectors[i]);

render_graph->setParameter("SSAO::Radius", 42.0f);
render_graph->setParameter("SSAO::Samples", 16);
render_graph->setParameter("SSAO::Bias", 0.01f);

render_graph->setTexture("SSAO::NoiseTexture", ssao_noise_tex);

render_graph->setParameter("Camera::View", ...);

render_graph->setScreenTexture("LDR Color", swap_chain);

//////////////////////////////////////////////////////////

render_graph->render(command_buffer);

//////////////////////////////////////////////////////////

class RenderGraph
{
public:
	void init(
		scapes::foundation::render::Device *device,
		scapes::foundation::game::World *world
	)
	{
		for (IRenderPass *pass : render_passes)
			pass->init(device, world);

		create_bindsets_for_parameter_groups();
	}

	bool load(const URI &uri)
	{
		get_json();
		deserialize(json);
	}

	bool save(const URI &uri)
	{
		json = serialize();
		save(json);
	}

	bool deserialize(json)
	{
		...

		const char *type = parse_render_pass_type();
		IRenderPass *pass = registered_render_pass_types[type]();
		pass->deserialize(json);
	}

	json serialize()
	{
		...
	}

	void resize(uint32_t width, uint32_t height)
	{
		for (TextureResource &resource : texture_resources)
		{
			if (resource.texture_type != render_buffer)
				continue;

			recreate_render_buffer(resource, width, height);
		}

		for (IRenderPass *pass : render_passes)
			pass->invalidate();
	}

	void shutdown()
	{
		for (IRenderPass *pass : render_passes)
			pass->init(device, world);

		destroy_bindsets_for_parameter_groups();
	}

	void render(scapes::foundation::render::CommandBuffer *command_buffer)
	{
		update_dirty_parameter_groups();

		for (IRenderPass *pass : render_passes)
			pass->render(command_buffer);
	}

	removeAllParameters();
	removeAllTextures();

	addParameter(...);
	addTexture(...);

	removeParameter(name);
	removeTexture(name);

	removeRenderPass(index);
	addRenderPass(render_pass, index);

	template<typename T>
	void setParameter(const char *name, const T &value);

	template<typename T>
	void setParameter(const char *name, int index, const T &value);

	template<typename T>
	const T &getParameter(const char *name) const;

	template<typename T>
	const T &getParameter(const char *name, int index) const;

	template<typename T>
	SCAPES_INLINE void registerRenderPassType()
	{
		registered_render_pass_types.insert({TypeTraits<T>::name, &T::create});
	}

	void setTexture(const char *name, TextureHandle texture);
	TextureHandle getTexture(const char *name) const;

	void setScreenTexture(const char *name, scapes::foundation::render::SwapChain *swap_chain);

private:
	struct Parameter
	{
		size_t size {0};
		size_t offset {0};
		void *memory {nullptr};
	};

	struct TextureResource
	{
		int resolution_downscale {1};
		scapes::foundation::render::Format format {scapes::foundation::render::Format::UNDEFINED};
		texture_type {render_buffer, screen, texture};
		union
		{
			scapes::foundation::render::SwapChain *screen_texture {nullptr};
			scapes::foundation::render::Texture *render_buffer_or_texture {nullptr};
		};
	};

	struct GroupResource
	{
		scapes::foundation::render::UniformBuffer *gpu_resource {nullptr};
		scapes::foundation::render::BindSet *bindings {nullptr};
		std::vector<Parameter> parameters;
		bool dirty {true};
	};

	std::unordered_map<const char *, PFN_createRenderPass> registered_render_pass_types;
	std::vector<IRenderPass *> passes;

	void *parameters_memory {nullptr};
	std::unordered_map<hash_t, Parameter> parameter_lookup;

	std::vector<GroupResource *> group_resources;
	std::unordered_map<hash_t, GroupResource *> group_resource_lookup;

	std::vector<TextureResource *> texture_resource;
	std::unordered_map<hash_t, TextureResource *> texture_resource_lookup;

	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::game::World *world {nullptr};
};

class IRenderPass
{
public:
	virtual void init(
		scapes::foundation::render::Device *device,
		scapes::foundation::game::World *world
	) = 0;

	virtual void shutdown(
	) = 0;

	virtual void invalidate(
	) = 0;

	virtual void render(
		scapes::foundation::render::CommandBuffer *command_buffer
	) = 0;

	virtual bool deserialize(
		json
	) = 0;

	virtual json serialize(
	) = 0;
};

class IGraphicsRenderPass : public IRenderPass
{
public:
	void init(
		scapes::foundation::render::Device *d,
		scapes::foundation::game::World *w
	) override
	{
		device = d;
		world = w;

		render_pass = create_from_outputs_and_loadstoreclear_specs();
		pipeline_state = device->createPipelineState();

		invalidate(device);
	}

	void invalidate(
	) override
	{
		device->destroyBindSet(texture_bindings);

		device->destroyFrameBuffer(frame_buffer);
		frame_buffer = nullptr;

		frame_buffer = create_from_outputs();

		texture_bindings = create_from_texture_parameters();

		device->clearBindSets(pipeline_state);
		for (size_t i = 0; i < input_groups.size(); ++i)
			device->setBindSet(pipeline_state, i, input_groups[i]);
	
		device->setBindSet(pipeline_state, last, texture_bindings);
	}

	void shutdown(
	) override
	{
		device->destroyRenderPass(render_pass);
		render_pass = nullptr;

		device->destroyFrameBuffer(frame_buffer);
		frame_buffer = nullptr;

		device->destroyBindSet(texture_bindings);
		texture_bindings = nullptr;

		device->destroyPipelineState(pipeline_state);
		pipeline_state = nullptr;
	}

	addInput(name);
	removeAllInputs();
	removeInput(name);

	addOutput(name, loadop, storeop, clearvalue);
	removeAllOutputs();
	removeOutput(name);

protected:

	struct FullscreenQuad
	{
		scapes::visual::ShaderHandle vertex_shader;
		scapes::visual::MeshHandle mesh;
	};

	static FullscreenQuad fullscreen_quad;

	struct Output
	{
		hash_t id;
		scapes::foundation::render::RenderPassLoadOp load_op;
		scapes::foundation::render::RenderPassStoreOp store_op;
		scapes::foundation::render::RenderPassClearValue clear_value;
	};

	std::vector<hash_t> input_groups;
	std::vector<hash_t> input_textures;
	std::vector<Output> outputs;

	scapes::foundation::render::Device *device {nullptr};
	scapes::foundation::game::World *world {nullptr};

	scapes::foundation::render::BindSet *texture_bindings {nullptr};
	scapes::foundation::render::RenderPass *render_pass {nullptr}; // naive impl
	scapes::foundation::render::FrameBuffer *frame_buffer {nullptr}; // naive impl
	scapes::foundation::render::PipelineState *pipeline_state {nullptr};
};

class RenderPassGeometry : public IGraphicsRenderPass
{
public:
	static IRenderPass *create() { return new RenderPassGeometry(); }

	void init(
		scapes::foundation::render::Device *device,
		scapes::foundation::game::World *world
	) final
	{
		base::init(device, world);

		device->clearShaders(pipeline_state);
		device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, vertex_shader->shader);
		device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, fragment_shader->shader);
	}

	void render(
		scapes::foundation::render::CommandBuffer *command_buffer
	) final
	{
		device->beginRenderPass(command_buffer, render_pass, frame_buffer);

		auto renderable_query = scapes::foundation::game::Query<
			scapes::foundation::components::Transform,
			scapes::visual::components::Renderable
		>(world);

		renderable_query->begin();

		while (renderable_query->next())
		{
			uint32_t num_items = renderable_query->getNumComponents();
			scapes::foundation::components::Transform *transforms = renderable_query->getComponents<scapes::foundation::components::Transform>(0);
			scapes::visual::components::Renderable *renderables = renderable_query->getComponents<scapes::visual::components::Renderable>(1);

			for (uint32_t i = 0; i < num_items; ++i)
			{
				const scapes::foundation::components::Transform &transform = transforms[i];
				const scapes::visual::components::Renderable &renderable = renderables[i];
				const scapes::foundation::math::mat4 &node_transform = transform.transform;

				scapes::foundation::render::BindSet *material_bindings = renderable.material->bindings;

				device->clearVertexStreams(pipeline_state);
				device->setVertexStream(pipeline_state, 0, renderable.mesh->vertex_buffer);

				device->setBindSet(pipeline_state, material_binding, material_bindings);
				device->setPushConstants(pipeline_state, static_cast<uint8_t>(sizeof(scapes::foundation::math::mat4)), &node_transform);

				device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, renderable.mesh->index_buffer, renderable.mesh->num_indices);
			}
		}

		device->endRenderPass(command_buffer);
	}

	setFragmentShader(...);
	setVertexShader(...);

private:
	uint32_t material_binding {0};
	scapes::foundation::io::URI vertex_shader_uri;
	scapes::visual::ShaderHandle vertex_shader;

	scapes::foundation::io::URI fragment_shader_uri;
	scapes::visual::ShaderHandle fragment_shader;
};

class RenderPassSkylight : public IGraphicsRenderPass
{
public:
	void init(
		scapes::foundation::render::Device *device,
		scapes::foundation::game::World *world
	) final
	{
		base::init(device, world);

		device->clearShaders(pipeline_state);
		device->setShader(pipeline_state, scapes::foundation::render::ShaderType::VERTEX, vertex_shader->shader);
		device->setShader(pipeline_state, scapes::foundation::render::ShaderType::FRAGMENT, fragment_shader->shader);
	}

	void render(
		scapes::foundation::render::CommandBuffer *command_buffer
	) final
	{
		device->beginRenderPass(command_buffer, render_pass, frame_buffer);

		audo skylight_query = scapes::foundation::game::Query<
			scapes::visual::components::SkyLight
		>(world);

		skylight_query->begin();

		while (skylight_query->next())
		{
			uint32_t num_items = skylight_query->getNumComponents();
			scapes::visual::components::SkyLight *skylights = skylight_query->getComponents<components::SkyLight>(0);

			for (uint32_t i = 0; i < num_items; ++i)
			{
				const scapes::visual::components::SkyLight &light = skylights[i];

				device->clearShaders(pipeline_state);
				device->setShader(pipeline_state, scapes::foundation::render::ShaderType::VERTEX, light.vertex_shader->shader);
				device->setShader(pipeline_state, scapes::foundation::render::ShaderType::FRAGMENT, light.fragment_shader->shader);

				device->setBindSet(pipeline_state, light_binding, light.ibl_environment->bindings);

				device->clearVertexStreams(pipeline_state);
				device->setVertexStream(pipeline_state, 0, light.mesh->vertex_buffer);

				device->drawIndexedPrimitiveInstanced(command_buffer, pipeline_state, light.mesh->index_buffer, light.mesh->num_indices);
			}
		}

		device->endRenderPass(command_buffer);
	}

	setFragmentShader();
	setVertexShader();

private:
	uint32_t light_binding {0};
	scapes::foundation::io::URI vertex_shader_uri;
	scapes::visual::ShaderHandle vertex_shader;

	scapes::foundation::io::URI fragment_shader_uri;
	scapes::visual::ShaderHandle fragment_shader;
};

class RenderPassPost final : public IGraphicsRenderPass
{
public:
	void init(
		scapes::foundation::render::Device *device,
		scapes::foundation::game::World *world
	) final
	{
		base::init(device, world);

		device->clearShaders(pipeline_state);
		device->setShader(pipeline_state, foundation::render::ShaderType::VERTEX, fullscreen_quad->vertex_shader->shader);
		device->setShader(pipeline_state, foundation::render::ShaderType::FRAGMENT, fragment_shader->shader);

		device->clearVertexStreams(pipeline_state);
		device->setVertexStream(pipeline_state, 0, fullscreen_quad->mesh->vertex_buffer);
	}

	void render(
		scapes::foundation::render::CommandBuffer *command_buffer
	) final
	{
		device->beginRenderPass(command_buffer, render_pass, frame_buffer);

		device->drawIndexedPrimitiveInstanced(
			command_buffer,
			pipeline_state,
			fullscreen_quad->mesh->index_buffer,
			fullscreen_quad->mesh->num_indices
		);

		device->endRenderPass(command_buffer);
	}

	setFragmentShader();

private:
	scapes::foundation::io::URI fragment_shader_uri;
	scapes::visual::ShaderHandle fragment_shader;
};

//////////////////////////////////////////////////////////
