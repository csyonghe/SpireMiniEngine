#include "RenderContext.h"
#include "Material.h"
#include "Mesh.h"
#include "Engine.h"
#include "TextureCompressor.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Graphics/TextureFile.h"
#include <assert.h>

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::IO;
	using namespace VectorMath;

	String GetSpireOutput(SpireDiagnosticSink * sink)
	{
		int bufferSize = spGetDiagnosticOutput(sink, nullptr, 0);
		List<char> buffer;
		buffer.SetSize(bufferSize);
		spGetDiagnosticOutput(sink, buffer.Buffer(), buffer.Count());
		return String(buffer.Buffer());
	}

	void Drawable::UpdateMaterialUniform()
	{
		if (material->ParameterDirty)
		{
			material->ParameterDirty = false;
			auto update = [=](ModuleInstance * moduleInstance)
			{
				if (moduleInstance->BufferLength)
				{
					unsigned char * ptr = moduleInstance->UniformPtr;
					auto end = ptr + moduleInstance->BufferLength;
					material->FillInstanceUniformBuffer([](const String&) {},
						[&](auto & val)
					{
						if (ptr + sizeof(val) > end)
							throw InvalidOperationException("insufficient buffer.");
						*((decltype(&val))ptr) = val;
						ptr += sizeof(val);
					},
						[&](int alignment)
					{
						if (auto m = (int)((long long)(void*)ptr) % alignment)
						{
							ptr += (alignment - m);
						}
					}
					);
					scene->instanceUniformMemory.Sync(moduleInstance->UniformPtr, moduleInstance->BufferLength);
				}
			};
			update(material->MaterialGeometryModule.Ptr());
			update(material->MaterialPatternModule.Ptr());
		}
	}

	void Drawable::UpdateTransformUniform(const VectorMath::Matrix4 & localTransform)
	{
		if (type != DrawableType::Static)
			throw InvalidOperationException("cannot update non-static drawable with static transform data.");
		if (!transformModule->UniformPtr)
			throw InvalidOperationException("invalid buffer.");

		Vec4 * bufferPtr = (Vec4*)transformModule->UniformPtr;
		*((Matrix4*)bufferPtr) = localTransform; // write(localTransform)
		bufferPtr += 4;

		Matrix4 normMat;
		localTransform.Inverse(normMat);
		normMat.Transpose();
		*(bufferPtr++) = *((Vec4*)(normMat.values));
		*(bufferPtr++) = *((Vec4*)(normMat.values + 4));
		*(bufferPtr++) = *((Vec4*)(normMat.values + 8));
		if (transformModule->BufferLength)
			scene->transformMemory.Sync(transformModule->UniformPtr, transformModule->BufferLength);
	}

	void Drawable::UpdateTransformUniform(const VectorMath::Matrix4 & localTransform, const Pose & pose)
	{
		if (type != DrawableType::Skeletal)
			throw InvalidOperationException("cannot update static drawable with skeletal transform data.");
		if (!transformModule->UniformPtr)
			throw InvalidOperationException("invalid buffer.");

		const int poseMatrixSize = skeleton->Bones.Count() * (sizeof(Vec4) * 7);

		// ensure allocated transform buffer is sufficient
		_ASSERT(transformModule->BufferLength >= poseMatrixSize);

		List<Matrix4> matrices;
		pose.GetMatrices(skeleton, matrices);
		
		Vec4 * bufferPtr = (Vec4*)transformModule->UniformPtr;

		for (int i = 0; i < matrices.Count(); i++)
		{
			Matrix4::Multiply(matrices[i], localTransform, matrices[i]);
			*((Matrix4*)bufferPtr) = matrices[i]; // write(matrices[i])
			bufferPtr += 4;
			Matrix4 normMat;
			matrices[i].Inverse(normMat);
			normMat.Transpose();
			*(bufferPtr++) = *((Vec4*)(normMat.values));
			*(bufferPtr++) = *((Vec4*)(normMat.values + 4));
			*(bufferPtr++) = *((Vec4*)(normMat.values + 8));
		}
		if (transformModule->BufferLength)
			scene->transformMemory.Sync(transformModule->UniformPtr, transformModule->BufferLength);
	}
    RefPtr<DrawableMesh> SceneResource::CreateDrawableMesh(Mesh * mesh)
    {
        RefPtr<DrawableMesh> result = new DrawableMesh(rendererResource);
        result->vertexBufferOffset = (int)((char*)rendererResource->vertexBufferMemory.Alloc(mesh->GetVertexCount() * mesh->GetVertexSize()) - (char*)rendererResource->vertexBufferMemory.BufferPtr());
        result->indexBufferOffset = (int)((char*)rendererResource->indexBufferMemory.Alloc(mesh->Indices.Count() * sizeof(mesh->Indices[0])) - (char*)rendererResource->indexBufferMemory.BufferPtr());
        result->vertexFormat = rendererResource->pipelineManager.LoadVertexFormat(mesh->GetVertexFormat());
        result->vertexCount = mesh->GetVertexCount();
        rendererResource->indexBufferMemory.GetBuffer()->SetData(result->indexBufferOffset, mesh->Indices.Buffer(), mesh->Indices.Count() * sizeof(mesh->Indices[0]));
        rendererResource->vertexBufferMemory.GetBuffer()->SetData(result->vertexBufferOffset, mesh->GetVertexBuffer(), mesh->GetVertexCount() * result->vertexFormat.Size());
        result->indexCount = mesh->Indices.Count();
        return result;
    }
	RefPtr<DrawableMesh> SceneResource::LoadDrawableMesh(Mesh * mesh)
	{
		RefPtr<DrawableMesh> result;
		if (meshes.TryGetValue(mesh, result))
		    return result;

        result = CreateDrawableMesh(mesh);
		meshes[mesh] = result;
		return result;
	}
	Texture2D * SceneResource::LoadTexture2D(const String & name, CoreLib::Graphics::TextureFile & data)
	{
		RefPtr<Texture2D> value;
		if (textures.TryGetValue(name, value))
			return value.Ptr();
		StorageFormat format;
		DataType dataType = DataType::Byte4;
		switch (data.GetFormat())
		{
		case CoreLib::Graphics::TextureStorageFormat::R8:
			format = StorageFormat::Int8;
			dataType = DataType::Byte;
			break;
		case CoreLib::Graphics::TextureStorageFormat::RG8:
			format = StorageFormat::RG_I8;
			dataType = DataType::Byte2;
			break;
		case CoreLib::Graphics::TextureStorageFormat::RGB8:
			format = StorageFormat::RGB_I8;
			dataType = DataType::Byte3;
			break;
		case CoreLib::Graphics::TextureStorageFormat::RGBA8:
			format = StorageFormat::RGBA_8;
			dataType = DataType::Byte4;
			break;
		case CoreLib::Graphics::TextureStorageFormat::R_F32:
			format = StorageFormat::Float32;
			dataType = DataType::Float;
			break;
		case CoreLib::Graphics::TextureStorageFormat::RG_F32:
			format = StorageFormat::RG_F32;
			dataType = DataType::Float2;
			break;
		case CoreLib::Graphics::TextureStorageFormat::RGB_F32:
			format = StorageFormat::RGB_F32;
			dataType = DataType::Float3;
			break;
		case CoreLib::Graphics::TextureStorageFormat::RGBA_F32:
			format = StorageFormat::RGBA_F32;
			dataType = DataType::Float4;
			break;
		case CoreLib::Graphics::TextureStorageFormat::BC1:
			format = StorageFormat::BC1;
			break;
		case CoreLib::Graphics::TextureStorageFormat::BC5:
			format = StorageFormat::BC5;
			break;
		default:
			throw NotImplementedException("unsupported texture format.");
		}

		auto hw = rendererResource->hardwareRenderer.Ptr();

		GameEngine::Texture2D* rs;
		if (format == StorageFormat::BC1 || format == StorageFormat::BC5)
		{
			Array<void*, 32> mipData;
			for(int level = 0; level < data.GetMipLevels(); level++)
				mipData.Add(data.GetData(level).Buffer());
			rs = hw->CreateTexture2D(TextureUsage::Sampled, data.GetWidth(), data.GetHeight(), data.GetMipLevels(), format, dataType, mipData.GetArrayView());
		}
		else
			rs = hw->CreateTexture2D(data.GetWidth(), data.GetHeight(), format, dataType, data.GetData(0).Buffer());
		textures[name] = rs;
		return rs;
	}
	Texture2D * SceneResource::LoadTexture(const String & filename)
	{
		RefPtr<Texture2D> value;
		if (textures.TryGetValue(filename, value))
			return value.Ptr();

		auto actualFilename = Engine::Instance()->FindFile(Path::ReplaceExt(filename, "texture"), ResourceType::Texture);
		if (!actualFilename.Length())
			actualFilename = Engine::Instance()->FindFile(filename, ResourceType::Texture);
		if (actualFilename.Length())
		{
			if (actualFilename.ToLower().EndsWith(".texture"))
			{
				CoreLib::Graphics::TextureFile file(actualFilename);
				return LoadTexture2D(filename, file);
			}
			else
			{
				CoreLib::Imaging::Bitmap bmp(actualFilename);
				List<unsigned int> pixelsInversed;
				int * sourcePixels = (int*)bmp.GetPixels();
				pixelsInversed.SetSize(bmp.GetWidth() * bmp.GetHeight());
				for (int i = 0; i < bmp.GetHeight(); i++)
				{
					for (int j = 0; j < bmp.GetWidth(); j++)
						pixelsInversed[i*bmp.GetWidth() + j] = sourcePixels[(bmp.GetHeight() - 1 - i)*bmp.GetWidth() + j];
				}
				CoreLib::Graphics::TextureFile texFile;
				TextureCompressor::CompressRGBA_BC1(texFile, MakeArrayView((unsigned char*)pixelsInversed.Buffer(), pixelsInversed.Count() * 4), bmp.GetWidth(), bmp.GetHeight());
				texFile.SaveToFile(Path::ReplaceExt(actualFilename, "texture"));
				return LoadTexture2D(filename, texFile);
			}
		}
		else
		{
			Print("cannot load texture '%S'\n", filename.ToWString());
			CoreLib::Graphics::TextureFile errTex;
			unsigned char errorTexContent[] =
			{
				255,0,255,255,   0,0,0,255,
				0,0,0,255,       255,0,255,255
			};
			errTex.SetData(CoreLib::Graphics::TextureStorageFormat::RGBA8, 2, 2, 0, ArrayView<unsigned char>(errorTexContent, 16));
			return LoadTexture2D("ERROR_TEXTURE", errTex);
		}
	}

	RefPtr<ModuleInstance> SceneResource::CreateMaterialModuleInstance(Material* material, const char * moduleName)
	{
		bool isValid = true;
		auto module = spFindModule(spireContext, moduleName);
		if (!module)
		{
			Print("Invalid material(%S): shader '%S' does not define '%s'.", material->Name.ToWString(), material->ShaderFile.ToWString(), moduleName);
			return nullptr;
		}
		else
		{
			int paramCount = spModuleGetParameterCount(module);
			EnumerableDictionary<String, int> bindingLocs;
			EnumerableDictionary<String, DynamicVariable> vars;
			
			int loc = 1;
			for (int i = 0; i < paramCount; i++)
			{
				SpireComponentInfo param;
				spModuleGetParameter(module, i, &param);
				if (param.BindableResourceType != SPIRE_NON_BINDABLE)
				{
					bindingLocs[param.Name] = loc++;
				}
				else
				{
					DynamicVariable val;
					if (!material->Variables.TryGetValue(param.Name, val))
					{
						Print("Invalid material(%S): shader parameter '%S' is not provided in material file.\n", material->Name.ToWString(), String(param.Name).ToWString());
						isValid = false;
					}
					val.Name = param.Name;
					vars[val.Name] = val;
				}
			}
			if (isValid)
			{
				RefPtr<ModuleInstance> result = rendererResource->CreateModuleInstance(module, &instanceUniformMemory);
				if (result)
				{
					result->Descriptors->BeginUpdate();
					for (auto binding : bindingLocs)
					{
						DynamicVariable val;
						if (material->Variables.TryGetValue(binding.Key, val))
						{
							auto tex = LoadTexture(val.StringValue);
							if (tex)
								result->Descriptors->Update(binding.Value, tex);
						}
						else
						{
							Print("Invalid material(%S): shader parameter '%S' is not provided in material file.\n", material->Name.ToWString(), binding.Key.ToWString());
							result->Descriptors->Update(binding.Value, LoadTexture("error.texture"));
						}
					}
					result->Descriptors->EndUpdate();
				}
				material->Variables = _Move(vars);
				return result;
			}
			return nullptr;
		}
	}

	void SceneResource::RegisterMaterial(Material * material)
	{
		auto shaderFile = Engine::Instance()->FindFile(material->ShaderFile, ResourceType::Shader);
		if (shaderFile.Length())
		{
			SpireDiagnosticSink * spireSink = spCreateDiagnosticSink(spireContext);
			spLoadModuleLibrary(spireContext, shaderFile.Buffer(), spireSink);
			if (spDiagnosticSinkHasAnyErrors(spireSink))
				Print("Invalid material(%S): cannot compile shader '%S'. Output message:\n%S", material->Name.ToWString(), shaderFile.ToWString(), GetSpireOutput(spireSink).ToWString());
			else
			{
				material->MaterialPatternModule = CreateMaterialModuleInstance(material, (Path::GetFileNameWithoutEXT(shaderFile) + "Pattern").Buffer());
				auto geometryModuleName = Path::GetFileNameWithoutEXT(shaderFile) + "Geometry";
				if (spFindModule(spireContext, geometryModuleName.Buffer()))
					material->MaterialGeometryModule = CreateMaterialModuleInstance(material, geometryModuleName.Buffer());
			}
			spDestroyDiagnosticSink(spireSink);
		}
		// use default material if failed to load
		if (!material->MaterialPatternModule)
		{
			if (!defaultMaterialPatternModule)
			{
				defaultMaterialPatternModule = CreateMaterialModuleInstance(material, "DefaultPattern");
			}
			material->MaterialPatternModule = defaultMaterialPatternModule;
			material->Variables.Clear();
			if (!material->MaterialPatternModule)
				throw InvalidOperationException("failed to load default material.");
		}
		// use default geometry if failed to load
		if (!material->MaterialGeometryModule)
		{
			if (!defaultMaterialGeometryModule)
			{
				defaultMaterialGeometryModule = CreateMaterialModuleInstance(material, "DefaultGeometry");
			}
			material->MaterialGeometryModule = defaultMaterialGeometryModule;
			material->Variables.Clear();
			if (!material->MaterialGeometryModule)
				throw InvalidOperationException("failed to load default material.");
		}
	}
	
	SceneResource::SceneResource(RendererSharedResource * resource, SpireCompilationContext * spireCtx)
		: rendererResource(resource), spireContext(spireCtx)
	{
		auto hwRenderer = resource->hardwareRenderer.Ptr();
		instanceUniformMemory.Init(hwRenderer, BufferUsage::UniformBuffer, 24, hwRenderer->UniformBufferAlignment());
		transformMemory.Init(hwRenderer, BufferUsage::UniformBuffer, 25, hwRenderer->UniformBufferAlignment());
		spPushContext(spireContext);
		Clear();
	}
	
	void SceneResource::Clear()
	{
		meshes = CoreLib::EnumerableDictionary<Mesh*, RefPtr<DrawableMesh>>();
		textures = EnumerableDictionary<String, RefPtr<Texture2D>>();
		spPopContext(spireContext);
		spPushContext(spireContext);
		SpireDiagnosticSink * spireSink = spCreateDiagnosticSink(spireContext);
		spLoadModuleLibrary(spireContext, Engine::Instance()->FindFile("Default.shader", ResourceType::Shader).Buffer(), spireSink);
		if (spDiagnosticSinkHasAnyErrors(spireSink))
			throw InvalidOperationException("cannot compile DefaultPattern.shader");
		spDestroyDiagnosticSink(spireSink);
	}
	void RendererSharedResource::UpdateRenderResultFrameBuffer(RenderOutput * output)
	{
		RenderAttachments attachments;
		for (int i = 0; i < output->bindings.Count(); i++)
		{
			if (output->bindings[i]->Texture)
				attachments.SetAttachment(i, output->bindings[i]->Texture.Ptr());
			else if (output->bindings[i]->TextureArray)
				attachments.SetAttachment(i, output->bindings[i]->TextureArray.Ptr(), output->bindings[i]->Layer);
		}
		if (attachments.attachments.Count())
			output->frameBuffer = output->renderTargetLayout->CreateFrameBuffer(attachments);
	}

	// Converts StorageFormat to DataType
	DataType GetStorageDataType(StorageFormat format)
	{
		DataType dataType;
		switch (format)
		{
		case StorageFormat::RGBA_8:
			dataType = DataType::Byte4;
			break;
		case StorageFormat::Depth24Stencil8:
			dataType = DataType::Float;
			break;
		case StorageFormat::Depth32:
			dataType = DataType::UInt;
			break;
		case StorageFormat::RGBA_F16:
			dataType = DataType::Float4;
			break;
		case StorageFormat::RGB10_A2:
			dataType = DataType::Float4;
			break;
		case StorageFormat::R11F_G11F_B10F:
			dataType = DataType::Float3;
			break;
		default:
			throw HardwareRendererException("Unsupported storage format as render target.");
		}
		return dataType;
	}

	void ShadowMapResource::Init(HardwareRenderer * hwRenderer, RendererSharedResource * res)
	{
		auto & graphicsSettings = Engine::Instance()->GetGraphicsSettings();

		shadowMapArray = hwRenderer->CreateTexture2DArray(TextureUsage::DepthAttachment, graphicsSettings.ShadowMapResolution, graphicsSettings.ShadowMapResolution,
			graphicsSettings.ShadowMapArraySize, 1, StorageFormat::Depth32);
		shadowMapArrayFreeBits.SetMax(graphicsSettings.ShadowMapArraySize);
		shadowMapArrayFreeBits.Clear();
		shadowMapArraySize = graphicsSettings.ShadowMapArraySize;

		shadowMapRenderTargetLayout = hwRenderer->CreateRenderTargetLayout(MakeArrayView(TextureUsage::DepthAttachment));
		shadowMapRenderOutputs.SetSize(shadowMapArraySize);
		for (int i = 0; i < shadowMapArraySize; i++)
		{
			RenderAttachments attachment;
			attachment.SetAttachment(0, shadowMapArray.Ptr(), i);
			shadowMapRenderOutputs[i] = res->CreateRenderOutput(shadowMapRenderTargetLayout.Ptr(),
				new RenderTarget(StorageFormat::Depth32, shadowMapArray, i, graphicsSettings.ShadowMapResolution, graphicsSettings.ShadowMapResolution));
		}
	}

	void ShadowMapResource::Destroy()
	{
		shadowMapRenderTargetLayout = nullptr;
		for (auto & x : shadowMapRenderOutputs)
			x = nullptr;
		shadowMapRenderOutputs.Clear();
		shadowMapArray = nullptr;
	}

	void ShadowMapResource::Reset()
	{
		shadowMapArrayFreeBits.Clear();
	}

	int ShadowMapResource::AllocShadowMaps(int count)
	{
		for (int i = 0; i <= shadowMapArraySize - count; i++)
		{
			if (!shadowMapArrayFreeBits.Contains(i))
			{
				bool occupied = false;
				for (int j = i + 1; j < i + count; j++)
				{
					if (shadowMapArrayFreeBits.Contains(j))
					{
						occupied = true;
						break;
					}
				}
				if (occupied)
					i += count - 1;
				else
				{
					for (int j = i; j < i + count; j++)
					{
						shadowMapArrayFreeBits.Add(j);
					}
					return i;
				}
			}
		}
		return -1;
	}
	void ShadowMapResource::FreeShadowMaps(int id, int count)
	{
		assert(id + count <= shadowMapArraySize);
		for (int i = id; i < id + count; i++)
		{
			assert(shadowMapArrayFreeBits.Contains(i));
			shadowMapArrayFreeBits.Remove(i);
		}
	}

	RefPtr<RenderTarget> RendererSharedResource::LoadSharedRenderTarget(String name, StorageFormat format, float ratio, int w, int h)
	{
		RefPtr<RenderTarget> result;
		if (renderTargets.TryGetValue(name, result))
		{
			if (result->Format == format)
				return result;
			else
				throw InvalidProgramException("the required buffer is not in required format.");
		}
		result = new RenderTarget();
		result->Format = format;
		result->UseFixedResolution = ratio == 0.0f;
		if (screenWidth > 0 || result->UseFixedResolution)
		{
			if (ratio == 0.0f)
			{
				result->Width = w;
				result->Height = h;
			}
			else
			{
				result->Width = (int)(screenWidth * ratio);
				result->Height = (int)(screenHeight * ratio);
			}
			result->Texture = hardwareRenderer->CreateTexture2D(TextureUsage::ColorAttachment, result->Width, result->Height, 1, format);
		}
		result->FixedWidth = w;
		result->FixedHeight = h;
		renderTargets[name] = result;
		return result;
	}
	void RendererSharedResource::Resize(int w, int h)
	{
		screenWidth = w;
		screenHeight = h;
		for (auto & r : renderTargets)
		{
			if (!r.Value->UseFixedResolution)
			{
				r.Value->Width = (int)(screenWidth * r.Value->ResolutionScale);
				r.Value->Height = (int)(screenHeight * r.Value->ResolutionScale);
				r.Value->Texture = hardwareRenderer->CreateTexture2D(TextureUsage::ColorAttachment, r.Value->Width, r.Value->Height, 1, r.Value->Format);
			}
		}
		for (auto & output : renderOutputs)
		{
			if (!output->bindings.First()->UseFixedResolution)
				UpdateRenderResultFrameBuffer(output.Ptr());
		}
	}

	ModuleInstance * RendererSharedResource::CreateModuleInstance(SpireModule * shaderModule, DeviceMemory * uniformMemory, int uniformBufferSize)
	{
		ModuleInstance * rs = new ModuleInstance(shaderModule);
		rs->BindingName = spGetModuleName(shaderModule);
		rs->BufferLength = Math::Max(spModuleGetParameterBufferSize(shaderModule), uniformBufferSize);
		if (rs->BufferLength > 0)
		{
			rs->UniformPtr = (unsigned char *)uniformMemory->Alloc(rs->BufferLength);
			rs->UniformMemory = uniformMemory;
			rs->BufferOffset = (int)(rs->UniformPtr - (unsigned char*)uniformMemory->BufferPtr());
		}
		else
			rs->UniformMemory = nullptr;
		int paramCount = spModuleGetParameterCount(shaderModule);
		List<DescriptorLayout> descs;
		descs.Add(DescriptorLayout(0, BindingType::UniformBuffer));
		for (int i = 0; i < paramCount; i++)
		{
			SpireComponentInfo info;
			spModuleGetParameter(shaderModule, i, &info);
			if (info.BindableResourceType != SPIRE_NON_BINDABLE)
			{
				DescriptorLayout layout;
				switch (info.BindableResourceType)
				{
				case SPIRE_TEXTURE:
					layout.Type = BindingType::Texture;
					break;
				case SPIRE_UNIFORM_BUFFER:
					layout.Type = BindingType::UniformBuffer;
					break;
				case SPIRE_SAMPLER:
					layout.Type = BindingType::Sampler;
					break;
				case SPIRE_STORAGE_BUFFER:
					layout.Type = BindingType::StorageBuffer;
					break;
				}
				descs.Add(layout);
			}
		}
		rs->DescriptorLayout = hardwareRenderer->CreateDescriptorSetLayout(descs.GetArrayView());
		rs->Descriptors = hardwareRenderer->CreateDescriptorSet(rs->DescriptorLayout.Ptr());
		rs->Descriptors->BeginUpdate();
		if (rs->UniformMemory)
			rs->Descriptors->Update(0, rs->UniformMemory->GetBuffer(), rs->BufferOffset, rs->BufferLength);
		rs->Descriptors->EndUpdate();
		return rs;
	}

	void RendererSharedResource::LoadShaderLibrary()
	{
		spAddSearchPath(spireContext, Engine::Instance()->GetDirectory(true, ResourceType::Shader).Buffer());
		spAddSearchPath(spireContext, Engine::Instance()->GetDirectory(false, ResourceType::Shader).Buffer());

		auto pipelineDefFile = Engine::Instance()->FindFile("GlPipeline.shader", ResourceType::Shader);
		if (!pipelineDefFile.Length())
			throw InvalidOperationException("'Pipeline.shader' not found. Engine directory is not setup correctly.");
		auto engineShaderDir = CoreLib::IO::Path::GetDirectoryName(pipelineDefFile);
		auto utilDefFile = Engine::Instance()->FindFile("Utils.shader", ResourceType::Shader);
		if (!utilDefFile.Length())
			throw InvalidOperationException("'Utils.shader' not found. Engine directory is not setup correctly.");
		SpireDiagnosticSink * spireSink = spCreateDiagnosticSink(spireContext);
		spLoadModuleLibrary(spireContext, pipelineDefFile.Buffer(), spireSink);
		spLoadModuleLibrary(spireContext, utilDefFile.Buffer(), spireSink);
		if (spDiagnosticSinkHasAnyErrors(spireSink))
		{
			Diagnostics::Debug::WriteLine(GetSpireOutput(spireSink));
			throw HardwareRendererException("shader compilation error.");
		}
		spDestroyDiagnosticSink(spireSink);
	}

	void RendererSharedResource::Init(HardwareRenderer * phwRenderer)
	{
		hardwareRenderer = phwRenderer;
		// Vertex buffer for VS bypass
		const float fsTri[] =
		{
			-1.0f, -1.0f, 0.0f, 0.0f,
			1.0f, -1.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 0.0f, 1.0f
		};
		fullScreenQuadVertBuffer = hardwareRenderer->CreateBuffer(BufferUsage::ArrayBuffer);
		fullScreenQuadVertBuffer->SetData((void*)&fsTri[0], sizeof(fsTri));

		// Create common texture samplers
		nearestSampler = hardwareRenderer->CreateTextureSampler();
		nearestSampler->SetFilter(TextureFilter::Nearest);

		linearSampler = hardwareRenderer->CreateTextureSampler();
		linearSampler->SetFilter(TextureFilter::Linear);

		textureSampler = hardwareRenderer->CreateTextureSampler();
		textureSampler->SetFilter(TextureFilter::Anisotropic16x);

		shadowSampler = hardwareRenderer->CreateTextureSampler();
		shadowSampler->SetFilter(TextureFilter::Linear);
		shadowSampler->SetDepthCompare(CompareFunc::LessEqual);

		shadowMapResources.Init(hardwareRenderer.Ptr(), this);

		spireContext = spCreateCompilationContext("");
		LoadShaderLibrary();
		pipelineManager.Init(spireContext, hardwareRenderer.Ptr());

		indexBufferMemory.Init(hardwareRenderer.Ptr(), BufferUsage::IndexBuffer, 26, 256);
		vertexBufferMemory.Init(hardwareRenderer.Ptr(), BufferUsage::ArrayBuffer, 28, 256);
	}
	void RendererSharedResource::Destroy()
	{
		shadowMapResources.Destroy();
		textureSampler = nullptr;
		nearestSampler = nullptr;
		linearSampler = nullptr;
		renderTargets = CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<RenderTarget>>();
		fullScreenQuadVertBuffer = nullptr;
		spDestroyCompilationContext(spireContext);
	}
	
	RenderTarget::RenderTarget(GameEngine::StorageFormat format, CoreLib::RefPtr<Texture2DArray> texArray, int layer, int w, int h)
	{
		TextureArray = texArray;
		Format = format;
		UseFixedResolution = true;
		Layer = layer;
		ResolutionScale = 0.0f;
		FixedWidth = Width = w;
		FixedHeight = Height = h;
	}
	Buffer * DrawableMesh::GetVertexBuffer()
	{
		return renderRes->vertexBufferMemory.GetBuffer();
	}
	Buffer * DrawableMesh::GetIndexBuffer()
	{
		return renderRes->indexBufferMemory.GetBuffer();
	}
	DrawableMesh::~DrawableMesh()
	{
		if (vertexCount)
			renderRes->vertexBufferMemory.Free((char*)renderRes->vertexBufferMemory.BufferPtr() + vertexBufferOffset, vertexCount * vertexFormat.Size());
		if (indexCount)
			renderRes->indexBufferMemory.Free((char*)renderRes->indexBufferMemory.BufferPtr() + indexBufferOffset, indexCount * sizeof(int));
	}
	void RenderPassInstance::SetFixedOrderDrawContent(PipelineContext & pipelineManager, CullFrustum frustum, CoreLib::ArrayView<Drawable*> drawables)
	{
		renderOutput->GetSize(viewport.Width, viewport.Height);
		commandBuffer->BeginRecording(renderOutput->GetFrameBuffer());
		commandBuffer->SetViewport(viewport.X, viewport.Y, viewport.Width, viewport.Height);
		commandBuffer->ClearAttachments(renderOutput->GetFrameBuffer());
		DescriptorSetBindingArray bindings;
		pipelineManager.GetBindings(bindings);
		for (int i = 0; i < bindings.Count(); i++)
			commandBuffer->BindDescriptorSet(i, bindings[i]);
		numDrawCalls = 0;
		for (auto obj : drawables)
		{
			if (!frustum.IsBoxInFrustum(obj->Bounds))
				continue;
			numDrawCalls++;
			pipelineManager.PushModuleInstance(obj->GetMaterial()->MaterialGeometryModule.Ptr());
			pipelineManager.PushModuleInstance(obj->GetMaterial()->MaterialPatternModule.Ptr());
			pipelineManager.PushModuleInstance(obj->GetTransformModule());
			if (auto pipelineInst = pipelineManager.GetPipeline(&obj->GetVertexFormat()))
			{
				auto mesh = obj->GetMesh();
				commandBuffer->BindPipeline(pipelineInst->pipeline.Ptr());
				commandBuffer->BindIndexBuffer(mesh->GetIndexBuffer(), mesh->indexBufferOffset);
				commandBuffer->BindVertexBuffer(mesh->GetVertexBuffer(), mesh->vertexBufferOffset);
				commandBuffer->BindDescriptorSet(bindings.Count(), obj->GetMaterial()->MaterialGeometryModule->Descriptors.Ptr());
				commandBuffer->BindDescriptorSet(bindings.Count() + 1, obj->GetMaterial()->MaterialPatternModule->Descriptors.Ptr());
				commandBuffer->BindDescriptorSet(bindings.Count() + 2, obj->GetTransformModule()->Descriptors.Ptr());
				commandBuffer->DrawIndexed(0, mesh->indexCount);
			}
			pipelineManager.PopModuleInstance();
			pipelineManager.PopModuleInstance();
			pipelineManager.PopModuleInstance();
		}
		commandBuffer->EndRecording();
	}
	void RenderPassInstance::SetDrawContent(PipelineContext & pipelineManager, CoreLib::List<Drawable*>& reorderBuffer, CullFrustum frustum, CoreLib::ArrayView<Drawable*> drawables)
	{
		reorderBuffer.Clear();
		for (auto obj : drawables)
		{
			obj->ReorderKey = nullptr;
			if (!frustum.IsBoxInFrustum(obj->Bounds))
				continue;
			pipelineManager.PushModuleInstance(obj->GetMaterial()->MaterialGeometryModule.Ptr());
			pipelineManager.PushModuleInstance(obj->GetMaterial()->MaterialPatternModule.Ptr());
			pipelineManager.PushModuleInstance(obj->GetTransformModule());
			obj->ReorderKey = pipelineManager.GetPipeline(&obj->GetVertexFormat());
			pipelineManager.PopModuleInstance();
			pipelineManager.PopModuleInstance();
			pipelineManager.PopModuleInstance();
			reorderBuffer.Add(obj);
		}
		reorderBuffer.Sort();
		renderOutput->GetSize(viewport.Width, viewport.Height);
		commandBuffer->BeginRecording(renderOutput->GetFrameBuffer());
		commandBuffer->SetViewport(viewport.X, viewport.Y, viewport.Width, viewport.Height);
		commandBuffer->ClearAttachments(renderOutput->GetFrameBuffer());
		DescriptorSetBindingArray bindings;
		pipelineManager.GetBindings(bindings);
		for (int i = 0; i < bindings.Count(); i++)
			commandBuffer->BindDescriptorSet(i, bindings[i]);
		numDrawCalls = 0;
		for (auto obj : reorderBuffer)
		{
			numDrawCalls++;
			if (auto pipelineInst = (PipelineClass*)obj->ReorderKey)
			{
				auto mesh = obj->GetMesh();
				commandBuffer->BindPipeline(pipelineInst->pipeline.Ptr());
				commandBuffer->BindIndexBuffer(mesh->GetIndexBuffer(), mesh->indexBufferOffset);
				commandBuffer->BindVertexBuffer(mesh->GetVertexBuffer(), mesh->vertexBufferOffset);
				commandBuffer->BindDescriptorSet(bindings.Count(), obj->GetMaterial()->MaterialGeometryModule->Descriptors.Ptr());
				commandBuffer->BindDescriptorSet(bindings.Count() + 1, obj->GetMaterial()->MaterialPatternModule->Descriptors.Ptr());
				commandBuffer->BindDescriptorSet(bindings.Count() + 2, obj->GetTransformModule()->Descriptors.Ptr());
				commandBuffer->DrawIndexed(0, mesh->indexCount);
			}
		}
		commandBuffer->EndRecording();
	}
}


