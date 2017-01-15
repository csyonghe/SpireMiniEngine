#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include "CoreLib/PerformanceCounter.h"
#include <glew/glew.h>
#include <glew/wglew.h>
#include "../VectorMath.h"
#include "../Spire/Spire.h"
#include "../WinForm/Debug.h"
#include "../GameEngineCore/HardwareRenderer.h"
#include "../Common.h"
#include <assert.h>
#pragma comment(lib, "opengl32.lib")

using namespace GameEngine;

namespace GLL
{
	const int TargetOpenGLVersion_Major = 4;
	const int TargetOpenGLVersion_Minor = 3;
	using namespace VectorMath;
	class GUIWindow
	{};

	bool DebugErrorEnabled = true;

	typedef GUIWindow* GUIHandle;

	class StencilMode
	{
	public:
		union
		{
			struct
			{
				CompareFunc StencilFunc;
				StencilOp Fail, DepthFail, DepthPass;
#pragma warning(suppress : 4201)
			};
			int Bits;
		};
		unsigned int StencilMask, StencilReference;
		bool operator == (const StencilMode & mode)
		{
			return Bits == mode.Bits && StencilMask == mode.StencilMask && StencilReference == mode.StencilReference;
		}
		bool operator != (const StencilMode & mode)
		{
			return Bits != mode.Bits || StencilMask != mode.StencilMask || StencilReference != mode.StencilReference;
		}
		StencilMode()
		{
			StencilFunc = CompareFunc::Disabled;
			Fail = StencilOp::Keep;
			DepthFail = StencilOp::Keep;
			DepthPass = StencilOp::Keep;
			StencilMask = 0xFFFFFFFF;
			StencilReference = 0;
		}
	};

	DataType GetSingularDataType(DataType type)
	{
		if (type == DataType::UInt4_10_10_10_2)
			return DataType::Int;
		return (DataType)(((int)type) & 0xF0);
	}

	int GetDataTypeComponenets(DataType type)
	{
		return (((int)type) & 0xF) + 1;
	}

	GLuint TranslateBufferUsage(BufferUsage usage)
	{
		switch (usage)
		{
		case BufferUsage::ArrayBuffer: return GL_ARRAY_BUFFER;
		case BufferUsage::IndexBuffer: return GL_ELEMENT_ARRAY_BUFFER;
		case BufferUsage::StorageBuffer: return GL_SHADER_STORAGE_BUFFER;
		case BufferUsage::UniformBuffer: return GL_UNIFORM_BUFFER;
		default: throw HardwareRendererException("Unsupported buffer usage.");
		}
	}

	GLuint TranslateStorageFormat(StorageFormat format)
	{
		int internalFormat = 0;
		switch (format)
		{
		case StorageFormat::R_F16:
			internalFormat = GL_R16F;
			break;
		case StorageFormat::R_F32:
			internalFormat = GL_R32F;
			break;
		case StorageFormat::R_16:
			internalFormat = GL_R16;
			break;
		case StorageFormat::R_I16:
			internalFormat = GL_R16I;
			break;
		case StorageFormat::Int32_Raw:
			internalFormat = GL_R32I;
			break;
		case StorageFormat::R_8:
			internalFormat = GL_R8;
			break;
		case StorageFormat::RG_F16:
			internalFormat = GL_RG16F;
			break;
		case StorageFormat::RG_F32:
			internalFormat = GL_RG32F;
			break;
		case StorageFormat::RG_I16:
			internalFormat = GL_RG16I;
			break;
		case StorageFormat::RG_16:
			internalFormat = GL_RG16;
			break;
		case StorageFormat::RG_I32_Raw:
			internalFormat = GL_RG32I;
			break;
		case StorageFormat::RG_8:
			internalFormat = GL_RG8;
			break;
		case StorageFormat::RG_I8:
			internalFormat = GL_RG8I;
			break;
		case StorageFormat::RGBA_F16:
			internalFormat = GL_RGBA16F;
			break;
		case StorageFormat::RGBA_F32:
			internalFormat = GL_RGBA32F;
			break;
		case StorageFormat::R11F_G11F_B10F:
			internalFormat = GL_R11F_G11F_B10F;
			break;
		case StorageFormat::RGB10_A2:
			internalFormat = GL_RGB10_A2;
			break;
		case StorageFormat::RGBA_I16:
			internalFormat = GL_RGBA16;
			break;
		case StorageFormat::RGBA_I32_Raw:
			internalFormat = GL_RGBA32I;
			break;
		case StorageFormat::RGBA_I8:
			internalFormat = GL_RGBA8I;
			break;
		case StorageFormat::RGBA_8:
			internalFormat = GL_RGBA8;
			break;
		case StorageFormat::RGBA_Compressed:
#ifdef _DEBUG
			internalFormat = GL_COMPRESSED_RGBA;
#else
			internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
#endif
			break;
		case StorageFormat::BC1:
			internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			break;
		case StorageFormat::BC5:
			internalFormat = GL_COMPRESSED_RG_RGTC2;
			break;
		case StorageFormat::BC3:
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		case StorageFormat::Depth24:
			internalFormat = GL_DEPTH_COMPONENT24;
			break;
		case StorageFormat::Depth24Stencil8:
			internalFormat = GL_DEPTH24_STENCIL8;
			break;
		case StorageFormat::Depth32:
			internalFormat = GL_DEPTH_COMPONENT32;
			break;
		default:
			throw HardwareRendererException("Unsupported storage format.");
		}
		return internalFormat;
	}

	GLuint TranslateDataTypeToFormat(DataType type)
	{
		switch (GetDataTypeComponenets(type))
		{
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: throw HardwareRendererException("Unsupported data type.");
		}
	}

	GLuint TranslateDataTypeToInputType(DataType type)
	{
		switch (type)
		{
		case DataType::Int:
		case DataType::Int2:
		case DataType::Int3:
		case DataType::Int4:
			return GL_INT;
		case DataType::UInt:
			return GL_UNSIGNED_INT;
		case DataType::Byte:
		case DataType::Byte2:
		case DataType::Byte3:
		case DataType::Byte4:
			return GL_UNSIGNED_BYTE;
		case DataType::Char:
		case DataType::Char2:
		case DataType::Char3:
		case DataType::Char4:
			return GL_BYTE;
		case DataType::Short:
		case DataType::Short2:
		case DataType::Short3:
		case DataType::Short4:
			return GL_SHORT;
		case DataType::UShort:
		case DataType::UShort2:
		case DataType::UShort3:
		case DataType::UShort4:
			return GL_UNSIGNED_SHORT;
		case DataType::Float:
		case DataType::Float2:
		case DataType::Float3:
		case DataType::Float4:
			return GL_FLOAT;
		case DataType::Half:
		case DataType::Half2:
		case DataType::Half3:
		case DataType::Half4:
			return GL_HALF_FLOAT;
		case DataType::UInt4_10_10_10_2:
			return GL_UNSIGNED_INT_2_10_10_10_REV;
		default:
			throw HardwareRendererException("Unsupported data type.");
		}
	}

	GLuint TranslateBufferType(BufferType type)
	{
		switch (type)
		{
		case BufferType::UniformBuffer: return GL_UNIFORM_BUFFER;
		case BufferType::ArrayBuffer: return GL_ARRAY_BUFFER;
		case BufferType::StorageBuffer: return GL_SHADER_STORAGE_BUFFER;
		case BufferType::ElementBuffer: return GL_ELEMENT_ARRAY_BUFFER;
		default: throw NotImplementedException();
		};
	}

	GLenum TranslateBufferAccess(BufferAccess access)
	{
		switch (access)
		{
		case BufferAccess::Read: return GL_MAP_READ_BIT;
		case BufferAccess::Write: return GL_MAP_WRITE_BIT;
		case BufferAccess::ReadWrite: return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		case BufferAccess::ReadWritePersistent: return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
		default: throw NotImplementedException();
		}
	}

	GLbitfield TranslateBufferStorageFlags(BufferStorageFlag flag)
	{
		GLbitfield result = 0;
		if ((int)flag & (int)BufferStorageFlag::DynamicStorage) result |= GL_DYNAMIC_STORAGE_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapRead) result |= GL_MAP_READ_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapWrite) result |= GL_MAP_WRITE_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapPersistent) result |= GL_MAP_PERSISTENT_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapCoherent) result |= GL_MAP_COHERENT_BIT;
		if ((int)flag & (int)BufferStorageFlag::ClientStorage) result |= GL_CLIENT_STORAGE_BIT;
		return result;
	}

	class GL_Object
	{
	public:
		GLuint Handle;
		GL_Object()
		{
			Handle = 0;
		}
	};

	class Fence : public GameEngine::Fence 
	{
	public:
		int waitCounter = 0;
		GLsync handle = nullptr;
		Fence()
		{}
		~Fence()
		{
			if (handle)
				glDeleteSync(handle);
		}
		virtual void Reset() override
		{
			if (handle)
			{
				glDeleteSync(handle);
				handle = nullptr;
			}
		}
		virtual void Wait() override
		{
			if (handle)
			{
				while (1)
				{
					GLenum waitReturn = glClientWaitSync(handle, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
					if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED)
						return;

					waitCounter++;
				}
			}
		}

	};

	class RenderBuffer : public GL_Object
	{
	public:
		StorageFormat storageFormat;
		GLint internalFormat;
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void GetSize(int & w, int &h)
		{
			glGetNamedRenderbufferParameterivEXT(Handle, GL_RENDERBUFFER_WIDTH, &w);
			glGetNamedRenderbufferParameterivEXT(Handle, GL_RENDERBUFFER_HEIGHT, &h);

		}
		void Resize(int width, int height, int samples)
		{
			if (samples <= 1)
				glNamedRenderbufferStorageEXT(Handle, internalFormat, width, height);
			else
				glNamedRenderbufferStorageMultisampleEXT(Handle, samples, internalFormat, width, height);
		}
	};

	class TextureSampler : public GL_Object, public GameEngine::TextureSampler
	{
	public:
		TextureFilter GetFilter()
		{
			GLint filter;
			float aniso = 0.0f;
			glGetSamplerParameteriv(Handle, GL_TEXTURE_MIN_FILTER, &filter);
			switch (filter)
			{
			case GL_NEAREST:
				return TextureFilter::Nearest;
			case GL_LINEAR:
				return TextureFilter::Linear;
			case GL_LINEAR_MIPMAP_LINEAR:
				glGetSamplerParameterfv(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
				if (aniso < 3.99f)
					return TextureFilter::Trilinear;
				else if (aniso < 7.99f)
					return TextureFilter::Anisotropic4x;
				else if (aniso < 15.99f)
					return TextureFilter::Anisotropic8x;
				else
					return TextureFilter::Anisotropic16x;
			default:
				return TextureFilter::Trilinear;
			}
		}
		void SetFilter(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::Nearest:
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;
			case TextureFilter::Linear:
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
			case TextureFilter::Trilinear:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				break;
			case TextureFilter::Anisotropic4x:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
				break;
			case TextureFilter::Anisotropic8x:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
				break;
			case TextureFilter::Anisotropic16x:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
				break;
			}
		}
		WrapMode GetWrapMode()
		{
			GLint mode;
			glGetSamplerParameteriv(Handle, GL_TEXTURE_WRAP_S, &mode);
			switch (mode)
			{
			case GL_REPEAT:
				return WrapMode::Repeat;
			default:
				return WrapMode::Clamp;
			}
		}
		void SetWrapMode(WrapMode wrap)
		{
			int mode = GL_CLAMP_TO_EDGE;
			switch (wrap)
			{
			case WrapMode::Clamp:
				mode = GL_CLAMP_TO_EDGE;
				break;
			case WrapMode::Repeat:
				mode = GL_REPEAT;
				break;
			case WrapMode::Mirror:
				mode = GL_MIRRORED_REPEAT;
				break;
			}
			glSamplerParameteri(Handle, GL_TEXTURE_WRAP_S, mode);
			glSamplerParameteri(Handle, GL_TEXTURE_WRAP_T, mode);
			glSamplerParameteri(Handle, GL_TEXTURE_WRAP_R, mode);
		}
		void SetDepthCompare(CompareFunc op)
		{
			if (op == CompareFunc::Disabled)
			{
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
			}
			switch (op)
			{
			case CompareFunc::Disabled:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
				break;
			case CompareFunc::Equal:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_EQUAL);
				break;
			case CompareFunc::Less:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
				break;
			case CompareFunc::Greater:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
				break;
			case CompareFunc::LessEqual:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				break;
			case CompareFunc::GreaterEqual:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
				break;
			case CompareFunc::NotEqual:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_NOTEQUAL);
				break;
			case CompareFunc::Always:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
				break;
			case CompareFunc::Never:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
				break;
			}
		}
		CompareFunc GetCompareFunc()
		{
			GLint compareMode;
			glGetSamplerParameterIiv(Handle, GL_TEXTURE_COMPARE_MODE, &compareMode);
			if (compareMode == GL_NONE) return CompareFunc::Disabled;

			GLint compareFunc;
			glGetSamplerParameterIiv(Handle, GL_TEXTURE_COMPARE_FUNC, &compareFunc);
			switch (compareFunc)
			{
			case GL_EQUAL: return CompareFunc::Equal;
			case GL_LESS: return CompareFunc::Less;
			case GL_GREATER: return CompareFunc::Greater;
			case GL_LEQUAL: return CompareFunc::LessEqual;
			case GL_GEQUAL: return CompareFunc::GreaterEqual;
			case GL_NOTEQUAL: return CompareFunc::NotEqual;
			case GL_ALWAYS: return CompareFunc::Always;
			case GL_NEVER: return CompareFunc::Never;
			default: throw HardwareRendererException("Not implemented");
			}
		}
	};

	class TextureHandle
	{
	public:
		uint64_t Handle = 0;
		TextureHandle() = default;
		TextureHandle(uint64_t handle)
		{
			this->Handle = handle;
		}
		void MakeResident() const
		{
			if (!glIsTextureHandleResidentARB(Handle))
				glMakeTextureHandleResidentARB(Handle);
		}
		void MakeNonResident() const
		{
			if (glIsTextureHandleResidentARB(Handle))
				glMakeTextureHandleNonResidentARB(Handle);
		}
	};

	class Texture : public GL_Object
	{
	public:
		GLuint BindTarget;
		//GLuint64 TextureHandle;
		GLint internalFormat;
		StorageFormat storageFormat;
		GLenum format, type;
		Texture()
		{
			BindTarget = GL_TEXTURE_2D;
			internalFormat = GL_RGBA;
			format = GL_RGBA;
			storageFormat = StorageFormat::RGBA_I8;
			type = GL_UNSIGNED_BYTE;
		}
		~Texture()
		{
			if (Handle)
				glDeleteTextures(1, &Handle);
		}
		bool operator == (const Texture &t) const
		{
			return Handle == t.Handle;
		}
		TextureHandle GetTextureHandle() const
		{
			return TextureHandle(glGetTextureHandleARB(Handle));
		}
		TextureHandle GetTextureHandle(TextureSampler sampler) const
		{
			return TextureHandle(glGetTextureSamplerHandleARB(Handle, sampler.Handle));
		}
	};

	class Texture2D : public Texture, public GameEngine::Texture2D
	{
	public:
		Texture2D()
		{
			BindTarget = GL_TEXTURE_2D;
		}
		~Texture2D()
		{
			if (Handle)
			{
				glDeleteTextures(1, &Handle);
				Handle = 0;
			}
		}
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void GetSize(int & width, int & height)
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void Clear(Vec4 data)
		{
			glClearTexImage(Handle, 0, format, type, &data);
		}
		virtual void SetData(int level, int width, int height, int samples, DataType inputType, void * data) override
		{
			if (storageFormat == StorageFormat::BC1 || storageFormat == StorageFormat::BC5)
				throw HardwareRendererException("Compressed textures must recreated instead of modified");

			this->internalFormat = TranslateStorageFormat(storageFormat);
			this->format = TranslateDataTypeToFormat(inputType);
			this->type = TranslateDataTypeToInputType(inputType);
			if (this->internalFormat == GL_DEPTH_COMPONENT16 || this->internalFormat == GL_DEPTH_COMPONENT24 || this->internalFormat == GL_DEPTH_COMPONENT32)
				this->format = GL_DEPTH_COMPONENT;
			else if (this->internalFormat == GL_DEPTH24_STENCIL8)
				this->format = GL_DEPTH_STENCIL;

			if (samples <= 1)
			{
				glBindTexture(GL_TEXTURE_2D, Handle);
				glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, this->format, this->type, data);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else
				throw NotImplementedException();
		}
		virtual void SetData(int width, int height, int samples, DataType inputType, void * data) override
		{
			SetData(0, width, height, samples, inputType, data);
		}
		int GetComponents()
		{
			switch (this->format)
			{
			case GL_RED:
				return 1;
			case GL_RG:
				return 2;
			case GL_RGB:
				return 3;
			case GL_RGBA:
				return 4;
			}
			return 4;
		}
		void GetData(int mipLevel, void * data, int /*bufSize*/)
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			glGetTexImage(GL_TEXTURE_2D, mipLevel, format, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void GetData(int level, DataType outputType, void * data, int /*bufSize*/)
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			if (format == GL_DEPTH_COMPONENT)
			{
				glGetTexImage(GL_TEXTURE_2D, level, format, GL_UNSIGNED_INT, data);

			}
			else
				glGetTexImage(GL_TEXTURE_2D, level, format, TranslateDataTypeToInputType(outputType), data);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void BuildMipmaps()
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};

	class Texture2DArray : public Texture, public GameEngine::Texture2DArray
	{
	public:
		Texture2DArray()
		{
			BindTarget = GL_TEXTURE_2D_ARRAY;
		}
		~Texture2DArray()
		{
			if (Handle)
			{
				glDeleteTextures(1, &Handle);
				Handle = 0;
			}
		}
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void GetSize(int & width, int & height, int & layers)
		{
			glBindTexture(GL_TEXTURE_2D_ARRAY, Handle);
			glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_HEIGHT, &height);
			glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_DEPTH, &layers);

			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		}
		virtual void SetData(int mipLevel, int xOffset, int yOffset, int layerOffset, int width, int height, int layerCount, DataType inputType, void * data) override
		{
			this->internalFormat = TranslateStorageFormat(storageFormat);
			this->format = TranslateDataTypeToFormat(inputType);
			this->type = TranslateDataTypeToInputType(inputType);
			if (this->internalFormat == GL_DEPTH_COMPONENT16 || this->internalFormat == GL_DEPTH_COMPONENT24 || this->internalFormat == GL_DEPTH_COMPONENT32)
				this->format = GL_DEPTH_COMPONENT;
			else if (this->internalFormat == GL_DEPTH24_STENCIL8)
				this->format = GL_DEPTH_STENCIL;
			if (storageFormat == StorageFormat::BC1 || storageFormat == StorageFormat::BC5)
			{
				assert(xOffset == 0 && yOffset == 0);
				int blocks = (int)(ceil(width / 4.0f) * ceil(height / 4.0f));
				int bufferSize = storageFormat == StorageFormat::BC5 ? blocks * 16 : blocks * 8;
				glBindTexture(GL_TEXTURE_2D_ARRAY, Handle);
				glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, mipLevel, internalFormat, width, height, layerOffset, 0, bufferSize, data);
				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			}
			else
			{		
				glBindTexture(GL_TEXTURE_2D_ARRAY, Handle);
				glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipLevel, xOffset, yOffset, layerOffset, width, height, layerCount, this->format, this->type, data);
				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			}
		}
		int GetComponents()
		{
			switch (this->format)
			{
			case GL_RED:
				return 1;
			case GL_RG:
				return 2;
			case GL_RGB:
				return 3;
			case GL_RGBA:
				return 4;
			}
			return 4;
		}
		
		void BuildMipmaps()
		{
			glBindTexture(GL_TEXTURE_2D_ARRAY, Handle);
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		}
	};

	class Texture3D : public Texture, public GameEngine::Texture3D
	{
	public:
		Texture3D()
		{
			BindTarget = GL_TEXTURE_3D;
		}
		~Texture3D()
		{
			if (Handle)
			{
				glDeleteTextures(1, &Handle);
				Handle = 0;
			}
		}
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void GetSize(int & width, int & height, int & layers)
		{
			glBindTexture(GL_TEXTURE_3D, Handle);
			glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &height);
			glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &layers);

			glBindTexture(GL_TEXTURE_3D, 0);
		}
		virtual void SetData(int mipLevel, int xOffset, int yOffset, int layerOffset, int width, int height, int layerCount, DataType inputType, void * data) override
		{
			this->internalFormat = TranslateStorageFormat(storageFormat);
			this->format = TranslateDataTypeToFormat(inputType);
			this->type = TranslateDataTypeToInputType(inputType);
			if (this->internalFormat == GL_DEPTH_COMPONENT16 || this->internalFormat == GL_DEPTH_COMPONENT24 || this->internalFormat == GL_DEPTH_COMPONENT32)
				this->format = GL_DEPTH_COMPONENT;
			else if (this->internalFormat == GL_DEPTH24_STENCIL8)
				this->format = GL_DEPTH_STENCIL;
			if (storageFormat == StorageFormat::BC1 || storageFormat == StorageFormat::BC5)
			{
				assert(xOffset == 0 && yOffset == 0);
				int blocks = (int)(ceil(width / 4.0f) * ceil(height / 4.0f));
				int bufferSize = storageFormat == StorageFormat::BC5 ? blocks * 16 : blocks * 8;
				glBindTexture(GL_TEXTURE_3D, Handle);
				glCompressedTexImage3D(GL_TEXTURE_3D, mipLevel, internalFormat, width, height, layerOffset, 0, bufferSize, data);
				glBindTexture(GL_TEXTURE_3D, 0);
			}
			else
			{
				glBindTexture(GL_TEXTURE_3D, Handle);
				glTexSubImage3D(GL_TEXTURE_3D, mipLevel, xOffset, yOffset, layerOffset, width, height, layerCount, this->format, this->type, data);
				glBindTexture(GL_TEXTURE_3D, 0);
			}
		}
		int GetComponents()
		{
			switch (this->format)
			{
			case GL_RED:
				return 1;
			case GL_RG:
				return 2;
			case GL_RGB:
				return 3;
			case GL_RGBA:
				return 4;
			}
			return 4;
		}
	};

	class TextureCube : public Texture, public GameEngine::TextureCube
	{
	public:
		int size = 0;
		TextureCube(int psize)
		{
			size = psize;
			BindTarget = GL_TEXTURE_CUBE_MAP;
		}
	
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void Resize(int width, int height, int samples)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			for (int i = 0; i < 6; i++)
			{
				if (samples <= 1)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, type, 0);
				else
					glTexImage2DMultisample(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, samples, internalFormat, width, height, GL_TRUE);
			}
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}
		void SetData(TextureCubeFace face, StorageFormat pFormat, int width, int height, int samples, DataType inputType, void * data)
		{
			this->internalFormat = TranslateStorageFormat(pFormat);
			this->format = TranslateDataTypeToFormat(inputType);
			this->type = TranslateDataTypeToInputType(inputType);
			if (this->internalFormat == GL_DEPTH_COMPONENT16 || this->internalFormat == GL_DEPTH_COMPONENT24 || this->internalFormat == GL_DEPTH_COMPONENT32)
				this->format = GL_DEPTH_COMPONENT;
			else if (this->internalFormat == GL_DEPTH24_STENCIL8)
				this->format = GL_DEPTH_STENCIL;
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			if (samples <= 1)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, 0, internalFormat, width, height, 0, this->format, this->type, data);
			else
				glTexImage2DMultisample(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, samples, internalFormat, width, height, GL_TRUE);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}

		void GetData(TextureCubeFace face, int level, DataType outputType, void * data)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, level, TranslateDataTypeToFormat(outputType), TranslateDataTypeToInputType(outputType), data);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}

		void BuildMipmaps()
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}

		virtual void GetSize(int & psize) override
		{
			psize = size;
		}
	};

	class FrameBuffer : public GL_Object
	{
	public:
		static FrameBuffer DefaultFrameBuffer;
		void SetColorRenderTarget(int attachmentPoint, const Texture2D &texture, int level = 0)
		{
			glNamedFramebufferTexture(Handle, GL_COLOR_ATTACHMENT0 + attachmentPoint, texture.Handle, level);
		}
		void SetColorRenderTarget(int attachmentPoint, const Texture2DArray &texture, int layer, int level = 0)
		{
			glNamedFramebufferTextureLayer(Handle, GL_COLOR_ATTACHMENT0 + attachmentPoint, texture.Handle, level, layer);
		}
		void SetColorRenderTarget(int attachmentPoint, const TextureCube &texture, TextureCubeFace face, int level)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Handle);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, texture.Handle, level);
		}
		void SetColorRenderTarget(int attachmentPoint, const RenderBuffer &buffer)
		{
			glNamedFramebufferRenderbuffer(Handle, GL_COLOR_ATTACHMENT0 + attachmentPoint, buffer.Handle, 0);
		}
		void EnableRenderTargets(int mask)
		{
			Array<GLenum, 16> buffers;
			for (int i = 0; i < 16; i++)
			{
				if (mask & (1 << i))
				{
					buffers.Add(GL_COLOR_ATTACHMENT0 + i);
				}
			}
			if (glNamedFramebufferDrawBuffers)
				glNamedFramebufferDrawBuffers(Handle, buffers.Count(), buffers.Buffer());
			else
			{
				glBindFramebuffer(GL_FRAMEBUFFER, Handle);
				glDrawBuffers(buffers.Count(), buffers.Buffer());
			}
		}

		void SetDepthStencilRenderTarget(const Texture2D &texture)
		{
			if (texture.internalFormat == GL_DEPTH24_STENCIL8)
			{
				glNamedFramebufferTexture(Handle, GL_DEPTH_ATTACHMENT, 0, 0);
				glNamedFramebufferTexture(Handle, GL_DEPTH_STENCIL_ATTACHMENT, texture.Handle, 0);
			}
			else
			{
				glNamedFramebufferTexture(Handle, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
				glNamedFramebufferTexture(Handle, GL_DEPTH_ATTACHMENT, texture.Handle, 0);
			}
		}

		void SetDepthStencilRenderTarget(const Texture2DArray &texture, int layer)
		{
			if (texture.internalFormat == GL_DEPTH24_STENCIL8)
			{
				glNamedFramebufferTextureLayer(Handle, GL_DEPTH_ATTACHMENT, 0, 0, layer);
				glNamedFramebufferTextureLayer(Handle, GL_DEPTH_STENCIL_ATTACHMENT, texture.Handle, 0, layer);
			}
			else
			{
				glNamedFramebufferTextureLayer(Handle, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0, layer);
				glNamedFramebufferTextureLayer(Handle, GL_DEPTH_ATTACHMENT, texture.Handle, 0, layer);
			}
		}

		void SetDepthStencilRenderTarget(const TextureCube &texture, TextureCubeFace face)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Handle);
			if (texture.internalFormat == GL_DEPTH24_STENCIL8)
			{
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, 0, 0);
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, texture.Handle, 0);
			}
			else
			{
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, 0, 0);
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, texture.Handle, 0);
			}
		}

		void SetDepthStencilRenderTarget(const RenderBuffer &buffer)
		{
			if (buffer.internalFormat == GL_DEPTH24_STENCIL8)
			{
				glNamedFramebufferRenderbuffer(Handle, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
				glNamedFramebufferRenderbuffer(Handle, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer.Handle);
			}
			else
			{
				glNamedFramebufferRenderbuffer(Handle, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
				glNamedFramebufferRenderbuffer(Handle, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer.Handle);
			}
		}

		void SetStencilRenderTarget(const RenderBuffer &buffer)
		{
			glNamedFramebufferRenderbuffer(Handle, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer.Handle);
		}

		void Check()
		{
			auto rs = glCheckNamedFramebufferStatus(Handle, GL_FRAMEBUFFER);
			if (rs != GL_FRAMEBUFFER_COMPLETE)
			{
				printf("Framebuffer check result: %d", rs);
				throw HardwareRendererException("Inconsistent frame buffer object setup.");
			}
		}
	};

	class FrameBufferDescriptor : public GameEngine::FrameBuffer
	{
	public:
		struct Attachment
		{
			struct
			{
				GLL::Texture2D* tex2D = nullptr;
				GLL::Texture2DArray* tex2DArray = nullptr;
				GLL::TextureCube* texCube = nullptr;
			} handle;
			int layer = -1;
			TextureCubeFace face;
			Attachment(GLL::Texture2D * tex)
			{
				handle.tex2D = tex;
				layer = -1;
			}
			Attachment(GLL::Texture2DArray* texArr, int l)
			{
				handle.tex2DArray = texArr;
				layer = l;
			}
			Attachment(GLL::TextureCube* texCube, TextureCubeFace pface, int l)
			{
				handle.texCube = texCube;
				layer = l;
				face = pface;
			}
			Attachment() = default;
		};
	public:
		CoreLib::List<Attachment> attachments;
	};

	class RenderTargetLayout : public GameEngine::RenderTargetLayout
	{
	public:
		CoreLib::List<AttachmentLayout> attachments;
	private:
		void Resize(int size)
		{
			if (attachments.Count() < size)
				attachments.SetSize(size);
		}

		void SetColorAttachment(int binding, AttachmentLayout layout)
		{
			Resize(binding + 1);
			attachments[binding] = layout;

			//if (samples > 1)
			//{
			//	//TODO: need to resolve?
			//}
		}

		void SetDepthAttachment(int binding, AttachmentLayout layout)
		{
			for (auto attachment : attachments)
			{
				if (attachment.Usage == TextureUsage::DepthAttachment)
					throw HardwareRendererException("Only 1 depth/stencil attachment allowed.");
			}

			Resize(binding + 1);
			attachments[binding] = layout;
		}
	public:
		RenderTargetLayout(CoreLib::ArrayView<AttachmentLayout> bindings)
		{
			int location = 0;
			for (auto binding : bindings)
			{
				switch (binding.Usage)
				{
				case TextureUsage::ColorAttachment:
				case TextureUsage::SampledColorAttachment:
					SetColorAttachment(location, binding);
					break;
				case TextureUsage::DepthAttachment:
				case TextureUsage::SampledDepthAttachment:
					SetDepthAttachment(location, binding);
					break;
				case TextureUsage::Unused:
					break;
				default:
					throw HardwareRendererException("Unsupported attachment usage");
				}
				location++;
			}
		}
		~RenderTargetLayout() {}

		virtual FrameBufferDescriptor* CreateFrameBuffer(const RenderAttachments& renderAttachments) override
		{
#if _DEBUG
			//// Ensure the RenderAttachments are compatible with this RenderTargetLayout
			//for (auto renderAttachment : renderAttachments)
			//{

			//}
			//
			//// Ensure the RenderAttachments are compatible with this RenderTargetLayout
			//for (auto colorReference : colorReferences)
			//{
			//	if (dynamic_cast<RenderAttachments*>(attachments)->usages[colorReference.attachment] != vk::ImageUsageFlagBits::eColorAttachment)
			//		throw HardwareRendererException(L"Incompatible RenderTargetLayout and RenderAttachments");
			//}
			//if (depthReference.layout != vk::ImageLayout::eUndefined)
			//{
			//	if (dynamic_cast<RenderAttachments*>(attachments)->usages[depthReference.attachment] != vk::ImageUsageFlagBits::eDepthStencilAttachment)
			//		throw HardwareRendererException(L"Incompatible RenderTargetLayout and RenderAttachments");
			//}
	#endif
			FrameBufferDescriptor* result = new FrameBufferDescriptor;

			for (auto renderAttachment : renderAttachments.attachments)
			{
				if (renderAttachment.handle.tex2D)
					result->attachments.Add(FrameBufferDescriptor::Attachment(dynamic_cast<GLL::Texture2D*>(renderAttachment.handle.tex2D)));
				else if (renderAttachment.handle.tex2DArray)
					result->attachments.Add(FrameBufferDescriptor::Attachment(dynamic_cast<GLL::Texture2DArray*>(renderAttachment.handle.tex2DArray), renderAttachment.layer));
				else if (renderAttachment.handle.texCube)
					result->attachments.Add(FrameBufferDescriptor::Attachment(dynamic_cast<GLL::TextureCube*>(renderAttachment.handle.texCube), renderAttachment.face, renderAttachment.layer));
			}

			return result;
		}
	};

	class BufferObject : public GL_Object, public GameEngine::Buffer
	{
	public:
#ifdef _DEBUG
		bool * isInTransfer = nullptr;
#endif
		bool persistentMapping = false;
		GLuint BindTarget;
		void * mappedPtr = nullptr;

		static float CheckBufferData(int bufferHandle)
		{
			float buffer;
			glGetNamedBufferSubData(bufferHandle, 0, 4, &buffer);
			return buffer;
		}
		int GetSize()
		{
			int sizeInBytes = 0;
			glGetNamedBufferParameteriv(Handle, GL_BUFFER_SIZE, &sizeInBytes);
			return sizeInBytes;
		}
		void SetData(void * data, int sizeInBytes)
		{
#ifdef _DEBUG
			if (!*isInTransfer)
				throw HardwareRendererException("Renderer not in data-transfer mode.");
#endif
			if (mappedPtr)
				memcpy(mappedPtr, data, sizeInBytes);
			else
				SetData(0, data, sizeInBytes);
		}
		void SetDataAsync(int offset, void * data, int size)
		{
#ifdef _DEBUG
			if (!*isInTransfer)
				throw HardwareRendererException("Renderer not in data-transfer mode.");
#endif
			if (mappedPtr)
				memcpy((char*)mappedPtr + offset, data, size);
			else
				glNamedBufferSubData(Handle, (GLintptr)offset, (GLsizeiptr)size, data);
		}
		void SetData(int offset, void * data, int size)
		{
#ifdef _DEBUG
			if (!*isInTransfer)
				throw HardwareRendererException("Renderer not in data-transfer mode.");
#endif
			if (mappedPtr)
				memcpy((char*)mappedPtr + offset, data, size);
			else 
				glNamedBufferSubData(Handle, (GLintptr)offset, (GLsizeiptr)size, data);
		}
		void BufferStorage(int size, void * data, int flags)
		{
			glNamedBufferStorage(Handle, (GLsizeiptr)size, data, flags);
			if (persistentMapping)
				Map();
		}
		void * Map(BufferAccess access, int offset, int len)
		{
			if (persistentMapping)
			{
				if (mappedPtr)
					return mappedPtr;
				else
				{
					mappedPtr = glMapNamedBufferRange(Handle, offset, len, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT);
					return mappedPtr;
				}
			}
			else
				return glMapNamedBufferRange(Handle, offset, len, TranslateBufferAccess(access));
		}
		void * Map(int offset, int size)
		{
			return Map(BufferAccess::ReadWrite, offset, size);
		}
		void * Map()
		{
			return Map(0, GetSize());
		}
		void Unmap()
		{
			mappedPtr = nullptr;
			glUnmapNamedBuffer(Handle);
		}
		void Flush(int /*offset*/, int /*size*/)
		{
			//glFlushMappedNamedBufferRange(Handle, offset, size);
		}
		void Flush()
		{
			//glFlushMappedNamedBufferRange();
		}
		bool GetData(void * buffer, int & bufferSize)
		{
			int sizeInBytes = 0;
			glGetNamedBufferParameteriv(Handle, GL_BUFFER_SIZE, &sizeInBytes);
			if (sizeInBytes > bufferSize)
			{
				bufferSize = sizeInBytes;
				return false;
			}
			glGetNamedBufferSubData(Handle, 0, sizeInBytes, buffer);
			return true;
		}
		void MakeResident(bool isReadOnly)
		{
			glMakeNamedBufferResidentNV(Handle, isReadOnly ? GL_READ_ONLY : GL_READ_WRITE);
		}
		void MakeNonResident()
		{
			glMakeNamedBufferNonResidentNV(Handle);
		}
	};

	class VertexArray : public GL_Object
	{
	private:
		int lastAttribCount = 0;
		GLuint lastBoundVBO = 0;
		int lastBufferOffset = 0;
		struct AttribState
		{
			bool activated = false;
			DataType dataType;
			int vertSize = 0, ptr = 0, divisor = 0;
		};
		AttribState attribStates[32];
	public:
		void SetIndex(const BufferObject & indices)
		{
			//glBindVertexArray(Handle);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices.Handle);
		}
		void SetVertex(const BufferObject & vertices, ArrayView<VertexAttributeDesc> attribs, int vertSize, int bufferOffset, int startId = 0, int instanceDivisor = 0)
		{
			bool bufferChanged = false;
			//glBindVertexArray(Handle);
			if (lastBoundVBO != vertices.Handle)
			{
				glBindBuffer(GL_ARRAY_BUFFER, vertices.Handle);
				lastBoundVBO = vertices.Handle;
				bufferChanged = true;
			}
			if (bufferOffset != lastBufferOffset)
			{
				lastBufferOffset = bufferOffset;
				bufferChanged = true;
			}
			for (int i = attribs.Count(); i < lastAttribCount; i++)
			{
				glDisableVertexAttribArray(i);
				attribStates[i].activated = false;
				attribStates[i].ptr = -1;
				attribStates[i].divisor = -1;
			}
			for (int i = 0; i < attribs.Count(); i++)
			{
				int id = i + startId;
				auto & attrib = attribs[i];
				if (attrib.Location != -1)
					id = attrib.Location;
				if (!attribStates[id].activated)
				{
					glEnableVertexAttribArray(id);
					attribStates[id].activated = true;
				}
				if (bufferChanged || attribStates[id].dataType != attrib.Type || attribStates[id].vertSize != vertSize ||
					attribStates[id].ptr != attrib.StartOffset)
				{
					if (attrib.Type == DataType::Int || attrib.Type == DataType::Int2 || attrib.Type == DataType::Int3 || attrib.Type == DataType::Int4
						|| attrib.Type == DataType::UInt)
						glVertexAttribIPointer(id, GetDataTypeComponenets(attrib.Type), TranslateDataTypeToInputType(attrib.Type), vertSize, (void*)(CoreLib::PtrInt)(attrib.StartOffset + bufferOffset));
					else
						glVertexAttribPointer(id, GetDataTypeComponenets(attrib.Type), TranslateDataTypeToInputType(attrib.Type), attrib.Normalized, vertSize, (void*)(CoreLib::PtrInt)(attrib.StartOffset + bufferOffset));
					attribStates[id].dataType = attrib.Type;
					attribStates[id].vertSize = vertSize;
					attribStates[id].ptr = attribs[i].StartOffset;
				}
				if (attribStates[id].divisor != instanceDivisor)
				{
					glVertexAttribDivisor(id, instanceDivisor);
					attribStates[id].divisor = instanceDivisor;
				}
			}
			lastAttribCount = attribs.Count() + startId;
		}
	};

	class TransformFeedback : public GL_Object
	{
	public:
		void BindBuffer(int index, BufferObject buffer)
		{
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, Handle);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer.Handle);
		}
		void Use(PrimitiveMode mode)
		{
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, Handle);
			glBeginTransformFeedback((int)mode);
		}
		void Unuse()
		{
			glEndTransformFeedback();
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		}
	};

	class Shader : public GL_Object, public GameEngine::Shader
	{
	public:
		ShaderType stage;
		GLuint GetHandle()
		{
			return Handle;
		}
	};

	class Program : public GL_Object 
	{
	public:
		void StorageBlockBinding(String blockName, int binding)
		{
			int index = glGetProgramResourceIndex(Handle, GL_SHADER_STORAGE_BLOCK, blockName.Buffer());
			glShaderStorageBlockBinding(Handle, index, binding);
		}
		void UniformBlockBinding(String blockName, int binding)
		{
			int index = glGetProgramResourceIndex(Handle, GL_UNIFORM_BLOCK, blockName.Buffer());
			glUniformBlockBinding(Handle, index, binding);
		}
		void UniformBlockBinding(int index, int binding)
		{
			glUniformBlockBinding(Handle, index, binding);
		}
		int GetAttribBinding(String name)
		{
			return glGetAttribLocation(Handle, name.Buffer());
		}
		int GetOutputLocation(String name)
		{
			return glGetFragDataLocation(Handle, name.Buffer());
		}
		void SetUniform(String name, unsigned int value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform1ui(Handle, loc, value);
		}
		void SetUniform(String name, int value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform1i(Handle, loc, value);
		}
		void SetUniform(String name, float value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform1f(Handle, loc, value);
		}
		void SetUniform(String name, Vec2 value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform2fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(String name, Vec3 value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform3fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(String name, Vec4 value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform4fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(String name, Vec2i value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform2iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(String name, Vec3i value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform3iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(String name, Vec4i value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniform4iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(String name, Matrix3 value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniformMatrix3fv(Handle, loc, 1, false, (float*)&value);
		}
		void SetUniform(String name, Matrix4 value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniformMatrix4fv(Handle, loc, 1, false, (float*)&value);
		}
		void SetUniform(String name, uint64_t value)
		{
			int loc = glGetUniformLocation(Handle, name.Buffer());
			if (loc != -1)
				glProgramUniformui64NV(Handle, loc, value);
		}
		void SetUniform(int loc, int value)
		{
			if (loc != -1)
				glProgramUniform1i(Handle, loc, value);
		}
		void SetUniform(int loc, Vec2i value)
		{
			if (loc != -1)
				glProgramUniform2iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(int loc, Vec3i value)
		{
			if (loc != -1)
				glProgramUniform3iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(int loc, Vec4i value)
		{
			if (loc != -1)
				glProgramUniform4iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(int loc, uint64_t value)
		{
			if (loc != -1)
				glProgramUniformui64NV(Handle, loc, value);
		}
		void SetUniform(int loc, float value)
		{
			if (loc != -1)
				glProgramUniform1f(Handle, loc, value);
		}
		void SetUniform(int loc, Vec2 value)
		{
			if (loc != -1)
				glProgramUniform2fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(int loc, Vec3 value)
		{
			if (loc != -1)
				glProgramUniform3fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(int loc, Vec4 value)
		{
			if (loc != -1)
				glProgramUniform4fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(int loc, Matrix3 value)
		{
			if (loc != -1)
				glProgramUniformMatrix3fv(Handle, loc, 1, false, (float*)&value);
		}
		void SetUniform(int loc, Matrix4 value)
		{
			if (loc != -1)
				glProgramUniformMatrix4fv(Handle, loc, 1, false, (float*)&value);
			//	glUniformMatrix4fv(loc, 1, false, (float*)&value);
		}
		int GetUniformLoc(String name)
		{
			return glGetUniformLocation(Handle, name.Buffer());
		}
		int GetUniformLoc(const char * name)
		{
			return glGetUniformLocation(Handle, name);
		}
		int GetUniformBlockIndex(String name)
		{
			return glGetUniformBlockIndex(Handle, name.Buffer());
		}
		void Use()
		{
			glUseProgram(Handle);
		}
		void Link()
		{
			int length, compileStatus;
			List<char> buffer;
			CoreLib::Diagnostics::DebugWriter dbgWriter;

			glLinkProgram(Handle);

			glGetProgramiv(Handle, GL_INFO_LOG_LENGTH, &length);
			buffer.SetSize(length);
			glGetProgramInfoLog(Handle, length, &length, buffer.Buffer());
			glGetProgramiv(Handle, GL_LINK_STATUS, &compileStatus);
			String logOutput(buffer.Buffer());
			if (length > 0)
				dbgWriter << logOutput;
			if (compileStatus != GL_TRUE)
			{
				throw HardwareRendererException("program linking\n" + logOutput);
			}

			glValidateProgram(Handle);
			glGetProgramiv(Handle, GL_INFO_LOG_LENGTH, &length);
			buffer.SetSize(length);
			glGetProgramInfoLog(Handle, length, &length, buffer.Buffer());
			logOutput = buffer.Buffer();
			if (length > 0)
				dbgWriter << logOutput;
			glGetProgramiv(Handle, GL_VALIDATE_STATUS, &compileStatus);
			if (compileStatus != GL_TRUE)
			{
				throw HardwareRendererException("program validation\n" + logOutput);
			}
		}
	};

	void __stdcall GL_DebugCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum /*severity*/, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
	{
		if (!DebugErrorEnabled)
			return;
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			CoreLib::Diagnostics::Debug::Write("[GL Error] ");
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			CoreLib::Diagnostics::Debug::Write("[GL Performance] ");
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			CoreLib::Diagnostics::Debug::Write("[GL Portability] ");
			break;
		default:
			return;
		}
		CoreLib::Diagnostics::Debug::WriteLine(String(message));
		if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
		{
			printf("%s\n", message);
			CoreLib::Diagnostics::Debug::WriteLine("--------");
		}
	}

	struct BindingLayout
	{
		BindingType Type = BindingType::Unused;
		CoreLib::List<int> BindingPoints;
	};

	struct Descriptor
	{
		BindingType Type = BindingType::Unused;
		int Offset = 0, Length = 0;
		union
		{
			GLL::Texture * texture = nullptr;
			GLL::TextureSampler * sampler;
			GLL::BufferObject * buffer;
		} binding;
	};

	class DescriptorSetLayout : public GameEngine::DescriptorSetLayout
	{
	public:
		List<BindingLayout> layouts;
	};

	class DescriptorSet : public GameEngine::DescriptorSet
	{
	public:
		List<Descriptor> descriptors;
		virtual void BeginUpdate() {}
		virtual void Update(int location, GameEngine::Texture* texture, TextureAspect /*aspect*/) override
		{
			if (location >= descriptors.Count())
				throw HardwareRendererException("descriptor set write out of bounds.");
			auto & desc = descriptors[location];
			desc.binding.texture = dynamic_cast<GLL::Texture*>(texture);
			desc.Offset = desc.Length = 0;
			desc.Type = BindingType::Texture;
		}
		virtual void Update(int location, GameEngine::TextureSampler* sampler) override
		{
			if (location >= descriptors.Count())
				throw HardwareRendererException("descriptor set write out of bounds.");
			auto & desc = descriptors[location];
			desc.binding.sampler = dynamic_cast<GLL::TextureSampler*>(sampler);
			desc.Offset = desc.Length = 0;
			desc.Type = BindingType::Sampler;
		}
		virtual void Update(int location, Buffer* buffer, int offset = 0, int length = -1) override
		{
			if (location >= descriptors.Count())
				throw HardwareRendererException("descriptor set write out of bounds.");
			auto & desc = descriptors[location];
			desc.binding.buffer = dynamic_cast<GLL::BufferObject*>(buffer);
			desc.Offset = offset;
			desc.Length = length;
			desc.Type = BindingType::UniformBuffer; // only suggests this is a buffer; actual type is stated in pipeline layout
		}
		virtual void EndUpdate() {}
	};

	struct PipelineSettings
	{
		Program program;
		VertexFormat format;
		bool primitiveRestart = false;
		PrimitiveType primitiveType = PrimitiveType::Triangles;
		int patchSize = 0;
		CompareFunc DepthCompareFunc = CompareFunc::Disabled;
		CompareFunc StencilCompareFunc = CompareFunc::Disabled;
		StencilOp StencilFailOp = StencilOp::Replace, StencilDepthFailOp = StencilOp::Replace, StencilDepthPassOp = StencilOp::Replace;
		BlendMode BlendMode = BlendMode::Replace;
		unsigned int StencilMask = 255, StencilReference = 0;
		CullMode CullMode = CullMode::Disabled;
		bool enablePolygonOffset = false;
		float polygonOffsetFactor = 0.0f, polygonOffsetUnits = 0.0f;
		List<List<BindingLayout>> bindingLayout; // indexed by bindingLayout[setid][location]
	};

	class Pipeline : public GameEngine::Pipeline
	{
	public:
		PipelineSettings settings;
		String Name;
		Pipeline(const PipelineSettings & pSettings)
			: settings(pSettings)
		{}
	};

	class PipelineBuilder : public GameEngine::PipelineBuilder
	{
	public:
		String debugName;
		Program shaderProgram;
		VertexFormat format;
		List<DescriptorSetLayout*> descLayouts;
		bool isTessellation = false;
		virtual void SetShaders(CoreLib::ArrayView<GameEngine::Shader*> shaders) override
		{
#if _DEBUG
			bool vertPresent = false;
			bool tessControlPresent = false;
			bool tessEvalPresent = false;
			//bool geometryPresent = false;
			bool fragPresent = false;
			bool computePresent = false;
#endif
			auto handle = glCreateProgram();
			for (auto shader : shaders)
			{
#if _DEBUG
				// Ensure only one of any shader stage is present in the requested shader stage
				switch (dynamic_cast<Shader*>(shader)->stage)
				{
				case ShaderType::VertexShader:
					assert(vertPresent == false);
					vertPresent = true;
					break;
				case ShaderType::FragmentShader:
					assert(fragPresent == false);
					fragPresent = true;
					break;
				case ShaderType::ComputeShader:
					assert(computePresent == false);
					computePresent = true;
					break;
				case ShaderType::HullShader:
					assert(tessControlPresent == false);
					tessControlPresent = true;
					break;
				case ShaderType::DomainShader:
					assert(tessEvalPresent == false);
					tessEvalPresent = true;
					break;
				default:
					throw HardwareRendererException("Unknown shader stage");
				}
#endif
				if (dynamic_cast<Shader*>(shader)->stage == ShaderType::HullShader)
					isTessellation = true;
				glAttachShader(handle, dynamic_cast<Shader*>(shader)->GetHandle());
			}

			shaderProgram.Handle = handle;
			shaderProgram.Link();
			//TODO: need to destroy this at some point
		}
		virtual void SetVertexLayout(VertexFormat vertexFormat) override
		{
			format = vertexFormat;
		}
		virtual void SetBindingLayout(CoreLib::ArrayView<GameEngine::DescriptorSetLayout*> descriptorSets) override
		{
			descLayouts.Clear();
			for (auto descSet : descriptorSets)
			{
				descLayouts.Add((DescriptorSetLayout*)descSet);
			}
		}
		virtual void SetDebugName(String name) override
		{
			debugName = name;
		}
		virtual Pipeline* ToPipeline(GameEngine::RenderTargetLayout* /*renderTargetLayout*/) override
		{
			PipelineSettings settings;
			settings.format = format;
			settings.primitiveRestart = FixedFunctionStates.PrimitiveRestartEnabled;
			if (isTessellation)
			{
				switch (FixedFunctionStates.PrimitiveTopology)
				{
				case PrimitiveType::Lines:
					settings.patchSize = 2;
					break;
				case PrimitiveType::Triangles:
					settings.patchSize = 3;
					break;
				case PrimitiveType::Quads:
					settings.patchSize = 4;
					break;
				case PrimitiveType::Patches:
					settings.patchSize = FixedFunctionStates.PatchSize;
					break;
				default:
					throw InvalidOperationException("invalid primitive type for tessellation.");
				}
				settings.primitiveType = PrimitiveType::Patches;
			}
			else
				settings.primitiveType = FixedFunctionStates.PrimitiveTopology;
			settings.program = shaderProgram;
			
			settings.DepthCompareFunc = FixedFunctionStates.DepthCompareFunc;
			settings.StencilCompareFunc = FixedFunctionStates.StencilCompareFunc;
			settings.StencilFailOp = FixedFunctionStates.StencilFailOp;
			settings.StencilDepthFailOp = FixedFunctionStates.StencilDepthFailOp;
			settings.StencilDepthPassOp = FixedFunctionStates.StencilDepthPassOp;
			settings.StencilMask = FixedFunctionStates.StencilMask;
			settings.StencilReference = FixedFunctionStates.StencilReference;
			settings.CullMode = FixedFunctionStates.CullMode;

			settings.BlendMode = FixedFunctionStates.BlendMode;
			settings.enablePolygonOffset = FixedFunctionStates.EnablePolygonOffset;
			settings.polygonOffsetFactor = FixedFunctionStates.PolygonOffsetFactor;
			settings.polygonOffsetUnits = FixedFunctionStates.PolygonOffsetUnits;

			settings.bindingLayout.SetSize(descLayouts.Count());
			for (int i = 0; i < descLayouts.Count(); i++)
			{
				if (descLayouts[i])
					settings.bindingLayout[i].AddRange(descLayouts[i]->layouts);
			}
			auto rs = new Pipeline(settings);
			rs->Name = debugName;
			return rs;
		}
	};

	enum class Command
	{
		SetViewport,
		BindVertexBuffer,
		BindIndexBuffer,
		BindPipeline,
		BindDescriptorSet,
		Draw,
		DrawInstanced,
		DrawIndexed,
		DrawIndexedInstanced,
		Blit,
		ClearAttachments,
	};

	struct SetViewportData
	{
		int x = 0, y = 0, width = 0, height = 0;
	};
	struct PipelineData
	{
		Pipeline* pipeline;
	};
	struct DrawData
	{
		int instances;
		int first;
		int count;
	};
	struct BlitData
	{
		GameEngine::Texture2D* dst;
		GameEngine::Texture2D* src;
	};
	struct AttachmentData
	{
		int drawBufferMask = 1;
		bool depth = true, stencil = true;
	};
	struct BindDescriptorSetData
	{
		DescriptorSet * descSet;
		int location = 0;
	};
	struct BindBufferData
	{
		BufferObject * buffer;
		int offset;
	};
	class CommandData
	{
	public:
		Command command;
		CommandData() {}
		union
		{
			SetViewportData viewport;
			BindBufferData vertexBufferBinding;
			PipelineData pipelineData;
			DrawData draw;
			BlitData blit;
			AttachmentData clear;
			BindDescriptorSetData bindDesc;
		};
	};

	class CommandBuffer : public GameEngine::CommandBuffer
	{
	public:
		CoreLib::List<CommandData> buffer;
	public:
		virtual void BeginRecording() override
		{
			buffer.Clear();
		}
		virtual void BeginRecording(GameEngine::FrameBuffer* /*frameBuffer*/) override
		{
			buffer.Clear();
		}
		virtual void BeginRecording(GameEngine::RenderTargetLayout* /*renderTargetLayout*/) override
		{
			buffer.Clear();
		}
		virtual void EndRecording() override { /*Do nothing*/ }
		virtual void SetViewport(int x, int y, int width, int height) override
		{
			CommandData data;
			data.command = Command::SetViewport;
			data.viewport.x = x;
			data.viewport.y = y;
			data.viewport.width = width;
			data.viewport.height = height;
			buffer.Add(data);
		}
		virtual void BindVertexBuffer(GameEngine::Buffer* vertexBuffer, int offset) override
		{
			CommandData data;
			data.command = Command::BindVertexBuffer;
			data.vertexBufferBinding.buffer = reinterpret_cast<BufferObject*>(vertexBuffer);
			data.vertexBufferBinding.offset = offset;
			buffer.Add(data);
		}
		virtual void BindIndexBuffer(GameEngine::Buffer* indexBuffer, int offset) override
		{
			CommandData data;
			data.command = Command::BindIndexBuffer;
			data.vertexBufferBinding.buffer = reinterpret_cast<BufferObject*>(indexBuffer);
			data.vertexBufferBinding.offset = offset;
			buffer.Add(data);
		}
		virtual void BindPipeline(GameEngine::Pipeline* pipeline) override
		{
			CommandData data;
			data.command = Command::BindPipeline;
			data.pipelineData.pipeline = reinterpret_cast<GLL::Pipeline*>(pipeline);
			buffer.Add(data);
		}
		virtual void BindDescriptorSet(int binding, GameEngine::DescriptorSet* descSet) override
		{
			CommandData data;
			data.command = Command::BindDescriptorSet;
			data.bindDesc.descSet = reinterpret_cast<GLL::DescriptorSet*>(descSet);
			data.bindDesc.location = binding;
			buffer.Add(data);
		}
		virtual void Draw(int firstVertex, int vertexCount) override
		{
			CommandData data;
			data.command = Command::Draw;
			data.draw.first = firstVertex;
			data.draw.count = vertexCount;
			buffer.Add(data);
		}
		virtual void DrawInstanced(int numInstances, int firstVertex, int vertexCount) override
		{
			CommandData data;
			data.command = Command::DrawInstanced;
			data.draw.instances = numInstances;
			data.draw.first = firstVertex;
			data.draw.count = vertexCount;
			buffer.Add(data);
		}
		virtual void DrawIndexed(int firstIndex, int indexCount) override
		{
			CommandData data;
			data.command = Command::DrawIndexed;
			data.draw.first = firstIndex;
			data.draw.count = indexCount;
			buffer.Add(data);
		}
		virtual void DrawIndexedInstanced(int numInstances, int firstIndex, int indexCount) override
		{
			CommandData data;
			data.command = Command::DrawIndexedInstanced;
			data.draw.instances = numInstances;
			data.draw.first = firstIndex;
			data.draw.count = indexCount;
			buffer.Add(data);
		}
		virtual void TransferLayout(const RenderAttachments& /*attachments*/, ArrayView<TextureUsage> /*layouts*/) override
		{
		}
		virtual void Blit(GameEngine::Texture2D* dstImage, GameEngine::Texture2D* srcImage) override
		{
			CommandData data;
			data.command = Command::Blit;
			data.blit.dst = dstImage;
			data.blit.src = srcImage;
			buffer.Add(data);
		}
		void ClearAttachmentsImpl(ArrayView<TextureUsage> attachments)
		{
			CommandData data;
			data.command = Command::ClearAttachments;
            data.clear = GLL::AttachmentData();
			int i = 0;
			for (auto & attach : attachments)
			{
				StorageFormat f = StorageFormat::Invalid;
				bool isDepth = (attach == TextureUsage::DepthAttachment || attach == TextureUsage::SampledDepthAttachment);
				i++;
				if (isDepth)
				{
					data.clear.depth = true;
					data.clear.stencil = true;
				}
				else if (f != StorageFormat::Invalid)
					data.clear.drawBufferMask |= (1 << i);
			}
			buffer.Add(data);
		}
		virtual void ClearAttachments(GameEngine::FrameBuffer * frameBuffer) override
		{
			auto & attachments = ((GLL::FrameBufferDescriptor*)frameBuffer)->attachments;
			Array<TextureUsage, 16> usage;
			for (auto & attach : attachments)
			{
				StorageFormat f = StorageFormat::Invalid;
				if (attach.handle.tex2D)
					f = dynamic_cast<GLL::Texture2D*>(attach.handle.tex2D)->storageFormat;
				else if (attach.handle.tex2DArray)
					f = dynamic_cast<GLL::Texture2DArray*>(attach.handle.tex2DArray)->storageFormat;
				if (isDepthFormat(f))
					usage.Add(TextureUsage::DepthAttachment);
				else
					usage.Add(TextureUsage::ColorAttachment);
			}
			ClearAttachmentsImpl(usage.GetArrayView());
		}
	};

	class HardwareRenderer : public GameEngine::HardwareRenderer
	{
	private:
		HWND hwnd;
		HDC hdc;
		HGLRC hrc;
		int width;
		int height;
		VertexArray currentVAO;
		FrameBuffer srcFrameBuffer;
		FrameBuffer dstFrameBuffer;
		struct BufferBindState
		{
			GLuint buffer = 0;
			int start = 0;
			int length = -1;
		};
		BufferBindState bufferBindState[4][64];
		GLuint textureBindState[160];
		GLuint samplerBindState[160];
		PipelineSettings currentFixedFuncState;
	private:
		void FindExtensionSubstitutes()
		{
			if (!glNamedBufferData)
			{
				glNamedBufferData = glNamedBufferDataEXT;
				glNamedBufferStorage = glNamedBufferStorageEXT;
				glMapNamedBuffer = glMapNamedBufferEXT;
				glMapNamedBufferRange = glMapNamedBufferRangeEXT;
				glUnmapNamedBuffer = glUnmapNamedBufferEXT;
				glGetNamedBufferParameteriv = glGetNamedBufferParameterivEXT;
				glGetNamedBufferSubData = glGetNamedBufferSubDataEXT;
				glNamedFramebufferRenderbuffer = glNamedFramebufferRenderbufferEXT;
				glNamedFramebufferTexture = glNamedFramebufferTextureEXT;
				glCheckNamedFramebufferStatus = glCheckNamedFramebufferStatusEXT;
			}
		}
	public:
		bool isInDataTransfer = false;
		HardwareRenderer()
		{
			hwnd = 0;
			hdc = 0;
			hrc = 0;
			for (int i = 0; i < 160; i++)
			{
				textureBindState[i] = 0;
				samplerBindState[i] = 0;
			}
		}
		~HardwareRenderer()
		{
			DestroyVertexArray(currentVAO);
			DestroyFrameBuffer(srcFrameBuffer);
			DestroyFrameBuffer(dstFrameBuffer);
		}
		virtual void * GetWindowHandle() override
		{
			return this->hwnd;
		}
		void BindWindow(void* windowHandle, int pwidth, int pheight)
		{
			this->hwnd = (HWND)windowHandle;
			this->width = pwidth;
			this->height = pheight;
			hdc = GetDC(hwnd); // Get the device context for our window

			PIXELFORMATDESCRIPTOR pfd; // Create a new PIXELFORMATDESCRIPTOR (PFD)
			memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR)); // Clear our  PFD
			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); // Set the size of the PFD to the size of the class
			pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW; // Enable double buffering, opengl support and drawing to a window
			pfd.iPixelType = PFD_TYPE_RGBA; // Set our application to use RGBA pixels
			pfd.cColorBits = 32; // Give us 32 bits of color information (the higher, the more colors)
			pfd.cDepthBits = 24; // Give us 32 bits of depth information (the higher, the more depth levels)
			pfd.cStencilBits = 8;
			pfd.iLayerType = PFD_MAIN_PLANE; // Set the layer of the PFD

			int nPixelFormat = ChoosePixelFormat(hdc, &pfd); // Check if our PFD is valid and get a pixel format back
			if (nPixelFormat == 0) // If it fails
				throw HardwareRendererException("Requried pixel format is not supported.");

			auto bResult = SetPixelFormat(hdc, nPixelFormat, &pfd); // Try and set the pixel format based on our PFD
			if (!bResult) // If it fails
				throw HardwareRendererException("Requried pixel format is not supported.");

			HGLRC tempOpenGLContext = wglCreateContext(hdc); // Create an OpenGL 2.1 context for our device context
			wglMakeCurrent(hdc, tempOpenGLContext); // Make the OpenGL 2.1 context current and active
			GLenum error = glewInit(); // Enable GLEW

			if (error != GLEW_OK) // If GLEW fails
				throw HardwareRendererException("Failed to load OpenGL.");

			int contextFlags = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
#ifdef _DEBUG
			contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

			int attributes[] = 
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, TargetOpenGLVersion_Major, 
				WGL_CONTEXT_MINOR_VERSION_ARB, TargetOpenGLVersion_Minor, 
				WGL_CONTEXT_FLAGS_ARB, contextFlags, // Set our OpenGL context to be forward compatible
				0
			};

			if (wglewIsSupported("WGL_ARB_create_context") == 1) 
			{ 
				// If the OpenGL 3.x context creation extension is available
				hrc = wglCreateContextAttribsARB(hdc, NULL, attributes); // Create and OpenGL 3.x context based on the given attributes
				wglMakeCurrent(NULL, NULL); // Remove the temporary context from being active
				wglDeleteContext(tempOpenGLContext); // Delete the temporary OpenGL 2.1 context
				wglMakeCurrent(hdc, hrc); // Make our OpenGL 3.0 context current
			}
			else 
			{
				hrc = tempOpenGLContext; // If we didn't have support for OpenGL 3.x and up, use the OpenGL 2.1 context
			}

			int glVersion[2] = { 0, 0 }; // Set some default values for the version
			glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]); // Get back the OpenGL MAJOR version we are using
			glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]); // Get back the OpenGL MAJOR version we are using

			CoreLib::Diagnostics::Debug::WriteLine("Using OpenGL: " + String(glVersion[0]) + "." + String(glVersion[1])); // Output which version of OpenGL we are using
			if (glVersion[0] < TargetOpenGLVersion_Major || (glVersion[0] == TargetOpenGLVersion_Major && glVersion[1] < TargetOpenGLVersion_Minor))
			{
				// supported OpenGL version is too low
				throw HardwareRendererException("OpenGL" + String(TargetOpenGLVersion_Major) + "." + String(TargetOpenGLVersion_Minor) + " is not supported.");
			}
			if (glDebugMessageCallback)
				glDebugMessageCallback(GL_DebugCallback, this);
//#ifdef _DEBUG
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
//#endif
			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

			FindExtensionSubstitutes();
			// Set up source/dest framebuffer for presenting images
			srcFrameBuffer = CreateFrameBuffer();
			dstFrameBuffer = CreateFrameBuffer();
			currentVAO = CreateVertexArray();
			wglSwapIntervalEXT(0);
			glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
		}

		virtual void Resize(int pwidth, int pheight) override
		{
			width = pwidth;
			height = pheight;
		}

		virtual int GetSpireTarget() override
		{
			return SPIRE_GLSL;
		}

		virtual void ClearTexture(GameEngine::Texture2D* texture) override
		{
			SetWriteFrameBuffer(srcFrameBuffer);
			switch (reinterpret_cast<GLL::Texture2D*>(texture)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(texture));
				Clear(1, 0, 0);
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(texture));
				Clear(1, 0, 1);
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(texture));
				srcFrameBuffer.EnableRenderTargets(1);
				Clear(0, 1, 0);
				srcFrameBuffer.SetColorRenderTarget(0, Texture2D());
				break;
			}
		}

		virtual GameEngine::Fence* CreateFence() override
		{
			return new GLL::Fence();
		}

		virtual void ExecuteCommandBuffers(GameEngine::FrameBuffer* frameBuffer, CoreLib::ArrayView<GameEngine::CommandBuffer*> commands, GameEngine::Fence * fence) override
		{
			auto fb = (GLL::FrameBufferDescriptor*)(frameBuffer);
			auto setupFrameBuffer = [&]()
			{
				int location = 0;
				int mask = 0;
				for (auto renderAttachment : fb->attachments)
				{
					bool isDepth = false;
					if (renderAttachment.handle.tex2D)
						isDepth = isDepthFormat(renderAttachment.handle.tex2D->storageFormat);
					else if(renderAttachment.handle.tex2DArray)
						isDepth = isDepthFormat(renderAttachment.handle.tex2DArray->storageFormat);
					else if (renderAttachment.handle.texCube)
						isDepth = isDepthFormat(renderAttachment.handle.texCube->storageFormat);
					if (isDepth)
					{
						if (renderAttachment.handle.tex2D)
							srcFrameBuffer.SetDepthStencilRenderTarget(*renderAttachment.handle.tex2D);
						else if (renderAttachment.handle.tex2DArray)
							srcFrameBuffer.SetDepthStencilRenderTarget(*renderAttachment.handle.tex2DArray, renderAttachment.layer);
					}
					else
					{
						if (renderAttachment.handle.tex2D)
							srcFrameBuffer.SetColorRenderTarget(location, *renderAttachment.handle.tex2D);
						else if (renderAttachment.handle.tex2DArray)
							srcFrameBuffer.SetColorRenderTarget(location, *renderAttachment.handle.tex2DArray, renderAttachment.layer);
						else if (renderAttachment.handle.texCube)
							srcFrameBuffer.SetColorRenderTarget(location, *renderAttachment.handle.texCube, renderAttachment.face, renderAttachment.layer);
						mask |= (1 << location);
					}
					location++;
				}
				srcFrameBuffer.EnableRenderTargets(mask);
			};

			setupFrameBuffer();

			// Prepare framebuffer
			SetWriteFrameBuffer(srcFrameBuffer);
			//SetViewport(0, 0, width, height);
			//SetZTestMode(CompareFunc::Disabled);
			//SetStencilMode(StencilMode());
			//currentFixedFuncState.DepthCompareFunc = CompareFunc::Disabled;
			// Execute command buffer
			
			Pipeline * currentPipeline = nullptr;
			BufferObject* currentVertexBuffer = nullptr, *currentIndexBuffer = nullptr;
			int currentVertexBufferOffset = 0; int currentIndexBufferOffset = 0;
			PrimitiveType primType = PrimitiveType::Triangles;
			Array<DescriptorSet*, 32> boundDescSets;
			boundDescSets.SetSize(boundDescSets.GetCapacity());
			for (auto & set : boundDescSets)
				set = nullptr;
			auto updateBindings = [&]()
			{
				if (currentPipeline)
				{
					for (int i = 0; i<currentPipeline->settings.bindingLayout.Count(); i++)
					{
						auto & descLayout = currentPipeline->settings.bindingLayout[i];
						auto descSet = boundDescSets[i];
						if (!descSet) continue;
						if (descSet->descriptors.Count() != descLayout.Count() && descLayout.Count() != 0)
							throw HardwareRendererException("bound descriptor set does not match descriptor set layout.");
						for (int j = 0; j < descLayout.Count(); j++)
						{
							auto & desc = descSet->descriptors[j];
							auto & layout = descLayout[j];
							if (!desc.binding.buffer || layout.BindingPoints.Count() == 0)
								continue;
							if (layout.Type == BindingType::Texture && desc.Type == BindingType::Texture)
							{
								UseTexture(layout.BindingPoints.First(), *desc.binding.texture);
							}
							else if (layout.Type == BindingType::Sampler && desc.Type == BindingType::Sampler)
							{
								for (auto binding : layout.BindingPoints)
									UseSampler(binding, *desc.binding.sampler);
							}
							else if (layout.Type == BindingType::UniformBuffer && (desc.Type == BindingType::UniformBuffer || desc.Type == BindingType::StorageBuffer))
							{
								if (desc.Offset == 0 && desc.Length == -1)
									BindBuffer(BufferType::UniformBuffer, layout.BindingPoints.First(), desc.binding.buffer->Handle);
								else
									BindBuffer(BufferType::UniformBuffer, layout.BindingPoints.First(), desc.binding.buffer->Handle, desc.Offset, desc.Length);
							}
							else if (layout.Type == BindingType::StorageBuffer && (desc.Type == BindingType::UniformBuffer || desc.Type == BindingType::StorageBuffer))
							{
								if (desc.Offset == 0 && desc.Length == -1)
									BindBuffer(BufferType::StorageBuffer, layout.BindingPoints.First(), desc.binding.buffer->Handle);
								else
									BindBuffer(BufferType::StorageBuffer, layout.BindingPoints.First(), desc.binding.buffer->Handle, desc.Offset, desc.Length);
							}
							else
								throw HardwareRendererException("descriptor type does not match descriptor layout description");
						}
					}
				}
				else
					throw HardwareRendererException("must bind pipeline before binding descriptor set.");
			};
			BindVertexArray(currentVAO);
			for (auto commandBuffer : commands)
			{
				for (auto & command : reinterpret_cast<GLL::CommandBuffer*>(commandBuffer)->buffer)
				{
					switch (command.command)
					{
					case Command::SetViewport:
						SetViewport(command.viewport.x, command.viewport.y, command.viewport.width, command.viewport.height);
						break;
					case Command::BindVertexBuffer:
					{
						if (currentVertexBuffer != command.vertexBufferBinding.buffer || currentVertexBufferOffset != command.vertexBufferBinding.offset)
						{
							currentVertexBuffer = command.vertexBufferBinding.buffer; 
							currentVertexBufferOffset = command.vertexBufferBinding.offset;
							currentVAO.SetVertex(*currentVertexBuffer, currentPipeline->settings.format.Attributes.GetArrayView(), currentPipeline->settings.format.Size(), command.vertexBufferBinding.offset, 0, 0);
						}
						break;
					}
					case Command::BindIndexBuffer:
						if (currentIndexBuffer != command.vertexBufferBinding.buffer)
						{
							currentIndexBuffer = command.vertexBufferBinding.buffer;
							currentVAO.SetIndex(*currentIndexBuffer);
						}
						currentIndexBufferOffset = command.vertexBufferBinding.offset;
						break;
					case Command::BindPipeline:
					{
						if (currentPipeline != command.pipelineData.pipeline)
						{
							currentPipeline = command.pipelineData.pipeline;
							auto & pipelineSettings = command.pipelineData.pipeline->settings;
							if (pipelineSettings.program.Handle != currentFixedFuncState.program.Handle)
								pipelineSettings.program.Use();
							if (pipelineSettings.primitiveRestart != currentFixedFuncState.primitiveRestart)
							{
								if (pipelineSettings.primitiveRestart)
								{
									glEnable(GL_PRIMITIVE_RESTART);
									glPrimitiveRestartIndex(0xFFFFFFFF);
								}
								else
								{
									glDisable(GL_PRIMITIVE_RESTART);
								}
							}
							if (pipelineSettings.primitiveType != currentFixedFuncState.primitiveType)
							{
								if (pipelineSettings.primitiveType == PrimitiveType::Patches)
									glPatchParameteri(GL_PATCH_VERTICES, pipelineSettings.patchSize);
							}
							if (pipelineSettings.enablePolygonOffset != currentFixedFuncState.enablePolygonOffset)
							{
								if (pipelineSettings.enablePolygonOffset)
								{
									glEnable(GL_POLYGON_OFFSET_FILL);
									glPolygonOffset(pipelineSettings.polygonOffsetFactor, pipelineSettings.polygonOffsetUnits);
								}
								else
									glDisable(GL_POLYGON_OFFSET_FILL);
							}
							if (pipelineSettings.DepthCompareFunc != currentFixedFuncState.DepthCompareFunc)
								SetZTestMode(pipelineSettings.DepthCompareFunc);
							if (pipelineSettings.BlendMode != currentFixedFuncState.BlendMode)
								SetBlendMode(pipelineSettings.BlendMode);
							if (pipelineSettings.StencilCompareFunc != currentFixedFuncState.StencilCompareFunc ||
								pipelineSettings.StencilDepthFailOp != currentFixedFuncState.StencilDepthFailOp ||
								pipelineSettings.StencilDepthPassOp != currentFixedFuncState.StencilDepthPassOp ||
								pipelineSettings.StencilFailOp != currentFixedFuncState.StencilFailOp ||
								pipelineSettings.StencilMask != currentFixedFuncState.StencilMask ||
								pipelineSettings.StencilReference != currentFixedFuncState.StencilReference)
							{
								StencilMode smode;
								smode.DepthFail = pipelineSettings.StencilDepthFailOp;
								smode.DepthPass = pipelineSettings.StencilDepthPassOp;
								smode.Fail = pipelineSettings.StencilFailOp;
								smode.StencilFunc = pipelineSettings.StencilCompareFunc;
								smode.StencilMask = pipelineSettings.StencilMask;
								smode.StencilReference = pipelineSettings.StencilReference;
								SetStencilMode(smode);
							}
							if (pipelineSettings.CullMode != currentFixedFuncState.CullMode)
								SetCullMode(pipelineSettings.CullMode);

							currentFixedFuncState = pipelineSettings;
							primType = pipelineSettings.primitiveType;
						}
						break;
					}
					case Command::BindDescriptorSet:
					{
						boundDescSets[command.bindDesc.location] = command.bindDesc.descSet;
						break;
					}
					case Command::Draw:
						updateBindings();
						glDrawArrays((GLenum)primType, command.draw.first, command.draw.count);
						break;
					case Command::DrawInstanced:
						updateBindings();
						glDrawArraysInstanced((GLenum)primType, command.draw.first, command.draw.count, command.draw.instances);
						break;
					case Command::DrawIndexed:
						updateBindings();
						glDrawElements((GLenum)primType, command.draw.count, GL_UNSIGNED_INT, (void*)(CoreLib::PtrInt)(command.draw.first * 4 + currentIndexBufferOffset));
						break;
					case Command::DrawIndexedInstanced:
						updateBindings();
						glDrawElementsInstanced((GLenum)primType, command.draw.count, GL_UNSIGNED_INT, (void*)(CoreLib::PtrInt)(command.draw.first * 4 + currentIndexBufferOffset), command.draw.instances);
						break;
					case Command::Blit:
						Blit(command.blit.dst, command.blit.src);
						setupFrameBuffer();
						SetWriteFrameBuffer(srcFrameBuffer);
						break;
					case Command::ClearAttachments:
						// TODO: ignoring drawBufferMask for now, assuming clearing all current framebuffer bindings
						Clear(command.clear.depth, command.clear.drawBufferMask!=0, command.clear.stencil);
						break;
					}
				}
			}
			if (fence)
			{
				auto pfence = (GLL::Fence*)fence;
				if (pfence->handle)
					glDeleteSync(pfence->handle);
				pfence->handle = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			}
			int location = 0;
			for (location = 0; location < fb->attachments.Count(); location++)
			{
				srcFrameBuffer.SetColorRenderTarget(location, Texture2D());
			}
			srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
		}

		void Blit(GameEngine::Texture2D* dstImage, GameEngine::Texture2D* srcImage)
		{
			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				dstFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(dstImage));
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				dstFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(dstImage));
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(srcImage));
				dstFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(dstImage));
				srcFrameBuffer.EnableRenderTargets(1);
				dstFrameBuffer.EnableRenderTargets(1);
				break;
			}

			// Blit from src to dst
			SetReadFrameBuffer(srcFrameBuffer);
			SetWriteFrameBuffer(dstFrameBuffer);
			CopyFrameBuffer(0, 0, width, height, 0, 0, width, height, true, false, false);

			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				dstFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				dstFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, Texture2D());
				dstFrameBuffer.SetColorRenderTarget(0, Texture2D());
				srcFrameBuffer.EnableRenderTargets(0);
				dstFrameBuffer.EnableRenderTargets(0);
				break;
			}
		}

		void Present(GameEngine::Texture2D* srcImage)
		{
			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(srcImage));
				srcFrameBuffer.EnableRenderTargets(1);
				break;
			}

			// Present rendered image to screen
			SetReadFrameBuffer(srcFrameBuffer);
			SetWriteFrameBuffer(GLL::FrameBuffer());
			CopyFrameBuffer(0, 0, width, height, 0, 0, width, height, true, false, false);

			SwapBuffers();

			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, Texture2D());
				srcFrameBuffer.EnableRenderTargets(0);
				break;
			}
		}

		void Destroy()
		{
			if (hrc)
			{
				glFinish();
				wglMakeCurrent(hdc, 0); // Remove the rendering context from our device context
				wglDeleteContext(hrc); // Delete our rendering context
			}
			if (hdc)
				ReleaseDC(hwnd, hdc); // Release the device context from our window
		}


		void SetClearColor(const Vec4 & color)
		{
			glClearColor(color.x, color.y, color.z, color.w);
		}
		void Clear(bool depth, bool color, bool stencil)
		{
			GLbitfield bitmask = 0;
			if (depth) bitmask |= GL_DEPTH_BUFFER_BIT;
			if (color) bitmask |= GL_COLOR_BUFFER_BIT;
			if (stencil) bitmask |= GL_STENCIL_BUFFER_BIT;
			glClear(bitmask);
		}


		void BindBuffer(BufferType bufferType, int index, GLuint buffer)
		{
			auto & state = bufferBindState[(int)bufferType][index];
			if (state.buffer != buffer ||
				state.start != 0 || state.length != -1)
			{
				glBindBufferBase(TranslateBufferType(bufferType), index, buffer);
				state.buffer = buffer;
				state.start = 0;
				state.length = -1;
			}
		}
		void BindBuffer(BufferType bufferType, int index, GLuint buffer, int start, int count)
		{
			if (count > 0)
			{
				auto & state = bufferBindState[(int)bufferType][index];
				if (state.buffer != buffer ||
					state.start != start || state.length != count)
				{
					glBindBufferRange(TranslateBufferType(bufferType), index, buffer, start, count);
					state.buffer = buffer;
					state.start = start;
					state.length = count;
				}
			}
		}
		void BindBufferAddr(BufferType bufferType, int index, uint64_t addr, int length)
		{
			switch (bufferType)
			{
			case BufferType::ArrayBuffer:
				glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, index, addr, length);
				break;
			case BufferType::UniformBuffer:
				glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, index, addr, length);
				break;
			case BufferType::ElementBuffer:
				glBufferAddressRangeNV(GL_ELEMENT_ARRAY_ADDRESS_NV, index, addr, length);
				break;
			default:
				throw NotImplementedException();
			}
		}
		void ExecuteComputeShader(int numGroupsX, int numGroupsY, int numGroupsZ)
		{
			glDispatchCompute((GLuint)numGroupsX, (GLuint)numGroupsY, (GLuint)numGroupsZ);
		}
		void SetViewport(int x, int y, int pwidth, int pheight)
		{
			glViewport(x, y, pwidth, pheight);
		}
		void SetCullMode(CullMode cullMode)
		{
			switch (cullMode)
			{
			case CullMode::Disabled:
				glDisable(GL_CULL_FACE);
				break;
			case CullMode::CullBackFace:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
			case CullMode::CullFrontFace:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			}
		}
		void SetDepthMask(bool write)
		{
			glDepthMask(write?GL_TRUE:GL_FALSE);
		}
		void SetColorMask(bool r, bool g, bool b, bool a)
		{
			glColorMask((GLboolean)r, (GLboolean)g, (GLboolean)b, (GLboolean)a);
		}

		void SetBlendMode(BlendMode blendMode)
		{
			switch (blendMode)
			{
			case BlendMode::Replace:
				glDisable(GL_BLEND);
				break;
			case BlendMode::AlphaBlend:
				glEnable(GL_BLEND);
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
				break;
			case BlendMode::Add:
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				break;
			default:
				throw HardwareRendererException("Unsupported blend mode.");
			}
		}
		void SetZTestMode(CompareFunc ztestMode)
		{
			switch (ztestMode)
			{
			case CompareFunc::Less:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				break;
			case CompareFunc::Equal:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_EQUAL);
				break;
			case CompareFunc::Disabled:
				glDisable(GL_DEPTH_TEST);
				break;
			case CompareFunc::LessEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
				break;
			case CompareFunc::Greater:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GREATER);
				break;
			case CompareFunc::GreaterEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GEQUAL);
				break;
			case CompareFunc::NotEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_NOTEQUAL);
				break;
			case CompareFunc::Always:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_ALWAYS);
				break;
			case CompareFunc::Never:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_NEVER);
				break;
			}
		}
		int TranslateStencilOp(StencilOp op)
		{
			switch (op)
			{
			case StencilOp::Keep:
				return GL_KEEP;
			case StencilOp::Replace:
				return GL_REPLACE;
			case StencilOp::Increment:
				return GL_INCR;
			case StencilOp::IncrementWrap:
				return GL_INCR_WRAP;
			case StencilOp::Decrement:
				return GL_DECR;
			case StencilOp::DecrementWrap:
				return GL_DECR_WRAP;
			case StencilOp::Invert:
				return GL_INVERT;
			}
			return GL_KEEP;
		}
		void SetStencilMode(StencilMode stencilMode)
		{
			switch (stencilMode.StencilFunc)
			{
			case CompareFunc::Disabled:
				glDisable(GL_STENCIL_TEST);
				break;
			case CompareFunc::Always:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_ALWAYS, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::Less:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_LESS, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::Equal:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_EQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::LessEqual:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_LEQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::Greater:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_GREATER, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::GreaterEqual:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_GEQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::NotEqual:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_NOTEQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			}
			if (stencilMode.StencilFunc != CompareFunc::Disabled)
				glStencilMask(stencilMode.StencilMask);
			glStencilOp(TranslateStencilOp(stencilMode.Fail), TranslateStencilOp(stencilMode.DepthFail), TranslateStencilOp(stencilMode.DepthPass));
		}
		void SwapBuffers()
		{
			DebugErrorEnabled = false;
			::SwapBuffers(hdc);
			DebugErrorEnabled = true;
		}

		void Flush()
		{
			glFlush();
		}

		void Finish()
		{
			glFinish();
		}

		RenderBuffer CreateRenderBuffer(StorageFormat format, int pwidth, int pheight, int samples)
		{
			auto rs = RenderBuffer();
			if (glCreateRenderbuffers)
				glCreateRenderbuffers(1, &rs.Handle);
			else
			{
				glGenRenderbuffers(1, &rs.Handle);
				glBindRenderbuffer(GL_RENDERBUFFER, rs.Handle);
			}
			rs.storageFormat = format;
			rs.internalFormat = TranslateStorageFormat(format);
			if (samples <= 1)
				glNamedRenderbufferStorageEXT(rs.Handle, rs.internalFormat, pwidth, pheight);
			else
				glNamedRenderbufferStorageMultisampleEXT(rs.Handle, samples, rs.internalFormat, pwidth, pheight);
			return rs;
		}

		FrameBuffer CreateFrameBuffer()
		{	
			GLuint handle = 0;
			if (glCreateFramebuffers)
				glCreateFramebuffers(1, &handle);
			else
			{
				glGenFramebuffers(1, &handle);
				glBindFramebuffer(GL_FRAMEBUFFER, handle);
			}
			auto rs = FrameBuffer();
			rs.Handle = handle;
			return rs;
		}

		TransformFeedback CreateTransformFeedback()
		{
			TransformFeedback rs;
			if (glCreateTransformFeedbacks)
				glCreateTransformFeedbacks(1, &rs.Handle);
			else
			{
				glGenTransformFeedbacks(1, &rs.Handle);
				glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, rs.Handle);
			}

			return rs;
		}

		Shader* CreateShader(ShaderType type, const char* data, int size)
		{
			GLuint handle = 0;
			switch (type)
			{
			case ShaderType::VertexShader:
				handle = glCreateShader(GL_VERTEX_SHADER);
				break;
			case ShaderType::FragmentShader:
				handle = glCreateShader(GL_FRAGMENT_SHADER);
				break;
			case ShaderType::HullShader:
				handle = glCreateShader(GL_TESS_CONTROL_SHADER);
				break;
			case ShaderType::DomainShader:
				handle = glCreateShader(GL_TESS_EVALUATION_SHADER);
				break;
			case ShaderType::ComputeShader:
				handle = glCreateShader(GL_COMPUTE_SHADER);
				break;
			default:
				throw HardwareRendererException("OpenGL hardware renderer does not support specified shader type.");
			}
			GLchar * src = (GLchar*)data;
			GLint length = size;
			glShaderSource(handle, 1, &src, &length);
			glCompileShader(handle);
			glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);
			List<char> buffer;
			buffer.SetSize(length);
			glGetShaderInfoLog(handle, length, &length, buffer.Buffer());
			int compileStatus;
			glGetShaderiv(handle, GL_COMPILE_STATUS, &compileStatus);
			if (compileStatus != GL_TRUE)
			{
				CoreLib::Diagnostics::Debug::WriteLine(String(buffer.Buffer()));
				throw HardwareRendererException("Shader compilation failed\n" + String(buffer.Buffer()));
			}
			auto rs = new Shader();
			rs->Handle = handle;
			rs->stage = type;
			return rs;
		}

		virtual void BeginDataTransfer() override
		{
#ifdef _DEBUG
			//if (isInDataTransfer)
			//	throw HardwareRendererException("Renderer is already in data transfer mode.");
#endif
			isInDataTransfer = true;
		}

		virtual void EndDataTransfer() override
		{
			isInDataTransfer = false;
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		}

		Program CreateTransformFeedbackProgram(const Shader &vertexShader, const List<String> & varyings, FeedbackStorageMode format)
		{
			auto handle = glCreateProgram();
			if (vertexShader.Handle)
				glAttachShader(handle, vertexShader.Handle);

			List<const char*> varyingPtrs;
			varyingPtrs.Reserve(varyings.Count());
			for (auto & v : varyings)
				varyingPtrs.Add(v.Buffer());
			glTransformFeedbackVaryings(handle, varyingPtrs.Count(), varyingPtrs.Buffer(), format == FeedbackStorageMode::Interleaved ? GL_INTERLEAVED_ATTRIBS : GL_SEPARATE_ATTRIBS);
			auto rs = Program();
			rs.Handle = handle;
			rs.Link();
			return rs;
		}

		Program CreateProgram(const Shader &computeProgram)
		{
			auto handle = glCreateProgram();
			if (computeProgram.Handle)
				glAttachShader(handle, computeProgram.Handle);
			auto rs = Program();
			rs.Handle = handle;
			rs.Link();
			return rs;
		}

		Program CreateProgram(const Shader &vertexShader, const Shader &fragmentShader)
		{
			auto handle = glCreateProgram();
			if (vertexShader.Handle)
				glAttachShader(handle, vertexShader.Handle);
			if (fragmentShader.Handle)
				glAttachShader(handle, fragmentShader.Handle);
			auto rs = Program();
			rs.Handle = handle;
			rs.Link();
			return rs;
		}

		Program CreateProgram(const Shader &vertexShader, const Shader &fragmentShader, EnumerableDictionary<String, int> & vertexAttributeBindings)
		{
			auto handle = glCreateProgram();
			if (vertexShader.Handle)
				glAttachShader(handle, vertexShader.Handle);
			if (fragmentShader.Handle)
				glAttachShader(handle, fragmentShader.Handle);
			auto rs = Program();
			rs.Handle = handle;
			for (auto & binding : vertexAttributeBindings)
			{
				glBindAttribLocation(handle, binding.Value, binding.Key.Buffer());
			}
			rs.Link();
			return rs;
		}

		TextureSampler* CreateTextureSampler()
		{
			auto rs = new TextureSampler();
			glGenSamplers(1, &rs->Handle);
			return rs;
		}

		virtual Texture2D* CreateTexture2D(int w, int h, StorageFormat storageFormat, DataType dataType, void* data) override
		{
			if (storageFormat == StorageFormat::BC1 || storageFormat == StorageFormat::BC5)
				throw HardwareRendererException("Cannot automatically generate mipmaps for compressed textures");

			int mipLevelCount = (int)Math::Log2Floor(Math::Max(w, h)) + 1;
			GLint internalformat = TranslateStorageFormat(storageFormat);
			GLenum format = TranslateDataTypeToFormat(dataType);
			GLenum type = TranslateDataTypeToInputType(dataType);

			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_2D, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_2D, handle);
			}
			glBindTexture(GL_TEXTURE_2D, handle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			glTextureStorage2D(handle, mipLevelCount, internalformat, w, h);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, type, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);

			auto rs = new Texture2D();
			rs->Handle = handle;
			rs->storageFormat = storageFormat;
			rs->BindTarget = GL_TEXTURE_2D;
			return rs;
		}

		virtual Texture2D* CreateTexture2D(TextureUsage /*usage*/, int w, int h, int mipLevelCount, StorageFormat format) override
		{
			if (format == StorageFormat::BC1 || format == StorageFormat::BC5)
				throw HardwareRendererException("Compressed textures must supply data at texture creation");

			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_2D, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_2D, handle);
			}
			glBindTexture(GL_TEXTURE_2D, handle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			glTextureStorage2D(handle, mipLevelCount, TranslateStorageFormat(format), w, h);
			glBindTexture(GL_TEXTURE_2D, 0);

			auto rs = new Texture2D();
			rs->Handle = handle;
			rs->storageFormat = format;
			rs->BindTarget = GL_TEXTURE_2D;
			return rs;
		}

		virtual Texture2D* CreateTexture2D(TextureUsage /*usage*/, int w, int h, int mipLevelCount, StorageFormat storageFormat, DataType dataType, ArrayView<void*> mipLevelData) override
		{
			GLint internalformat = TranslateStorageFormat(storageFormat);
			GLenum format = TranslateDataTypeToFormat(dataType);
			GLenum type = TranslateDataTypeToInputType(dataType);

			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_2D, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_2D, handle);
			}
			glBindTexture(GL_TEXTURE_2D, handle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			if (storageFormat == StorageFormat::BC1 || storageFormat == StorageFormat::BC5 || storageFormat == StorageFormat::BC3)
			{
				for (int level = 0; level < mipLevelCount; level++)
				{
					int wl = Math::Max(w >> level, 1);
					int hl = Math::Max(h >> level, 1);
					int blocks = (int)(ceil(wl / 4.0f) * ceil(hl / 4.0f));
					int bufferSize = storageFormat == StorageFormat::BC1 ? blocks * 8 : blocks * 16;
					glCompressedTexImage2D(GL_TEXTURE_2D, level, internalformat, wl, hl, 0, bufferSize, mipLevelData[level]);
				}
			}
			else
			{
				glTextureStorage2D(handle, mipLevelCount, internalformat, w, h);
				for (int level = 0; level < mipLevelCount; level++)
					glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, Math::Max(w >> level, 1), Math::Max(h >> level, 1), format, type, mipLevelData[level]);
			}
			glBindTexture(GL_TEXTURE_2D, 0);

			auto rs = new Texture2D();
			rs->Handle = handle;
			rs->storageFormat = storageFormat;
			rs->BindTarget = GL_TEXTURE_2D;
			return rs;
		}

		virtual Texture3D* CreateTexture3D(TextureUsage /*usage*/, int w, int h, int depth, int mipLevelCount, StorageFormat format) override
		{
			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_3D, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_3D, handle);
			}
			glBindTexture(GL_TEXTURE_3D, handle);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			glTextureStorage3D(handle, mipLevelCount, TranslateStorageFormat(format), w, h, depth);
			glBindTexture(GL_TEXTURE_3D, 0);

			auto rs = new Texture3D();
			rs->Handle = handle;
			rs->storageFormat = format;
			rs->BindTarget = GL_TEXTURE_3D;
			return rs;
		}

		virtual Texture2DArray* CreateTexture2DArray(TextureUsage /*usage*/, int w, int h, int layers, int mipLevelCount, StorageFormat format) override
		{
			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
			}
			glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			glTextureStorage3D(handle, mipLevelCount, TranslateStorageFormat(format), w, h, layers);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			auto rs = new Texture2DArray();
			rs->Handle = handle;
			rs->storageFormat = format;
			rs->BindTarget = GL_TEXTURE_2D_ARRAY;
			return rs;
		}


		virtual TextureCube* CreateTextureCube(TextureUsage /*usage*/, int size, int mipLevelCount, StorageFormat format) override
		{
			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
			}
			glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipLevelCount, TranslateStorageFormat(format), size, size);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

			auto rs = new TextureCube(size);
			rs->Handle = handle;
			rs->storageFormat = format;
			rs->BindTarget = GL_TEXTURE_CUBE_MAP;
			return rs;
		}

		VertexArray CreateVertexArray()
		{
			auto rs = VertexArray();
			if (glCreateVertexArrays)
				glCreateVertexArrays(1, &rs.Handle);
			else
			{
				glGenVertexArrays(1, &rs.Handle);
				glBindVertexArray(rs.Handle);
			}
			return rs;
		}

		void BindVertexArray(VertexArray vertArray)
		{
			glBindVertexArray(vertArray.Handle);
		}

		void DrawElements(PrimitiveType primType, int count, DataType indexType)
		{
			glDrawElements((GLenum)primType, count, TranslateDataTypeToInputType(indexType), nullptr);
		}

		void DrawRangeElements(PrimitiveType primType, int lowIndex, int highIndex, int startLoc, int count, DataType indexType)
		{
			int indexSize = 4;
			if (indexType == DataType::UShort)
				indexSize = 2;
			else if (indexType == DataType::Byte)
				indexSize = 1;
			glDrawRangeElements((GLenum)primType, lowIndex, highIndex, count, TranslateDataTypeToInputType(indexType), (void*)(CoreLib::PtrInt)(startLoc * indexSize));
		}

		void DrawArray(PrimitiveType primType, int first, int count)
		{
			glDrawArrays((GLenum)primType, first, count);
		}

		void DrawArrayInstances(PrimitiveType primType, int vertCount, int numInstances)
		{
			glDrawArraysInstanced((GLenum)primType, 0,  vertCount, numInstances);
		}

		void DrawInstances(DataType type, PrimitiveType primType, int numInstances, int indexCount)
		{
			GLenum gl_type;
			switch (GetSingularDataType(type))
			{
			case DataType::Int:
				gl_type = GL_UNSIGNED_INT;
				break;
			case DataType::Short:
			case DataType::UShort:
				gl_type = GL_UNSIGNED_SHORT;
				break;
			case DataType::Byte:
				gl_type = GL_UNSIGNED_BYTE;
				break;
			default:
				gl_type = GL_UNSIGNED_INT;
				break;
			}
			glDrawElementsInstanced((GLenum)primType, indexCount, gl_type, 0, numInstances);
		}

		void DestroyBuffer(BufferObject & obj)
		{
			glDeleteBuffers(1, &obj.Handle);
			obj.Handle = 0;
		}

		void DestroyFrameBuffer(FrameBuffer & obj)
		{
			glDeleteFramebuffers(1, &obj.Handle);
			obj.Handle = 0;
		}

		void DestroyRenderBuffer(RenderBuffer & obj)
		{
			glDeleteRenderbuffers(1, &obj.Handle);
			obj.Handle = 0;

		}

		void DestroyShader(Shader & obj)
		{
			glDeleteShader(obj.Handle);
			obj.Handle = 0;

		}

		void DestroyProgram(Program & obj)
		{
			glDeleteProgram(obj.Handle); 
			obj.Handle = 0;
		}

		void DestroyTransformFeedback(TransformFeedback & obj)
		{
			glDeleteTransformFeedbacks(1, &obj.Handle);
			obj.Handle = 0;
		}

		void DestroyTexture(Texture & obj)
		{
			glDeleteTextures(1, &obj.Handle);
			obj.Handle = 0;

		}

		void DestroyVertexArray(VertexArray & obj)
		{
			glDeleteVertexArrays(1, &obj.Handle);
			obj.Handle = 0;

		}

		void DestroyTextureSampler(TextureSampler & obj)
		{
			glDeleteSamplers(1, &obj.Handle);
			obj.Handle = 0;

		}

		BufferObject* CreateBuffer(BufferUsage usage, int bufferSize, int storageFlags)
		{
			auto rs = new BufferObject();
#ifdef _DEBUG
			rs->isInTransfer = &isInDataTransfer;
#endif
			if (storageFlags & GL_MAP_PERSISTENT_BIT)
				rs->persistentMapping = true;

			rs->BindTarget = TranslateBufferUsage(usage);
			if (glCreateBuffers)
				glCreateBuffers(1, &rs->Handle);
			else
			{
				glGenBuffers(1, &rs->Handle);
				glBindBuffer(rs->BindTarget, rs->Handle);
			}
			rs->BufferStorage(bufferSize, nullptr, storageFlags);
			return rs;
		}

		BufferObject* CreateBuffer(BufferUsage usage, int bufferSize)
		{
			return CreateBuffer(usage, bufferSize, GL_DYNAMIC_STORAGE_BIT);
		}
		BufferObject* CreateMappedBuffer(BufferUsage usage, int bufferSize)
		{
			auto result = CreateBuffer(usage, bufferSize, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT);
			return result;
		}

		void SetReadFrameBuffer(const FrameBuffer &buffer)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer.Handle);
		}

		void SetWriteFrameBuffer(const FrameBuffer &buffer)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer.Handle);
		}

		void CopyFrameBuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, bool color, bool depth, bool stencil)
		{
			GLbitfield bitmask = 0;
			if (depth) bitmask |= GL_DEPTH_BUFFER_BIT;
			if (color) bitmask |= GL_COLOR_BUFFER_BIT;
			if (stencil) bitmask |= GL_STENCIL_BUFFER_BIT;
			glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, bitmask, GL_LINEAR);
		}
		void UseTexture(int channel, const Texture &tex)
		{
			if (textureBindState[channel] != tex.Handle)
			{
				glActiveTexture(GL_TEXTURE0 + channel);
				glBindTexture(tex.BindTarget, tex.Handle);
				textureBindState[channel] = tex.Handle;
			}
		}
		void UseSampler(int channel, const TextureSampler &sampler)
		{
			if (samplerBindState[channel] != sampler.Handle)
			{
				glActiveTexture(GL_TEXTURE0 + channel);
				glBindSampler(channel, sampler.Handle);
				samplerBindState[channel] = sampler.Handle;
			}
		}
		int UniformBufferAlignment() override
		{
			GLint uniformBufferAlignment;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignment);
			return uniformBufferAlignment;
		}

		int StorageBufferAlignment() override
		{
			GLint rs;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &rs);
			return rs;
		}

		RenderTargetLayout* CreateRenderTargetLayout(CoreLib::ArrayView<AttachmentLayout> bindings)
		{
			return new RenderTargetLayout(bindings);
		}

		PipelineBuilder* CreatePipelineBuilder()
		{
			return new PipelineBuilder();
		}

		GameEngine::CommandBuffer* CreateCommandBuffer() 
		{
			return new CommandBuffer();
		}

		virtual GameEngine::DescriptorSetLayout * CreateDescriptorSetLayout(CoreLib::ArrayView<GameEngine::DescriptorLayout> descriptors) override
		{
			auto result = new DescriptorSetLayout();
			for (auto & desc : descriptors)
			{
				BindingLayout layout;
				layout.BindingPoints = desc.LegacyBindingPoints;
				layout.Type = desc.Type;
				result->layouts.Add(layout);
			}
			return result;
		}

		virtual GameEngine::DescriptorSet * CreateDescriptorSet(GameEngine::DescriptorSetLayout * playout) override
		{
			auto layout = (DescriptorSetLayout*)playout;
			DescriptorSet * result = new DescriptorSet();
			result->descriptors.SetSize(layout->layouts.Count());
			for (int i = 0; i < layout->layouts.Count(); i++)
			{
				result->descriptors[i].Type = layout->layouts[i].Type;
			}
			return result;
		}
		
		virtual int GetDescriptorPoolCount() override
		{
			return 0;
		}

		void Wait()
		{
			glFinish();
		}
	};
}

namespace GameEngine
{
	HardwareRenderer * CreateGLHardwareRenderer()
	{
		return new GLL::HardwareRenderer();
	}
}