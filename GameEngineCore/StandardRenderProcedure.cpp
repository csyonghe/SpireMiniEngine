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
		int padding2 = 0, pad3 = 0;
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
		ViewResource * viewRes = nullptr;

		WorldRenderPass * shadowRenderPass = nullptr;
		WorldRenderPass * forwardRenderPass = nullptr;
		WorldRenderPass * gBufferRenderPass = nullptr;
		PostRenderPass * atmospherePass = nullptr;
		PostRenderPass * deferredLightingPass = nullptr;

		RenderOutput * forwardBaseOutput = nullptr;
		RenderOutput * transparentAtmosphereOutput = nullptr;

		RenderOutput * gBufferOutput = nullptr;
		RenderOutput * deferredLightingOutput = nullptr;
		StandardViewUniforms viewUniform;

		RenderPassInstance forwardBaseInstance, transparentPassInstance;
		RenderPassInstance gBufferInstance;

		DeviceMemory renderPassUniformMemory;
		SharedModuleInstances sharedModules;
		ModuleInstance forwardBasePassParams, lightingParams;
		CoreLib::List<ModuleInstance> shadowViewInstances;

		DrawableSink sink;

		List<DirectionalLightActor*> directionalLights;
		List<LightUniforms> lightingData;

		List<Drawable*> reorderBuffer, drawableBuffer;
	
		AtmosphereParameters lastAtmosphereParams;
		bool useAtmosphere = false;

	public:
		~StandardRenderProcedure()
		{
			if (forwardBaseOutput)
				viewRes->DestroyRenderOutput(forwardBaseOutput);
			if (gBufferOutput)
				viewRes->DestroyRenderOutput(gBufferOutput);
			if (deferredLightingOutput)
				viewRes->DestroyRenderOutput(deferredLightingOutput);
			if (transparentAtmosphereOutput)
				viewRes->DestroyRenderOutput(transparentAtmosphereOutput);

		}
		virtual RenderTarget* GetOutput() override
		{
			if (useAtmosphere)
				return viewRes->LoadSharedRenderTarget("litAtmosphereColor", StorageFormat::RGBA_8).Ptr();
			else
				return viewRes->LoadSharedRenderTarget("litColor", StorageFormat::RGBA_8).Ptr();
		}
		virtual void UpdateSharedResourceBinding() override
		{
			for (int i = 0; i < DynamicBufferLengthMultiplier; i++)
			{
				auto descSet = forwardBasePassParams.GetDescriptorSet(i);
				descSet->BeginUpdate();
				descSet->Update(1, sharedRes->textureSampler.Ptr());
				descSet->EndUpdate();
				descSet = lightingParams.GetDescriptorSet(i);
				descSet->BeginUpdate();
				descSet->Update(1, sharedRes->shadowMapResources.shadowMapArray.Ptr(), TextureAspect::Depth);
				descSet->Update(2, sharedRes->shadowSampler.Ptr());
				descSet->Update(3, sharedRes->envMap.Ptr(), TextureAspect::Color);
				descSet->EndUpdate();
			}
		}
		virtual void Init(Renderer * renderer, ViewResource * pViewRes) override
		{
			viewRes = pViewRes;
			sharedRes = renderer->GetSharedResource();
			deferred = Engine::Instance()->GetGraphicsSettings().UseDeferredRenderer;
			shadowRenderPass = CreateShadowRenderPass();
			renderer->RegisterWorldRenderPass(shadowRenderPass);
			if (deferred)
			{
				gBufferRenderPass = CreateGBufferRenderPass();
				renderer->RegisterWorldRenderPass(gBufferRenderPass);
				deferredLightingPass = CreateDeferredLightingPostRenderPass(viewRes);
				renderer->RegisterPostRenderPass(deferredLightingPass);
				gBufferOutput = viewRes->CreateRenderOutput(
					gBufferRenderPass->GetRenderTargetLayout(),
					viewRes->LoadSharedRenderTarget("baseColorBuffer", StorageFormat::RGBA_8),
					viewRes->LoadSharedRenderTarget("pbrBuffer", StorageFormat::RGBA_8),
					viewRes->LoadSharedRenderTarget("normalBuffer", StorageFormat::RGB10_A2),
					viewRes->LoadSharedRenderTarget("depthBuffer", DepthBufferFormat)
				);
				gBufferInstance = gBufferRenderPass->CreateInstance(gBufferOutput, true);
			}
			
			forwardRenderPass = CreateForwardBaseRenderPass();
			renderer->RegisterWorldRenderPass(forwardRenderPass);
			forwardBaseOutput = viewRes->CreateRenderOutput(
				forwardRenderPass->GetRenderTargetLayout(),
				viewRes->LoadSharedRenderTarget("litColor", StorageFormat::RGBA_8),
				viewRes->LoadSharedRenderTarget("depthBuffer", DepthBufferFormat)
			);
			transparentAtmosphereOutput = viewRes->CreateRenderOutput(
				forwardRenderPass->GetRenderTargetLayout(),
				viewRes->LoadSharedRenderTarget("litAtmosphereColor", StorageFormat::RGBA_8),
				viewRes->LoadSharedRenderTarget("depthBuffer", DepthBufferFormat)
			);
			
			atmospherePass = CreateAtmospherePostRenderPass(viewRes);
			renderer->RegisterPostRenderPass(atmospherePass);

			// initialize forwardBasePassModule and lightingModule
			renderPassUniformMemory.Init(sharedRes->hardwareRenderer.Ptr(), BufferUsage::UniformBuffer, true, 22, sharedRes->hardwareRenderer->UniformBufferAlignment());
			sharedRes->CreateModuleInstance(forwardBasePassParams, spFindModule(sharedRes->spireContext, "ForwardBasePassParams"), &renderPassUniformMemory);
			sharedRes->CreateModuleInstance(lightingParams, spFindModule(sharedRes->spireContext, "Lighting"), &renderPassUniformMemory);
			UpdateSharedResourceBinding();
			sharedModules.View = &forwardBasePassParams;
			sharedModules.Lighting = &lightingParams;
			shadowViewInstances.Reserve(1024);
		}

		ArrayView<Drawable*> GetDrawable(DrawableSink * objSink, bool transparent, CullFrustum cf, bool append = false)
		{
			if (!append)
				drawableBuffer.Clear();
			for (auto obj : objSink->GetDrawables(transparent))
				if (cf.IsBoxInFrustum(obj->Bounds))
					drawableBuffer.Add(obj);
			return drawableBuffer.GetArrayView();
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
				gBufferOutput->GetSize(w, h);
			}
			
			if (!deferred)
			{
				forwardBaseInstance = forwardRenderPass->CreateInstance(forwardBaseOutput, true);
			}

			shadowRenderPass->ResetInstancePool();
			
			auto shadowMapRes = params.renderer->GetSharedResource()->shadowMapResources;
			shadowMapRes.Reset();
			lightingData.Clear();
			GetDrawablesParameter getDrawableParam;
			viewUniform.CameraPos = params.view.Position;
			viewUniform.ViewTransform = params.view.Transform;
			getDrawableParam.CameraDir = params.view.GetDirection();
			Matrix4 mainProjMatrix;
			Matrix4::CreatePerspectiveMatrixFromViewAngle(mainProjMatrix,
				params.view.FOV, w / (float)h,
				params.view.ZNear, params.view.ZFar, ClipSpaceType::ZeroToOne);
			Matrix4::Multiply(viewUniform.ViewProjectionTransform, mainProjMatrix, viewUniform.ViewTransform);
			
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
					if (!(lastAtmosphereParams == atmosphere->Parameters))
					{
						atmosphere->Parameters.SunDir = atmosphere->Parameters.SunDir.Normalize();
						atmospherePass->SetParameters(&atmosphere->Parameters, sizeof(atmosphere->Parameters));
						lastAtmosphereParams = atmosphere->Parameters;
					}
				}
			}

			float aspect = w / (float)h;

			int shadowMapSize = Engine::Instance()->GetGraphicsSettings().ShadowMapResolution;
			float zmin = params.view.ZNear;
			int shadowMapViewInstancePtr = 0;
			auto camFrustum = params.view.GetFrustum(aspect);
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
							auto center = params.view.Position + params.view.GetDirection() * t;
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
								shadowMapPassModuleInstance = &shadowViewInstances[shadowMapViewInstancePtr++];
							}
							else
							{
								shadowViewInstances.Add(ModuleInstance());
								sharedRes->CreateModuleInstance(shadowViewInstances.Last(), spFindModule(sharedRes->spireContext, "ForwardBasePassParams"), &renderPassUniformMemory);
								shadowMapViewInstancePtr = shadowViewInstances.Count();
								shadowMapPassModuleInstance = &shadowViewInstances.Last();
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
							drawableBuffer.Clear();
							auto cullFrustum = CullFrustum(shadowMapView.InvViewProjTransform);
							GetDrawable(&sink, true, cullFrustum);
							GetDrawable(&sink, false, cullFrustum, true);
							pass.SetDrawContent(sharedRes->pipelineManager, reorderBuffer, drawableBuffer.GetArrayView());
							sharedRes->pipelineManager.PopModuleInstance();
							task.renderPasses.Add(pass);
						}
					}
				}
				lightingData.Add(lightData);
			}
			lightingParams.SetUniformData(lightingData.Buffer(), (int)(lightingData.Count()*sizeof(LightUniforms)));
			forwardBasePassParams.SetUniformData(&viewUniform, (int)sizeof(viewUniform));
			
			auto cameraCullFrustum = CullFrustum(params.view.GetFrustum(aspect));

			if (deferred)
			{
				gBufferRenderPass->Bind();
				sharedRes->pipelineManager.PushModuleInstance(&forwardBasePassParams);
				gBufferInstance.SetDrawContent(sharedRes->pipelineManager, reorderBuffer, GetDrawable(&sink, false, cameraCullFrustum));
				sharedRes->pipelineManager.PopModuleInstance();
				task.renderPasses.Add(gBufferInstance);
				task.renderPasses.Add(deferredLightingPass->CreateInstance(sharedModules));
			}
			else
			{
				forwardRenderPass->Bind();
				sharedRes->pipelineManager.PushModuleInstance(&forwardBasePassParams);
				sharedRes->pipelineManager.PushModuleInstance(&lightingParams);
				forwardBaseInstance.SetDrawContent(sharedRes->pipelineManager, reorderBuffer, GetDrawable(&sink, false, cameraCullFrustum));
				sharedRes->pipelineManager.PopModuleInstance();
				sharedRes->pipelineManager.PopModuleInstance();
				task.renderPasses.Add(forwardBaseInstance);
			}

			if (useAtmosphere)
			{
				task.renderPasses.Add(atmospherePass->CreateInstance(sharedModules));
			}
			task.sharedModuleInstances = sharedModules;
			
			// transparency pass
			reorderBuffer.Clear();
			for (auto drawable : GetDrawable(&sink, true, cameraCullFrustum))
			{
				reorderBuffer.Add(drawable);
			}
			if (reorderBuffer.Count())
			{
				reorderBuffer.Sort([=](Drawable* d1, Drawable* d2) { return d1->Bounds.Distance(params.view.Position) > d2->Bounds.Distance(params.view.Position); });
				if (useAtmosphere)
					transparentPassInstance = forwardRenderPass->CreateInstance(transparentAtmosphereOutput, false);
				else
					transparentPassInstance = forwardRenderPass->CreateInstance(forwardBaseOutput, false);

				forwardRenderPass->Bind();
				sharedRes->pipelineManager.PushModuleInstance(&forwardBasePassParams);
				sharedRes->pipelineManager.PushModuleInstance(&lightingParams);

				transparentPassInstance.SetFixedOrderDrawContent(sharedRes->pipelineManager, reorderBuffer.GetArrayView());
				sharedRes->pipelineManager.PopModuleInstance();
				sharedRes->pipelineManager.PopModuleInstance();
				task.renderPasses.Add(transparentPassInstance);
			}
		}
	};

	IRenderProcedure * CreateStandardRenderProcedure()
	{
		return new StandardRenderProcedure();
	}
}