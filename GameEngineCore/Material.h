#ifndef GAME_ENGINE_MATERIAL_H
#define GAME_ENGINE_MATERIAL_H

#include "CoreLib/Basic.h"
#include "DynamicVariable.h"
#include "RenderContext.h"

namespace GameEngine
{
	class Level;

	class Material
	{
	public:
		CoreLib::String Name;
		CoreLib::String ShaderFile;
		bool ParameterDirty = true;
		CoreLib::RefPtr<ModuleInstance> MaterialPatternModule, MaterialGeometryModule;
		CoreLib::EnumerableDictionary<CoreLib::String, DynamicVariable> Variables;
		void SetVariable(CoreLib::String name, DynamicVariable value);
		void Parse(CoreLib::Text::TokenReader & parser);
		void LoadFromFile(const CoreLib::String & fullFileName);

		template<typename WriteTextureFunc, typename WriteFunc, typename AlignFunc>
		void FillInstanceUniformBuffer(const WriteTextureFunc & writeTex, const WriteFunc & write, const AlignFunc & align)
		{
			for (auto & mvar : Variables)
			{
				if (mvar.Value.VarType == DynamicVariableType::Texture)
				{
					writeTex(mvar.Value.StringValue);
				}
				else if (mvar.Value.VarType == DynamicVariableType::Float)
					write(mvar.Value.FloatValue);
				else if (mvar.Value.VarType == DynamicVariableType::Int)
					write(mvar.Value.IntValue);
				else if (mvar.Value.VarType == DynamicVariableType::Vec2)
				{
					align(8);
					write(mvar.Value.Vec2Value);
				}
				else if (mvar.Value.VarType == DynamicVariableType::Vec3)
				{
					align(16);
					write(mvar.Value.Vec3Value);
				}
				else if (mvar.Value.VarType == DynamicVariableType::Vec4)
				{
					align(16);
					write(mvar.Value.Vec4Value);
				}
			}
		}
	};
}

#endif