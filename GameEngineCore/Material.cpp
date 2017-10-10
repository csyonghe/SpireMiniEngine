#include "Material.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	Material::Material()
	{
		static int idAlloc = 0;
		Id = idAlloc;
		idAlloc++;
	}

	void Material::SetVariable(CoreLib::String name, DynamicVariable value)
	{
		ParameterDirty = true;
		if (auto oldValue = Variables.TryGetValue(name))
		{
			if (oldValue->VarType == value.VarType)
				*oldValue = value;
			else
				throw CoreLib::InvalidOperationException("type mismatch.");
		}
		else
			throw CoreLib::InvalidOperationException("variable not found");
	}

	void Material::Parse(CoreLib::Text::TokenReader & parser)
	{
		parser.Read("material");
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			if (parser.LookAhead("shader"))
			{
				parser.ReadToken();
				ShaderFile = parser.ReadStringLiteral();
			}
			else if (parser.LookAhead("var"))
			{
				parser.ReadToken();
				auto name = parser.ReadWord();
				parser.Read("=");
				auto val = DynamicVariable::Parse(parser);
				Variables[name] = val;
			}
			else
				throw CoreLib::Text::TextFormatException("invalid material syntax");
		}
		parser.Read("}");
	}

	void Material::Serialize(CoreLib::StringBuilder & sb)
	{
		sb << "material\n{\n";
		sb << "shader " << CoreLib::Text::EscapeStringLiteral(ShaderFile) << "\n";
		for (auto & var : Variables)
		{
			sb << "var " << var.Key << " = ";
			var.Value.Serialize(sb);
			sb << "\n";
		}
		sb << "}\n";
	}

	void Material::LoadFromFile(const CoreLib::String & fullFileName)
	{
		CoreLib::Text::TokenReader parser(CoreLib::IO::File::ReadAllText(fullFileName));
		Name = CoreLib::IO::Path::GetFileName(fullFileName);
		Parse(parser);
	}
}

