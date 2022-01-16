BLAS
	Geometry[]
		Triangles[]
		Indices[]

TLAS
	SBT
	BLAS[]

RayTracePipeline
	ShaderModule[]
	ShaderGroup[]
		union
		{
			struct
			{
				int intersection;
				int anyhit;
				int closesthit;
			} hit;

			struct
			{
				int callable;
			} callable;

			struct
			{
				int raygen;
			} raygen;

			struct
			{
				int miss;
			} miss;
		}

	SBTData
		UBO raygen;
		UBO hit; (instance0_geometry0, instance0_geometry1, instance0_geometry2, ...)
		UBO miss;
		UBO callable;

API
	traceRays(raygen_ubo, hit_ubo, miss_ubo, callable_ubo, ...);

TLAS
	ShaderBindings
	{

	}

bind_set = device->createBindSet();
device->bindTopAccelerationStructure(bind_set, 0, tlas);

device->setBindSet(raytrace_pipeline, 0, bind_set);

device->clearShaders(raytrace_pipeline);
device->clearHitShaders(raytrace_pipeline);
device->clearMissShaders(raytrace_pipeline);
device->clearCallableShaders(raytrace_pipeline);

device->pushMissShader(raytrace_pipeline, shader);
device->pushHitShaders(raytrace_pipeline, closesthit, anyhit, intersection);
device->setRaygenShader(raytrace_pipeline, shader);

device->traceRays(command_buffer, raytrace_pipeline, ...);

BLAS1 -> MaterialBall (foundation_mesh, ball_mesh, cloth_mesh)
BLAS2 -> GroundPlane (quad_mesh)

TLAS ->
	sbt_instance_offset0, BLAS1
	sbt_instance_offset1, BLAS1
	sbt_instance_offset2, BLAS2

sbt
	raygen,
	instance0_materialball_foundation_mesh_closesthit | anyhit | intersection,
	instance0_materialball_ball_mesh_closesthit | anyhit | intersection,
	instance0_materialball_cloth_mesh_closesthit | anyhit | intersection,
	instance1_materialball_foundation_mesh_closesthit | anyhit | intersection,
	instance1_materialball_ball_mesh_closesthit | anyhit | intersection,
	instance1_materialball_cloth_mesh_closesthit | anyhit | intersection,
	instance2_groundplane_quad_mesh_closesthit | anyhit | intersection,
	...,
	miss0,
	miss1,
	...,
	missN,

raytracepipeline
	raygen
	hitgroup(instance0, foundation_mesh, closesthit1)
	hitgroup(instance0, ball_mesh, closesthit2)
	hitgroup(instance0, cloth_mesh, closesthit3)
	hitgroup(instance1, foundation_mesh, closesthit4)
	hitgroup(instance1, ball_mesh, closesthit5)
	hitgroup(instance1, cloth_mesh, closesthit6)
	hitgroup(instance2, quad_mesh, closesthit7)
	miss
