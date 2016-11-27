#include "CoreLib/Basic.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/VectorMath.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace VectorMath;

String ExtractField(const String & src, const String & fieldName)
{
	int idx = src.IndexOf(fieldName);
	if (idx != -1)
	{
		int endIdx = src.IndexOf(L' ', idx);
		int endIdx2 = Math::Min(src.IndexOf(L'\n', idx), src.IndexOf(L'\r', idx));

		if (endIdx == -1 || endIdx2 != -1 && endIdx2 < endIdx)
			endIdx = endIdx2;
		return src.SubString(idx + fieldName.Length(), endIdx - idx - fieldName.Length());
	}
	return "";
}

Vec3 ParseRotation(String src)
{
	Vec3 rs;
	TokenReader p(src);
	p.Read("(");
	p.Read("Pitch");
	p.Read("=");
	rs.x = (float)p.ReadDouble();
	p.Read(",");
	p.Read("Yaw");
	p.Read("=");
	rs.y = (float)p.ReadDouble();
	p.Read(",");
	p.Read("Roll");
	p.Read("=");
	rs.z = (float)p.ReadDouble();
	return rs;
}

Vec3 ParseTranslation(String src)
{
	Vec3 rs;
	TokenReader p(src);
	p.Read("(");
	p.Read("X");
	p.Read("=");
	rs.x = (float)p.ReadDouble();
	p.Read(",");
	p.Read("Y");
	p.Read("=");
	rs.y = (float)p.ReadDouble();
	p.Read(",");
	p.Read("Z");
	p.Read("=");
	rs.z = (float)p.ReadDouble();
	return rs;
}

String IndentString(String src)
{
	StringBuilder  sb;
	int indent = 0;
	bool beginTrim = true;
	for (int c = 0; c < src.Length(); c++)
	{
		auto ch = src[c];
		if (ch == '\n')
		{
			sb << "\n";

			beginTrim = true;
		}
		else
		{
			if (beginTrim)
			{
				while (c < src.Length() - 1 && (src[c] == '\t' || src[c] == '\n' || src[c] == '\r' || src[c] == ' '))
				{
					c++;
					ch = src[c];
				}
				for (int i = 0; i < indent - 1; i++)
					sb << '\t';
				if (ch != '}' && indent > 0)
					sb << '\t';
				beginTrim = false;
			}

			if (ch == L'{')
				indent++;
			else if (ch == L'}')
				indent--;
			if (indent < 0)
				indent = 0;

			sb << ch;
		}
	}
	return sb.ProduceString();
}

int wmain(int argc, const wchar_t ** argv)
{
	if (argc <= 1)
		return 0;
	String fileName = String::FromWString(argv[1]);
	String src = File::ReadAllText(fileName);
	TokenReader parser(src);
	StringBuilder sb;
	while (!parser.IsEnd())
	{
		if (parser.ReadToken().Content == "Begin" &&
			parser.ReadToken().Content == "Actor" && 
			parser.ReadToken().Content == "Class" &&
			parser.ReadToken().Content == "=")
		{
			if (parser.ReadToken().Content == "StaticMeshActor")
			{
				auto beginPos = parser.NextToken().Position;
				while (!(parser.NextToken().Content == "End" && parser.NextToken(1).Content == "Actor"))
				{
					parser.ReadToken();
				}
				auto endToken = parser.ReadToken();
				auto endPos = endToken.Position;
				auto actorStr = src.SubString(beginPos.Pos, endPos.Pos);
				auto name = ExtractField(actorStr, "Name=");
				auto mesh = ExtractField(actorStr, "StaticMesh=");
				auto location = ExtractField(actorStr, "RelativeLocation=");
				auto rotation = ExtractField(actorStr, "RelativeRotation=");
				auto scale = ExtractField(actorStr, "RelativeScale=");
				auto material = ExtractField(actorStr, "OverrideMaterials(0)=");
				sb << "StaticMesh\n{\n";
				sb << "name \"" << name << "\"\n";
				sb << "mesh \"" << mesh.SubString(mesh.IndexOf('.') + 1, mesh.Length() - mesh.IndexOf('.') - 3) << ".mesh\"\n";
				Matrix4 transform;
				Matrix4::CreateIdentityMatrix(transform);
				if (rotation.Length())
				{
					Matrix4 rot;
					auto r = ParseRotation(rotation);
					Matrix4::Rotation(rot, r.y, r.x, r.z);
					Matrix4::Multiply(transform, rot, transform);
				}
				if (scale.Length())
				{
					Matrix4 matS;
					auto s = ParseTranslation(scale);
					Matrix4::Scale(matS, s.x, s.y, s.z);
					Matrix4::Multiply(transform, matS, transform);
				}
				if (location.Length())
				{
					Matrix4 matTrans;
					auto s = ParseTranslation(location);
					Matrix4::Translation(matTrans, s.x, s.y, s.z);
					Matrix4::Multiply(transform, matTrans, transform);
				}
				sb << "transform [";
				for (int i = 0; i < 16; i++)
					sb << transform.values[i] << " ";
				sb << "]\n";
				if (material.Length())
				{
					sb << "material \"" << material.SubString(material.IndexOf('.') + 1, material.Length() - material.IndexOf('.') - 3) << ".material\"\n";
				}
				sb << "}\n";
			}
		}
	}
	File::WriteAllText(Path::ReplaceExt(fileName, "level"), IndentString(sb.ProduceString()));
    return 0;
}

