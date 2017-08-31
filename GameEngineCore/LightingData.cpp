#include "LightingData.h"
#include "LightActor.h"
#include "DirectionalLightActor.h"
#include "WorldRenderPass.h"
#include "Engine.h"

using namespace CoreLib;
using namespace VectorMath;

namespace GameEngine
{
	const int MaxLights = 1024;

	Vec3 UnpackDirection(unsigned int dir)
	{
		Vec3 rs;
		float alpha = ((dir >> 16) / 65535.0f) * Math::Pi * 2.0f - Math::Pi;
		float beta = ((dir & 65535) / 65535.0f) * Math::Pi - Math::Pi * 0.5f;
		rs.x = cos(alpha) * cos(beta);
		rs.z = sin(alpha) * cos(beta);
		rs.y = sin(beta);
		return rs;
	}
	unsigned int PackDirection(Vec3 dir)
	{
		float alpha = atan2(dir.z, dir.x);
		float beta = asin(dir.y);
		return ((unsigned int)(Math::Clamp(((alpha + Math::Pi) / (Math::Pi  * 2.0f)), 0.0f, 1.0f)*65535.0f) << 16) +
			(unsigned int)(Math::Clamp(((beta + Math::Pi * 0.5f) / Math::Pi), 0.0f, 1.0f)*65535.0f);
	}

	void GetDrawable(List<Drawable*> & drawableBuffer, DrawableSink * objSink, bool transparent, CullFrustum cf)
	{
		for (auto obj : objSink->GetDrawables(transparent))
		{
			if (!obj->CastShadow)
				continue;
			if (cf.IsBoxInFrustum(obj->Bounds))
				drawableBuffer.Add(obj);
		}
	}

	void LightingEnvironment::GatherInfo(FrameRenderTask & tasks, Renderer* renderer, DrawableSink * sink, const RenderProcedureParameters & params, int w, int h, StandardViewUniforms & viewUniform, Level* level, WorldRenderPass * shadowRenderPass)
	{
		lights.Clear();
		uniformData.sunLightEnabled = false;
		auto shadowMapRes = renderer->GetSharedResource()->shadowMapResources;
		shadowMapRes.Reset();
		CoreLib::Graphics::BBox levelBounds;
		levelBounds.Init();
		DirectionalLightActor * sunlight = nullptr;
		for (auto & actor : level->Actors)
		{
			auto actorType = actor.Value->GetEngineType();
			if (actorType == EngineActorType::Light)
			{
				auto light = dynamic_cast<LightActor*>(actor.Value.Ptr());
				if (light->lightType == LightType::Directional)
				{
					auto dirLight = (DirectionalLightActor*)(light);
					GpuLightData lightData;
					lightData.lightType = GpuLightType_Directional;
					lightData.color = dirLight->Color;
					lightData.direction = PackDirection(dirLight->Direction);
					auto localTransform = dirLight->GetLocalTransform();
					lightData.position = Vec3::Create(localTransform.values[12], localTransform.values[13], localTransform.values[14]);
					lightData.radius = 0.0f;
					lightData.startAngle = lightData.endAngle = 0.0f;
					lightData.shaderMapId = 0xFFFF;
					lightData.decay = 0.0f;
					if (dirLight->EnableCascadedShadows && !uniformData.sunLightEnabled)
					{
						uniformData.sunLightEnabled = true;
						sunlight = dirLight;
						uniformData.lightColor = lightData.color;
						uniformData.lightDir = dirLight->Direction;
						
					}
					else
					{
						lights.Add(lightData);
					}
				}
			}
		}
		tasks.AddTask(new ImageTransferRenderTask(ArrayView<Texture*>(), MakeArrayView(dynamic_cast<Texture*>(shadowMapRes.shadowMapArray.Ptr()))));
		float zmin = params.view.ZNear;
		int shadowMapViewInstancePtr = 0;
		int shadowMapSize = Engine::Instance()->GetGraphicsSettings().ShadowMapResolution;
		float aspect = w / (float)h;
		auto camFrustum = params.view.GetFrustum(aspect);

		if (uniformData.sunLightEnabled)
		{
			shadowRenderPass->Bind();
			int shadowMapStartId = shadowMapRes.AllocShadowMaps(sunlight->NumShadowCascades);
			uniformData.shadowMapId = shadowMapStartId;
			if (shadowMapStartId != -1)
			{
				auto dirLightLocalTrans = sunlight->GetLocalTransform();
				Vec3 dirLightPos = Vec3::Create(dirLightLocalTrans.values[12], dirLightLocalTrans.values[13], dirLightLocalTrans.values[14]);
				float zmax = sunlight->ShadowDistance;

				for (int i = 0; i < sunlight->NumShadowCascades; i++)
				{
					StandardViewUniforms shadowMapView;
					Vec3 viewZ = sunlight->Direction;
					Vec3 viewX, viewY;
					GetOrthoVec(viewX, viewZ);
					viewY = Vec3::Cross(viewZ, viewX);
					shadowMapView.CameraPos = viewUniform.CameraPos;
					shadowMapView.Time = viewUniform.Time;
					Matrix4::CreateIdentityMatrix(shadowMapView.ViewTransform);
					shadowMapView.ViewTransform.m[0][0] = viewX.x; shadowMapView.ViewTransform.m[1][0] = viewX.y; shadowMapView.ViewTransform.m[2][0] = viewX.z;
					shadowMapView.ViewTransform.m[0][1] = viewY.x; shadowMapView.ViewTransform.m[1][1] = viewY.y; shadowMapView.ViewTransform.m[2][1] = viewY.z;
					shadowMapView.ViewTransform.m[0][2] = viewZ.x; shadowMapView.ViewTransform.m[1][2] = viewZ.y; shadowMapView.ViewTransform.m[2][2] = viewZ.z;
					float iOverN = (i + 1) / (float)sunlight->NumShadowCascades;
					float zi = sunlight->TransitionFactor * zmin * pow(zmax / zmin, iOverN) + (1.0f - sunlight->TransitionFactor)*(zmin + (iOverN)*(zmax - zmin));
					uniformData.zPlanes[i] = zi;
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
					if (sunlight->Direction.x > 0)
					{
						levelBoundMax.x = levelBounds.Min.x;
						levelBoundMin.x = levelBounds.Max.x;
					}
					if (sunlight->Direction.y > 0)
					{
						levelBoundMax.y = levelBounds.Min.y;
						levelBoundMin.y = levelBounds.Max.y;
					}
					if (sunlight->Direction.z > 0)
					{
						levelBoundMax.z = levelBounds.Min.z;
						levelBoundMin.z = levelBounds.Max.z;
					}
					Matrix4 projMatrix;
					Matrix4::CreateOrthoMatrix(projMatrix, transformedCorner.x, transformedCorner.x + viewSize,
						transformedCorner.y + viewSize, transformedCorner.y, -Vec3::Dot(sunlight->Direction, levelBoundMin),
						-Vec3::Dot(sunlight->Direction, levelBoundMax), ClipSpaceType::ZeroToOne);

					Matrix4::Multiply(shadowMapView.ViewProjectionTransform, projMatrix, shadowMapView.ViewTransform);

					shadowMapView.ViewProjectionTransform.Inverse(shadowMapView.InvViewProjTransform);
					shadowMapView.ViewTransform.Inverse(shadowMapView.InvViewTransform);

					Matrix4 viewportMatrix;
					Matrix4::CreateIdentityMatrix(viewportMatrix);
					viewportMatrix.m[0][0] = 0.5f; viewportMatrix.m[3][0] = 0.5f;
					viewportMatrix.m[1][1] = 0.5f; viewportMatrix.m[3][1] = 0.5f;
					viewportMatrix.m[2][2] = 1.0f; viewportMatrix.m[3][2] = 0.0f;
					Matrix4::Multiply(uniformData.lightMatrix[i], viewportMatrix, shadowMapView.ViewProjectionTransform);
					auto pass = shadowRenderPass->CreateInstance(shadowMapRes.shadowMapRenderOutputs[i + shadowMapStartId].Ptr(), true);

					ModuleInstance * shadowMapPassModuleInstance = nullptr;
					if (shadowMapViewInstancePtr < shadowViewInstances.Count())
					{
						shadowMapPassModuleInstance = &shadowViewInstances[shadowMapViewInstancePtr++];
					}
					else
					{
						shadowViewInstances.Add(ModuleInstance());
						sharedRes->CreateModuleInstance(shadowViewInstances.Last(), spFindModule(sharedRes->spireContext, "ForwardBasePassParams"), uniformMemory);
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
					GetDrawable(drawableBuffer, sink, true, cullFrustum);
					GetDrawable(drawableBuffer, sink, false, cullFrustum);
					pass->SetDrawContent(sharedRes->pipelineManager, reorderBuffer, drawableBuffer.GetArrayView());
					sharedRes->pipelineManager.PopModuleInstance();
					tasks.AddTask(pass);
				}
			}
		}
		tasks.AddTask(new ImageTransferRenderTask(MakeArrayView(dynamic_cast<Texture*>(shadowMapRes.shadowMapArray.Ptr())), ArrayView<Texture*>()));
		uniformData.lightCount = lights.Count();
		moduleInstance.SetUniformData(&uniformData, sizeof(uniformData));
		auto lightPtr = (GpuLightData*)((char*)lightBufferPtr + moduleInstance.GetCurrentVersion() * lightBufferSize);
		memcpy(lightPtr, lights.Buffer(), lights.Count() * sizeof(GpuLightData));
	}


	LightingEnvironment::LightingEnvironment(RendererSharedResource & pSharedRes, DeviceMemory * pUniformMemory)
	{
		this->uniformMemory = pUniformMemory;
		this->sharedRes = &pSharedRes;
		sharedRes->CreateModuleInstance(moduleInstance, spFindModule(sharedRes->spireContext, "Lighting"), uniformMemory, sizeof(LightingUniform));
		lightBufferSize = Math::RoundUpToAlignment((int)sizeof(GpuLightData) * MaxLights, sharedRes->hardwareRenderer->UniformBufferAlignment());
		lightBuffer = sharedRes->hardwareRenderer->CreateMappedBuffer(GameEngine::BufferUsage::StorageBuffer, lightBufferSize * DynamicBufferLengthMultiplier);
		for (int i = 0; i < DynamicBufferLengthMultiplier; i++)
		{
			auto descSet = moduleInstance.GetDescriptorSet(i);
			descSet->BeginUpdate();
			descSet->Update(1, lightBuffer.Ptr(), lightBufferSize * i);
			descSet->Update(2, sharedRes->shadowMapResources.shadowMapArray.Ptr(), TextureAspect::Depth);
			descSet->Update(3, sharedRes->shadowSampler.Ptr());
			descSet->Update(4, sharedRes->envMap.Ptr(), TextureAspect::Color);
			descSet->EndUpdate();
		}
		lightBufferPtr = lightBuffer->Map();
	}

}
