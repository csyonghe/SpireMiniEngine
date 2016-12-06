#include "RenderContext.h"
#include "Material.h"
#include "Mesh.h"
#include "Engine.h"
#include "TextureCompressor.h"

#include "CoreLib/LibIO.h"
#include "CoreLib/Graphics/TextureFile.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::IO;
	using namespace VectorMath;

	void Drawable::UpdateMaterialUniform()
	{
		if (material->ParameterDirty)
		{
			material->ParameterDirty = false;
			unsigned char * ptr = uniforms.instanceUniform;
			auto end = ptr + uniforms.instanceUniformCount;
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
			instanceUniformUpdated = true;
		}
	}

	void Drawable::UpdateTransformUniform(const VectorMath::Matrix4 & localTransform)
	{
		if (type != DrawableType::Static)
			throw InvalidOperationException("cannot update non-static drawable with static transform data.");
		if (!uniforms.transformUniform)
			throw InvalidOperationException("invalid buffer.");

		Vec4 * bufferPtr = (Vec4*)uniforms.transformUniform;
		*((Matrix4*)bufferPtr) = localTransform; // write(localTransform)
		bufferPtr += 4;

		Matrix4 normMat;
		localTransform.Inverse(normMat);
		normMat.Transpose();
		*(bufferPtr++) = *((Vec4*)(normMat.values));
		*(bufferPtr++) = *((Vec4*)(normMat.values + 4));
		*(bufferPtr++) = *((Vec4*)(normMat.values + 8));

		transformUniformUpdated = true;
	}

	void Drawable::UpdateTransformUniform(const VectorMath::Matrix4 & localTransform, const Pose & pose)
	{
		if (type != DrawableType::Skeletal)
			throw InvalidOperationException("cannot update static drawable with skeletal transform data.");
		if (!uniforms.transformUniform)
			throw InvalidOperationException("invalid buffer.");

		const int poseMatrixSize = skeleton->Bones.Count() * (sizeof(Vec4) * 7);

		// ensure allocated transform buffer is sufficient
		_ASSERT(uniforms.transformUniformCount >= poseMatrixSize);

		List<Matrix4> matrices;
		pose.GetMatrices(skeleton, matrices);
		
		Vec4 * bufferPtr = (Vec4*)uniforms.transformUniform;

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
		transformUniformUpdated = true;
	}
	RefPtr<DrawableMesh> SceneResource::LoadDrawableMesh(Mesh * mesh)
	{
		RefPtr<DrawableMesh> result;
		if (meshes.TryGetValue(mesh, result))
			return result;

		auto hw = rendererResource->hardwareRenderer.Ptr();

		result = new DrawableMesh();
		result->indexBuffer = hw->CreateBuffer(BufferUsage::IndexBuffer);
		result->vertexBuffer = hw->CreateBuffer(BufferUsage::ArrayBuffer);
		result->vertexFormat = LoadVertexFormat(mesh->GetVertexFormat());
		result->vertexCount = mesh->GetVertexCount();
		result->indexBuffer->SetData(mesh->Indices.Buffer(), mesh->Indices.Count() * sizeof(mesh->Indices[0]));
		result->vertexBuffer->SetData(mesh->GetVertexBuffer(), mesh->GetVertexCount() * mesh->GetVertexSize());
		result->indexCount = mesh->Indices.Count();
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

		auto rs = hw->CreateTexture2D(TextureUsage::Sampled);
		for (int i = 0; i < data.GetMipLevels(); i++)
			rs->SetData(format, i, Math::Max(data.GetWidth() >> i, 1), Math::Max(data.GetHeight() >> i, 1), 1, dataType, data.GetData(i).Buffer());
		if (format != StorageFormat::BC1 && format != StorageFormat::BC5)
			rs->BuildMipmaps();
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
	Shader * SceneResource::LoadShader(const String & src, void * data, int size, ShaderType shaderType)
	{
		{
			String debugFileName = src + ".spv";
			BinaryWriter debugWriter(new FileStream(debugFileName, FileMode::Create));
			debugWriter.Write((unsigned char*)data, size);
		}
		RefPtr<Shader> result;
		if (shaders.TryGetValue(src, result))
			return result.Ptr();

		auto hw = rendererResource->hardwareRenderer.Ptr();

		result = hw->CreateShader(shaderType, (char*)data, size);
		shaders[src] = result;
		return result.Ptr();
	}
	VertexFormat SceneResource::LoadVertexFormat(MeshVertexFormat vertFormat)
	{
		VertexFormat rs;
		auto vertTypeId = vertFormat.GetTypeId();
		if (vertexFormats.TryGetValue(vertTypeId, rs))
			return rs;
		VertexFormat vertexFormat;
		int location = 0;

		const int UNNORMALIZED = 0;
		const int NORMALIZED = 1;

		// Always starts with vec3 pos
		rs.Attributes.Add(VertexAttributeDesc(DataType::Float3, UNNORMALIZED, 0, location));
		location++;

		for (int i = 0; i < vertFormat.GetUVChannelCount(); i++)
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::Half2, UNNORMALIZED, vertFormat.GetUVOffset(i), location));
			location++;
		}
		if (vertFormat.HasTangent())
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetTangentFrameOffset(), location));
			location++;
		}
		for (int i = 0; i < vertFormat.GetColorChannelCount(); i++)
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::Byte4, NORMALIZED, vertFormat.GetColorOffset(i), location));
			location++;
		}
		if (vertFormat.HasSkinning())
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetBoneIdsOffset(), location));
			location++;
			rs.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetBoneWeightsOffset(), location));
			location++;
		}

		vertexFormats[vertTypeId] = rs;
		return rs;
	}

	PipelineClass SceneResource::LoadMaterialPipeline(String identifier, Material * material, RenderTargetLayout * renderTargetLayout, MeshVertexFormat vertFormat, String entryPointShader)
	{
		auto hw = rendererResource->hardwareRenderer;

		PipelineClass pipelineClass;
		if (pipelineClassCache.TryGetValue(identifier, pipelineClass))
			return pipelineClass;

		RefPtr<PipelineBuilder> pipelineBuilder = hw->CreatePipelineBuilder();

		int offset = rendererResource->GetTextureBindingStart();
		int location = 0;
		pipelineBuilder->SetBindingLayout(0, BindingType::UniformBuffer);
		pipelineBuilder->SetBindingLayout(1, BindingType::UniformBuffer);
		pipelineBuilder->SetBindingLayout(2, BindingType::UniformBuffer);
		pipelineBuilder->SetBindingLayout(3, BindingType::StorageBuffer);
		pipelineBuilder->DepthCompareFunc = CompareFunc::LessEqual;
		for (auto var : material->Variables)
		{
			if (var.Value.VarType == DynamicVariableType::Texture)
			{
				pipelineBuilder->SetBindingLayout(offset + location, BindingType::Texture);
			}
			location++;
		}

		// Set vertex layout

		pipelineBuilder->SetVertexLayout(LoadVertexFormat(vertFormat));
		// Compile shaders
		ShaderCompilationResult rs;

		if (!rendererResource->hardwareFactory->CompileShader(rs, material->ShaderFile, vertFormat.GetShaderDefinition(), entryPointShader, ""))
			throw HardwareRendererException("Shader compilation failure");

		for (auto& compiledShader : rs.Shaders)
		{
			Shader* shader;
			if (compiledShader.Key == "vs")
			{
				shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::VertexShader);
			}
			else if (compiledShader.Key == "fs")
			{
				shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::FragmentShader);
			}
			else if (compiledShader.Key == "tcs")
			{
				shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::HullShader);
			}
			else if (compiledShader.Key == "tes")
			{
				shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::DomainShader);
			}
			pipelineClass.shaders.Add(shader);
		}

		pipelineBuilder->SetShaders(pipelineClass.shaders.GetArrayView());

		pipelineClass.pipeline = pipelineBuilder->ToPipeline(renderTargetLayout);

		pipelineClassCache[identifier] = pipelineClass;
		return pipelineClass;
	}
	
	
	SceneResource::SceneResource(RendererSharedResource * resource)
		: rendererResource(resource)
	{
		auto hwRenderer = resource->hardwareRenderer.Ptr();
		instanceUniformMemory.Init(hwRenderer, BufferUsage::UniformBuffer, 24, hwRenderer->UniformBufferAlignment());
		staticTransformMemory.Init(hwRenderer, BufferUsage::UniformBuffer, 24, hwRenderer->UniformBufferAlignment());
		skeletalTransformMemory.Init(hwRenderer, BufferUsage::StorageBuffer, 24, hwRenderer->StorageBufferAlignment());
	}
	
	void SceneResource::Clear()
	{
		meshes = CoreLib::EnumerableDictionary<Mesh*, RefPtr<DrawableMesh>>();
		shaders = EnumerableDictionary<String, RefPtr<Shader>>();
		pipelineClassCache = EnumerableDictionary<String, PipelineClass>();
		textures = EnumerableDictionary<String, RefPtr<Texture2D>>();
	}
	RefPtr<RenderTarget> RendererSharedResource::AcquireRenderTarget(String name, StorageFormat format)
	{
		RefPtr<RenderTarget> result;
		if (renderTargets.TryGetValue(name, result))
		{
			if (result->Format == format)
				return result;
			else
				throw InvalidProgramException("the required buffer is not in required format.");
		}
		throw InvalidProgramException("the required buffer does not exist.");
	}
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
			throw NotImplementedException("Unsupported storage format as render target.");
		}
		return dataType;
	}
	RefPtr<RenderTarget> RendererSharedResource::ProvideOrModifyRenderTarget(String name, StorageFormat format)
	{
		RefPtr<RenderTarget> result;
		if (renderTargets.TryGetValue(name, result))
		{
			if (result->Format == format)
				return result;
			else
				throw InvalidProgramException("the required buffer is not in required format.");
		}
		return ProvideRenderTarget(name, format);
	}
	RefPtr<RenderTarget> RendererSharedResource::ProvideRenderTarget(String name, StorageFormat format)
	{
		if (renderTargets.ContainsKey(name))
			throw InvalidProgramException("the buffer already exists.");
		RefPtr<RenderTarget> result = new RenderTarget();
		result->Format = format;
		if (screenWidth > 0)
		{
			result->Texture = hardwareRenderer->CreateTexture2D(TextureUsage::ColorAttachment);
			result->Texture->SetData(format, screenWidth, screenHeight, 1, GetStorageDataType(format), nullptr);
		}
		renderTargets[name] = result;
		return result;
	}
	void RendererSharedResource::Resize(int w, int h)
	{
		screenWidth = w;
		screenHeight = h;
		for (auto & r : renderTargets)
		{
			r.Value->Texture = hardwareRenderer->CreateTexture2D(TextureUsage::ColorAttachment);
			r.Value->Texture->SetData(r.Value->Format, (int)(screenWidth * r.Value->ResolutionScale), 
				(int)(screenHeight * r.Value->ResolutionScale), 1, GetStorageDataType(r.Value->Format), nullptr);
		}
	}
	int RendererSharedResource::GetTextureBindingStart()
	{
		if (api == RenderAPI::Vulkan)
			return 4;
		else
			return 0;
	}
}


