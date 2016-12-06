#include "WorldRenderPass.h"
#include "Material.h"

using namespace CoreLib;

namespace GameEngine
{
	void WorldRenderPass::Create()
	{
		clearCommandBuffer = hwRenderer->CreateCommandBuffer();
		AcquireRenderTargets();
		UpdateFrameBuffer();
	}
	
	CoreLib::RefPtr<PipelineInstance> WorldRenderPass::CreatePipelineStateObject(Material * material, Mesh * mesh, const DrawableSharedUniformBuffer & uniforms, DrawableType drawableType)
	{
		auto animModule = drawableType == DrawableType::Skeletal ? "SkeletalAnimation" : "NoAnimation";
		auto entryPoint = GetEntryPointShader().ReplaceAll("ANIMATION", animModule);

		StringBuilder identifierSB(128);
		identifierSB << material->ShaderFile << GetName() << "_" << mesh->GetVertexFormat().GetTypeId();
		auto identifier = identifierSB.ProduceString();

		auto meshVertexFormat = mesh->GetVertexFormat();

		auto pipelineClass = sceneRes->LoadMaterialPipeline(identifier, material, renderTargetLayout.Ptr(), meshVertexFormat, entryPoint);

		PipelineBinding pipelineBinding;
		if (drawableType == DrawableType::Static)
			pipelineBinding.BindUniformBuffer(0, sceneRes->staticTransformMemory.GetBuffer(),
			(int)(uniforms.transformUniform - (unsigned char*)sceneRes->staticTransformMemory.BufferPtr()),
				uniforms.transformUniformCount);
		else
			pipelineBinding.BindStorageBuffer(3, sceneRes->skeletalTransformMemory.GetBuffer(),
			(int)(uniforms.transformUniform - (unsigned char*)sceneRes->skeletalTransformMemory.BufferPtr()),
				uniforms.transformUniformCount);
		pipelineBinding.BindUniformBuffer(1, sharedRes->sysUniformBuffer.Ptr());
		if (uniforms.instanceUniformCount)
		{
			pipelineBinding.BindUniformBuffer(2, sceneRes->instanceUniformMemory.GetBuffer(),
				(int)(uniforms.instanceUniform - (unsigned char*)sceneRes->instanceUniformMemory.BufferPtr()),
				uniforms.instanceUniformCount);
		}
		int k = 0;
		Array<Texture2D*, MaxTextureBindings> textures;
		material->FillInstanceUniformBuffer([&](String tex) {textures.Add(sceneRes->LoadTexture(tex)); }, [](auto) {}, [](int) {});
		for (auto texture : textures)
			pipelineBinding.BindTexture(sharedRes->GetTextureBindingStart() + k++, texture, sharedRes->textureSampler.Ptr());

		return pipelineClass.pipeline->CreateInstance(pipelineBinding);
	}
}

