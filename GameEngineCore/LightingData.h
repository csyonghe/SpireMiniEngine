#ifndef GAME_ENGINE_LIGHTING_DATA_H
#define GAME_ENGINE_LIGHTING_DATA_H

#include "CoreLib/VectorMath.h"
#include "CoreLib/Basic.h"
#include "HardwareRenderer.h"
#include "PipelineContext.h"
#include "Level.h"
#include "RenderProcedure.h"
#include "StandardViewUniforms.h"

namespace GameEngine
{
	const unsigned short GpuLightType_Point = 0;
	const unsigned short GpuLightType_Directional = 1;
	const unsigned short GpuLightType_Spot = 2;

	struct GpuLightData
	{
		unsigned short lightType;
		unsigned short shaderMapId;
		float radius;
		float decay;
		float startAngle;
		VectorMath::Vec3 position;
		float endAngle;
		VectorMath::Vec3 color;
		unsigned int direction;
		VectorMath::Matrix4 lightMatrix;
	};

	struct GpuLightProbeData
	{
		VectorMath::Vec3 position;
		float radius;
		VectorMath::Vec3 tintColor;
		int envMapId;
	};

	struct LightingUniform
	{
		VectorMath::Vec3 lightDir; int sunLightEnabled = 0;
		VectorMath::Vec3 lightColor;
		float ambient = 0.2f;
		int shadowMapId = -1;
		int numCascades = 0;
		int lightCount = 0, lightProbeCount = 0;
		VectorMath::Matrix4 lightMatrix[MaxShadowCascades];
		float zPlanes[MaxShadowCascades];
	};

	class LightingEnvironment
	{
	private:
		bool useEnvMap = true;
		CoreLib::RefPtr<TextureCubeArray> emptyEnvMapArray;
		void AddShadowPass(FrameRenderTask & tasks, WorldRenderPass * shadowRenderPass, DrawableSink * sink, ShadowMapResource & shadowMapRes, int shadowMapId,
			StandardViewUniforms & shadowMapView, int & shadowMapViewInstancePtr);
	public:
		DeviceMemory * uniformMemory;
		ModuleInstance moduleInstance;
		CoreLib::List<GpuLightData> lights;
		CoreLib::List<GpuLightProbeData> lightProbes;
		CoreLib::List<Texture*> lightProbeTextures;
		CoreLib::List<RefPtr<Texture2D>> shadowMaps;
		CoreLib::RefPtr<Buffer> lightBuffer, lightProbeBuffer;
		CoreLib::List<ModuleInstance> shadowViewInstances;
		CoreLib::List<Drawable*> drawableBuffer, reorderBuffer;
		RendererSharedResource * sharedRes;
		void* lightBufferPtr, *lightProbeBufferPtr;
		int lightBufferSize, lightProbeBufferSize;
		LightingUniform uniformData;
		void GatherInfo(FrameRenderTask & tasks, DrawableSink * sink, const RenderProcedureParameters & params, int w, int h, StandardViewUniforms & cameraView, WorldRenderPass * shadowPass);
		void Init(RendererSharedResource & sharedRes, DeviceMemory * uniformMemory, bool pUseEnvMap);
		void UpdateSharedResourceBinding();
	};
}

#endif