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
		int Id = 0;
		bool ParameterDirty = true;
		bool IsTransparent = false;
		bool IsDoubleSided = false;
		ModuleInstance MaterialPatternModule, MaterialGeometryModule;
		CoreLib::EnumerableDictionary<CoreLib::String, DynamicVariable> Variables;
		CoreLib::List<DynamicVariable*> PatternVariables, GeometryVariables;
		void SetVariable(CoreLib::String name, DynamicVariable value);
		void Parse(CoreLib::Text::TokenReader & parser);
		void LoadFromFile(const CoreLib::String & fullFileName);
		Material();
		template<typename WriteTextureFunc, typename WriteFunc, typename AlignFunc>
		void FillInstanceUniformBuffer(ModuleInstance * module, const WriteTextureFunc & writeTex, const WriteFunc & write, const AlignFunc & align)
		{
			auto& vars = module == &MaterialPatternModule ? PatternVariables : GeometryVariables;
			for (auto & mvar : vars)
			{
				if (mvar->VarType == DynamicVariableType::Texture)
				{
					writeTex(mvar->StringValue);
				}
				else if (mvar->VarType == DynamicVariableType::Float)
					write(mvar->FloatValue);
				else if (mvar->VarType == DynamicVariableType::Int)
					write(mvar->IntValue);
				else if (mvar->VarType == DynamicVariableType::Vec2)
				{
					align(8);
					write(mvar->Vec2Value);
				}
				else if (mvar->VarType == DynamicVariableType::Vec3)
				{
					align(16);
					write(mvar->Vec3Value);
				}
				else if (mvar->VarType == DynamicVariableType::Vec4)
				{
					align(16);
					write(mvar->Vec4Value);
				}
			}
		}
	};
}

#endif