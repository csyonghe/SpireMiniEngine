#include "Engine.h"
#include "WorldRenderPass.h"
#include "PostRenderPass.h"
#include "Renderer.h"
#include "RenderPassRegistry.h"
#include "DirectionalLightActor.h"
#include "AtmosphereActor.h"
#include "FrustumCulling.h"

namespace GameEngine
{
	class StandardViewUniforms
	{
	public:
		VectorMath::Matrix4 ViewTransform, ViewProjectionTransform, InvViewTransform, InvViewProjTransform;
		VectorMath::Vec3 CameraPos;
		float Time;
	};

	class LightUniforms
	{
	public:
		VectorMath::Vec3 lightDir; float pad0 = 0.0f;
		VectorMath::Vec3 lightColor; float pad1 = 0.0f;
		int numCascades = 0;
		int shadowMapId = -1;
		int pad2 = 0, pad3 = 0;
		VectorMath::Matrix4 lightMatrix[MaxShadowCascades];
		float zPlanes[MaxShadowCascades];
		LightUniforms()
		{
			lightDir.SetZero();
			lightColor.SetZero();
			for (int i = 0; i < MaxShadowCascades; i++)
				zPlanes[i] = 0.0f;
		}
	};

	class StandardRenderProcedure : public IRenderProcedure
	{
	private:
		bool deferred = false;
		RendererSharedResource * sharedRes = nullptr;

		WorldRenderPass * shadowRenderPass = nullptr;
		WorldRenderPass * forwardRenderPass = nullptr;
		WorldRenderPass * gBufferRenderPass = nullptr;
		PostRenderPass * atmospherePass = nullptr;
		PostRenderPass * deferredLightingPass = nullptr;

		RenderOutput * forwardBaseOutput = nullptr;
		RenderOutput * gBufferOutput = nullptr;
		RenderOutput * deferredLightingOutput = nullptr;
		StandardViewUniforms viewUniform;

		RenderPassInstance forwardBaseInstance;
		RenderPassInstance gBufferInstance;

		DeviceMemory renderPassUniformMemory;
		SharedModuleInstances sharedModules;
		CoreLib::RefPtr<ModuleInstance> forwardBasePassParams, lightingParams;
		CoreLib::List<CoreLib::RefPtr<ModuleInstance>> shadowViewInstances;

		DrawableSink sink;

		List<DirectionalLightActor*> directionalLights;
		List<LightUniforms> lightingData;

		List<Drawable*> reorderBuffer;

		bool useAtmosphere = false;

	public:
		~StandardRenderProcedure()
		{
			if (forwardBaseOutput)
				sharedRes->DestroyRenderOutput(forwardBaseOutput);
			if (gBufferOutput)
				sharedRes->DestroyRenderOutput(gBufferOutput);
			if (deferredLightingOutput)
				sharedRes->DestroyRenderOutput(deferredLightingOutput);
		}
		virtual RenderTarget* GetOutput() override
		{
			if (useAtmosphere)
				return sharedRes->LoadSharedRenderTarget("litAtmosphereColor", StorageFormat::RGBA_8).Ptr();
			else
				return sharedRes->LoadSharedRenderTarget("litColor", StorageFormat::RGBA_8).Ptr();
		}
		virtual void Init(Renderer * renderer) override
		{
			sharedRes = renderer->GetSharedResource();
			deferred = Engine::Instance()->GetGraphicsSettings().UseDeferredRenderer;
			shadowRenderPass = CreateShadowRenderPass();
			renderer->RegisterWorldRenderPass(shadowRenderPass);
			if (deferred)
			{
				gBufferRenderPass = CreateGBufferRenderPass();
				renderer->RegisterWorldRenderPass(gBufferRenderPass);
				deferredLightingPass = CreateDeferredLightingPostRenderPass();
				renderer->RegisterPostRenderPass(deferredLightingPass);
				gBufferOutput = sharedRes->CreateRenderOutput(
					gBufferRenderPass->GetRenderTargetLayout(),
					sharedRes->LoadSharedRenderTarget("baseColorBuffer", StorageFormat::RGBA_8),
					sharedRes->LoadSharedRenderTarget("pbrBuffer", StorageFormat::RGBA_8),
					sharedRes->LoadSharedRenderTarget("normalBuffer", StorageFormat::RGB10_A2),
					sharedRes->LoadSharedRenderTarget("depthBuffer", DepthBufferFormat)
				);
				gBufferInstance = gBufferRenderPass->CreateInstance(gBufferOutput, true);
			}
			else
			{
				forwardRenderPass = CreateForwardBaseRenderPass();
				renderer->RegisterWorldRenderPass(forwardRenderPass);
				forwardBaseOutput = sharedRes->CreateRenderOutput(
					forwardRenderPass->GetRenderTargetLayout(),
					sharedRes->LoadSharedRenderTarget("litColor", StorageFormat::RGBA_8),
					sharedRes->LoadSharedRenderTarget("depthBuffer", DepthBufferFormat)
				);
				forwardBaseInstance = forwardRenderPass->CreateInstance(forwardBaseOutput, true);
			}

			atmospherePass = CreateAtmospherePostRenderPass();
			renderer->RegisterPostRenderPass(atmospherePass);

			// initialize forwardBasePassModule and lightingModule
			renderPassUniformMemory.Init(sharedRes->hardwareRenderer.Ptr(), BufferUsage::UniformBuffer, true, 22, sharedRes->hardwareRenderer->UniformBufferAlignment());
			forwardBasePassParams = sharedRes->CreateModuleInstance(spFindModule(sharedRes->spireContext, "ForwardBasePassParams"), &renderPassUniformMemory);
			lightingParams = sharedRes->CreateModuleInstance(spFindModule(sharedRes->spireContext, "Lighting"), &renderPassUniformMemory);
			for (int i = 0; i < DynamicBufferLengthMultiplier; i++)
			{
				auto descSet = forwardBasePassParams->GetDescriptorSet(i);
				descSet->BeginUpdate();
				descSet->Update(1, sharedRes->textureSampler.Ptr());
				descSet->EndUpdate();
				descSet = lightingParams->GetDescriptorSet(i);
				descSet->BeginUpdate();
				descSet->Update(1, sharedRes->shadowMapResources.shadowMapArray.Ptr(), TextureAspect::Depth);
				descSet->Update(2, sharedRes->shadowSampler.Ptr());
				descSet->EndUpdate();
			}
			sharedModules.View = forwardBasePassParams.Ptr();
			sharedModules.Lighting = lightingParams.Ptr();
		}
		virtual void Run(FrameRenderTask & task, const RenderProcedureParameters & params) override
		{
			int w = 0, h = 0;
			if (forwardRenderPass)
			{
				forwardRenderPass->ResetInstancePool();
				forwardBaseOutput->GetSize(w, h);
			}
			else if (gBufferRenderPass)
			{
				gBufferRenderPass->ResetInstancePool();
				gBufferOutput->GetSize(w, h);
			}
			shadowRenderPass->ResetInstancePool();
			
			auto shadowMapRes = params.renderer->GetSharedResource()->shadowMapResources;
			shadowMapRes.Reset();
			lightingData.Clear();
			GetDrawablesParameter getDrawableParam;
			CameraActor * camera = nullptr;
			if (params.cameras.Count())
			{
				camera = params.cameras[0];
				viewUniform.CameraPos = camera->GetPosition();
				viewUniform.ViewTransform = camera->GetLocalTransform();
				getDrawableParam.CameraDir = camera->GetDirection();
				Matrix4 projMatrix;
				Matrix4::CreatePerspectiveMatrixFromViewAngle(projMatrix,
					camera->FOV, w / (float)h,
					camera->ZNear, camera->ZFar, ClipSpaceType::ZeroToOne);
				Matrix4::Multiply(viewUniform.ViewProjectionTransform, projMatrix, viewUniform.ViewTransform);
			}
			else
			{
				viewUniform.CameraPos = Vec3::Create(0.0f); 
				getDrawableParam.CameraDir = Vec3::Create(0.0f, 0.0f, -1.0f);
				Matrix4::CreateIdentityMatrix(viewUniform.ViewTransform);
				Matrix4::CreatePerspectiveMatrixFromViewAngle(viewUniform.ViewProjectionTransform,
					75.0f, w / (float)h, 40.0f, 40000.0f, ClipSpaceType::ZeroToOne);
			}
			viewUniform.ViewTransform.Inverse(viewUniform.InvViewTransform);
			viewUniform.ViewProjectionTransform.Inverse(viewUniform.InvViewProjTransform);
			viewUniform.Time = Engine::Instance()->GetTime();

			getDrawableParam.CameraPos = viewUniform.CameraPos;
			getDrawableParam.rendererService = params.rendererService;
			getDrawableParam.sink = &sink;
			
			sink.Clear();
			
			directionalLights.Clear();
			
			CoreLib::Graphics::BBox levelBounds;
			levelBounds.Init();

			for (auto & actor : params.level->Actors)
			{
				levelBounds.Union(actor.Value->Bounds);
				actor.Value->GetDrawables(getDrawableParam);
				auto actorType = actor.Value->GetEngineType();
				if (actorType == EngineActorType::Light)
				{
					auto light = dynamic_cast<LightActor*>(actor.Value.Ptr());
					if (light->lightType == LightType::Directional)
					{
						directionalLights.Add((DirectionalLightActor*)(light));
					}
				}
				else if (actorType == EngineActorType::Atmosphere)
				{
					useAtmosphere = true;
					auto atmosphere = dynamic_cast<AtmosphereActor*>(actor.Value.Ptr());
					atmosphere->Parameters.SunDir = atmosphere->Parameters.SunDir.Normalize();
					atmospherePass->SetParameters(&atmosphere->Parameters, sizeof(atmosphere->Parameters));
				}
			}

			float aspect = w / (float)h;
			if (camera)
			{
				int shadowMapSize = Engine::Instance()->GetGraphicsSettings().ShadowMapResolution;
				float zmin = camera->ZNear;
				int shadowMapViewInstancePtr = 0;
				auto camFrustum = camera->GetFrustum(aspect);
				for (auto dirLight : directionalLights)
				{
					LightUniforms lightData;
					lightData.lightColor = dirLight->Color;
					lightData.lightDir = dirLight->Direction;
					lightData.numCascades = dirLight->EnableCascadedShadows ? dirLight->NumShadowCascades : 0;
					float zmax = dirLight->ShadowDistance;
					if (dirLight->EnableCascadedShadows && dirLight->NumShadowCascades > 0 && dirLight->NumShadowCascades <= MaxShadowCascades)
					{
						shadowRenderPass->Bind();
						int shadowMapStartId = shadowMapRes.AllocShadowMaps(dirLight->NumShadowCascades);
						lightData.shadowMapId = shadowMapStartId;
						if (shadowMapStartId != -1)
						{
							auto dirLightLocalTrans = dirLight->GetLocalTransform();
							Vec3 dirLightPos = Vec3::Create(dirLightLocalTrans.values[12], dirLightLocalTrans.values[13], dirLightLocalTrans.values[14]);
							for (int i = 0; i < dirLight->NumShadowCascades; i++)
							{
								StandardViewUniforms shadowMapView;
								Vec3 viewZ = dirLight->Direction;
								Vec3 viewX, viewY;
								GetOrthoVec(viewX, viewZ);
								viewY = Vec3::Cross(viewZ, viewX);
								shadowMapView.CameraPos = viewUniform.CameraPos;
								shadowMapView.Time = viewUniform.Time;
								Matrix4::CreateIdentityMatrix(shadowMapView.ViewTransform);
								shadowMapView.ViewTransform.m[0][0] = viewX.x; shadowMapView.ViewTransform.m[1][0] = viewX.y; shadowMapView.ViewTransform.m[2][0] = viewX.z;
								shadowMapView.ViewTransform.m[0][1] = viewY.x; shadowMapView.ViewTransform.m[1][1] = viewY.y; shadowMapView.ViewTransform.m[2][1] = viewY.z;
								shadowMapView.ViewTransform.m[0][2] = viewZ.x; shadowMapView.ViewTransform.m[1][2] = viewZ.y; shadowMapView.ViewTransform.m[2][2] = viewZ.z;
								float iOverN = (i+1) / (float)dirLight->NumShadowCascades;
								float zi = dirLight->TransitionFactor * zmin * pow(zmax / zmin, iOverN) + (1.0f - dirLight->TransitionFactor)*(zmin + (iOverN)*(zmax - zmin));
								lightData.zPlanes[i] = zi;
								auto verts = camFrustum.GetVertices(zmin, zi);
								float d1 = (verts[0] - verts[2]).Length2() * 0.25f;
								float d2 = (verts[4] - verts[6]).Length2() * 0.25f;
								float f = zi - zmin;
								float ti = Math::Min((d1 + d2 + f*f) / (2.0f*f), f);
								float t = zmin + ti;
								auto center = camera->GetPosition() + camera->GetDirection() * t;
								float radius = (verts[6] - center).Length();
								auto transformedCenter = shadowMapView.ViewTransform.TransformNormal(center);
								auto transformedCorner = transformedCenter - Vec3::Create(radius);
								float viewSize = radius * 2.0f;
								float texelSize = radius * 2.0f / shadowMapSize;

								transformedCorner.x = Math::FastFloor(transformedCorner.x / texelSize) * texelSize;
								transformedCorner.y = Math::FastFloor(transformedCorner.y / texelSize) * texelSize;
								transformedCorner.z = Math::FastFloor(transformedCorner.z / texelSize) * texelSize;

								Vec3 levelBoundMax = levelBounds.Max;
								Vec3 levelBoundMin = levelBounds.Min;
								if (dirLight->Direction.x > 0)
								{
									levelBoundMax.x = levelBounds.Min.x;
									levelBoundMin.x = levelBounds.Max.x;
								}
								if (dirLight->Direction.y > 0)
								{
									levelBoundMax.y = levelBounds.Min.y;
									levelBoundMin.y = levelBounds.Max.y;
								}
								if (dirLight->Direction.z > 0)
								{
									levelBoundMax.z = levelBounds.Min.z;
									levelBoundMin.z = levelBounds.Max.z;
								}
								Matrix4 projMatrix;
								Matrix4::CreateOrthoMatrix(projMatrix, transformedCorner.x, transformedCorner.x + viewSize,
									transformedCorner.y + viewSize, transformedCorner.y, -Vec3::Dot(dirLight->Direction, levelBoundMin), 
									-Vec3::Dot(dirLight->Direction, levelBoundMax), ClipSpaceType::ZeroToOne);

								Matrix4::Multiply(shadowMapView.ViewProjectionTransform, projMatrix, shadowMapView.ViewTransform);

								shadowMapView.ViewProjectionTransform.Inverse(shadowMapView.InvViewProjTransform);
								shadowMapView.ViewTransform.Inverse(shadowMapView.InvViewTransform);

								Matrix4 viewportMatrix;
								Matrix4::CreateIdentityMatrix(viewportMatrix);
								viewportMatrix.m[0][0] = 0.5f; viewportMatrix.m[3][0] = 0.5f;
								viewportMatrix.m[1][1] = 0.5f; viewportMatrix.m[3][1] = 0.5f;
								viewportMatrix.m[2][2] = 1.0f; viewportMatrix.m[3][2] = 0.0f;
								Matrix4::Multiply(lightData.lightMatrix[i], viewportMatrix, shadowMapView.ViewProjectionTransform);
								auto pass = shadowRenderPass->CreateInstance(shadowMapRes.shadowMapRenderOutputs[i + shadowMapStartId].Ptr(), true);

								ModuleInstance * shadowMapPassModuleInstance = nullptr;
								if (shadowMapViewInstancePtr < shadowViewInstances.Count())
								{
									shadowMapPassModuleInstance = shadowViewInstances[shadowMapViewInstancePtr++].Ptr();
								}
								else
								{
									shadowViewInstances.Add(sharedRes->CreateModuleInstance(spFindModule(sharedRes->spireContext, "ForwardBasePassParams"), &renderPassUniformMemory));
									shadowMapViewInstancePtr = shadowViewInstances.Count();
									shadowMapPassModuleInstance = shadowViewInstances.Last().Ptr();
									for (int j = 0; j < DynamicBufferLengthMultiplier; j++)
									{
										auto descSet = shadowMapPassModuleInstance->GetDescriptorSet(j);
										descSet->BeginUpdate();
										descSet->Update(1, sharedRes->textureSampler.Ptr());
										descSet->EndUpdate();
									}
								}
								shadowMapPassModuleInstance->SetUniformData(&shadowMapView, sizeof(shadowMapView));
								sharedRes->pipelineManager.PushModuleInstance(shadowMapPassModuleInstance);
								pass.SetDrawContent(sharedRes->pipelineManager, reorderBuffer, CullFrustum(shadowMapView.InvViewProjTransform), sink.GetDrawables());
								sharedRes->pipelineManager.PopModuleInstance();
								task.renderPasses.Add(pass);
							}
						}
					}
					lightingData.Add(lightData);
				}
			}
			lightingParams->SetUniformData(lightingData.Buffer(), (int)(lightingData.Count()*sizeof(LightUniforms)));
			forwardBasePassParams->SetUniformData(&viewUniform, (int)sizeof(viewUniform));
			
			if (deferred)
			{
				gBufferRenderPass->Bind();
				sharedRes->pipelineManager.PushModuleInstance(forwardBasePassParams.Ptr());
				gBufferInstance.SetDrawContent(sharedRes->pipelineManager, reorderBuffer, CullFrustum(camera->GetFrustum(aspect)), sink.GetDrawables());
				task.renderPasses.Add(gBufferInstance);
				task.postPasses.Add(deferredLightingPass);
			}
			else
			{
				forwardRenderPass->Bind();
				sharedRes->pipelineManager.PushModuleInstance(forwardBasePassParams.Ptr());
				sharedRes->pipelineManager.PushModuleInstance(lightingParams.Ptr());
				forwardBaseInstance.SetDrawContent(sharedRes->pipelineManager, reorderBuffer, CullFrustum(camera->GetFrustum(aspect)), sink.GetDrawables());
				sharedRes->pipelineManager.PopModuleInstance();
				task.renderPasses.Add(forwardBaseInstance);
			}
			sharedRes->pipelineManager.PopModuleInstance();

			if (useAtmosphere)
			{
				task.postPasses.Add(atmospherePass);
			}
			task.sharedModuleInstances = sharedModules;
		}

		virtual void ResizeFrame(int w, int h) override
		{
			atmospherePass->Resize(w, h);
			if (deferred)
				deferredLightingPass->Resize(w, h);
		}
	};

	IRenderProcedure * CreateStandardRenderProcedure()
	{
		return new StandardRenderProcedure();
	}
}