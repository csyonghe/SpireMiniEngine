#include "AseFile.h"
#include "../LibIO.h"
#include "../Tokenizer.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		void SkipBlock(TokenReader & parser)
		{
			while (!parser.IsEnd() && !parser.LookAhead("}"))
			{
				auto word = parser.ReadToken();
				if (word.Content == "{")
					SkipBlock(parser);
			}
			parser.Read("}");
		}

		void SkipField(TokenReader & parser)
		{
			while (!parser.IsEnd() && !parser.LookAhead("*") && !parser.LookAhead("}"))
			{
				auto word = parser.ReadToken();
				if (word.Content == "{")
					SkipBlock(parser);
			}
		}

		void ParseAttributes(EnumerableDictionary<String, AseValue> & attribs, TokenReader & parser)
		{
			parser.Read("*");
			auto fieldName = parser.ReadWord();
			if (parser.NextToken().Type == TokenType::StringLiterial)
			{
				attribs[fieldName] = AseValue(parser.ReadStringLiteral());
			}
			else
			{
				Vec4 val;
				val.SetZero();
				int ptr = 0;
				while (!parser.LookAhead("*") && !parser.LookAhead("}"))
				{
					if (ptr < 4)
					{
						if (parser.NextToken().Type == TokenType::DoubleLiterial || parser.NextToken().Type == TokenType::IntLiterial)
						{
							val[ptr] = (float)parser.ReadDouble();
							ptr++;
						}
						else if (parser.LookAhead("{"))
						{
							parser.ReadToken();
							SkipBlock(parser);
						}
						else
							SkipField(parser);
					}
					else
						throw InvalidOperationException("Invalid file format: attribute contains more than 4 values.");
				}
				attribs[fieldName] = AseValue(val);
			}
		}

		void ParseAttributeBlock(EnumerableDictionary<String, AseValue> & attribs, TokenReader & parser)
		{
			parser.Read("{");
			while (!parser.LookAhead("}"))
			{
				if (parser.LookAhead("*"))
					ParseAttributes(attribs, parser);
				else
					break;
			}
			parser.Read("}");
		}

		RefPtr<AseMaterial> ParseSubMaterial(TokenReader & parser)
		{
			RefPtr<AseMaterial> result = new AseMaterial();
			ParseAttributeBlock(result->Fields, parser);
			AseValue val;
			val.Str = "noname";
			result->Fields.TryGetValue("MATERIAL_NAME", val);
			result->Name = val.Str;
			return result;
		}

		RefPtr<AseMaterial> ParseMaterial(TokenReader & parser)
		{
			RefPtr<AseMaterial> result = new AseMaterial();
			parser.Read("{");
			while (!parser.LookAhead("}"))
			{
				parser.Read("*");
				auto fieldName = parser.ReadWord();
				if (fieldName == "SUBMATERIAL")
				{
					parser.ReadInt(); // id
					result->SubMaterials.Add(ParseSubMaterial(parser));
				}
				else if (fieldName == "MATERIAL_NAME")
				{
					result->Name = parser.ReadStringLiteral();
				}
				else
				{
					parser.Back(2);
					ParseAttributes(result->Fields, parser);
				}
			}
			parser.Read("}");
			return result;
		}


		void ParseVertexList(List<Vec3> & verts, TokenReader & parser)
		{
			parser.Read("{");
			while (!parser.LookAhead("}"))
			{
				parser.Read("*");
				parser.ReadToken(); // name
				parser.ReadToken(); // id
				Vec3 c;
				c.x = (float)parser.ReadDouble();
				c.y = (float)parser.ReadDouble();
				c.z = (float)parser.ReadDouble();
				verts.Add(c);
			}
			parser.Read("}");
		}

		void ParseFaceList(List<AseMeshFace> & faces, TokenReader & parser)
		{
			parser.Read("{");
			while (!parser.LookAhead("}"))
			{
				AseMeshFace f;
				parser.Read("*");
				parser.Read("MESH_FACE"); // name
				parser.ReadToken(); // id
				parser.Read(":");
				parser.Read("A"); parser.Read(":");
				f.Ids[0] = parser.ReadInt();
				parser.Read("B"); parser.Read(":");
				f.Ids[1] = parser.ReadInt();
				parser.Read("C"); parser.Read(":");
				f.Ids[2] = parser.ReadInt();
				while (!parser.LookAhead("*") && !parser.LookAhead("}"))
					parser.ReadToken();
				while (parser.LookAhead("*"))
				{
					parser.Read("*");
					auto word = parser.ReadWord();
					if (word == "MESH_SMOOTHING")
					{
						try
						{
							// comma separated int list
							f.SmoothGroup = 0;
							while (parser.NextToken().Type == TokenType::IntLiterial)
							{
								f.SmoothGroup |= (1 << parser.ReadInt());
								if (parser.LookAhead(","))
									parser.ReadToken();
								else
									break;
							}
						}
						catch (Exception)
						{
							f.SmoothGroup = 0;
							parser.Back(1);
						}
					}
					else if (word == "MESH_MTLID")
						f.MaterialId = parser.ReadInt();
					else if (word == "MESH_FACE")
					{
						parser.Back(2);
						break;
					}
				}
				faces.Add(f);
			}
			parser.Read("}");
		}

		void ParseAttribFaceList(List<AseMeshAttribFace> & faces, TokenReader & parser)
		{
			parser.Read("{");
			while (!parser.LookAhead("}"))
			{
				AseMeshAttribFace f;
				parser.Read("*");
				parser.ReadToken(); // name
				parser.ReadToken(); // id
				f.Ids[0] = parser.ReadInt();
				f.Ids[1] = parser.ReadInt();
				f.Ids[2] = parser.ReadInt();
				faces.Add(f);
			}
			parser.Read("}");
		}

		RefPtr<AseMesh> ParseMesh(TokenReader & parser)
		{
			RefPtr<AseMesh> result = new AseMesh();
			parser.Read("{");
			while (!parser.LookAhead("}"))
			{
				parser.Read("*");
				auto fieldName = parser.ReadWord();
				if (fieldName == "MESH_NUMVERTEX")
				{
					int count = parser.ReadInt();
					result->Vertices.Reserve(count);
				}
				else if (fieldName == "MESH_NUMFACES")
				{
					int count = parser.ReadInt();
					result->Faces.Reserve(count);
				}
				else if (fieldName == "MESH_VERTEX_LIST")
				{
					ParseVertexList(result->Vertices, parser);
				}
				else if (fieldName == "MESH_FACE_LIST")
				{
					ParseFaceList(result->Faces, parser);
				}
				else if (fieldName == "MESH_NUMCVERTEX")
				{
					result->Colors.Data.Reserve(parser.ReadInt());
				}
				else if (fieldName == "MESH_NUMCVFACES")
				{
					result->Colors.Faces.Reserve(parser.ReadInt());
				}
				else if (fieldName == "MESH_CVERTLIST")
				{
					ParseVertexList(result->Colors.Data, parser);
				}
				else if (fieldName == "MESH_CFACELIST")
				{
					ParseAttribFaceList(result->Colors.Faces, parser);
				}
				else if (fieldName == "MESH_TVERTLIST")
				{
					result->TexCoords.SetSize(Math::Max(result->TexCoords.Count(), 1));
					ParseVertexList(result->TexCoords[0].Data, parser);
				}
				else if (fieldName == "MESH_TFACELIST")
				{
					result->TexCoords.SetSize(Math::Max(result->TexCoords.Count(), 1));
					result->TexCoords[0].Faces.Reserve(result->Faces.Count());
					ParseAttribFaceList(result->TexCoords[0].Faces, parser);
				}
				else if (fieldName == "MESH_NUMTVFACES")
				{
					result->TexCoords.SetSize(Math::Max(result->TexCoords.Count(), 1));
					result->TexCoords[0].Faces.Reserve(parser.ReadInt());
				}
				else if (fieldName == "MESH_MAPPINGCHANNEL")
				{
					int channelId = parser.ReadInt();
					result->TexCoords.SetSize(Math::Max(channelId, result->TexCoords.Count()));
					auto & texCoords = result->TexCoords[channelId - 1];
					parser.Read("{");
					while (!parser.LookAhead("}"))
					{
						parser.Read("*");
						auto word = parser.ReadWord();
						if (word == "MESH_NUMTVERTEX")
						{
							texCoords.Data.Reserve(parser.ReadInt());
						}
						else if (word == "MESH_NUMTVFACES")
						{
							texCoords.Faces.Reserve(parser.ReadInt());
						}
						else if (word == "MESH_TVERTLIST")
						{
							ParseVertexList(texCoords.Data, parser);
						}
						else if (word == "MESH_TFACELIST")
						{
							ParseAttribFaceList(texCoords.Faces, parser);
						}
						else
							SkipField(parser);
					}
					parser.Read("}");
				}
				else
				{
					SkipField(parser);
				}
			}
			parser.Read("}");
			return result;
		}

		RefPtr<AseGeomObject> ParseGeomObject(TokenReader & parser)
		{
			RefPtr<AseGeomObject> result = new AseGeomObject();
			parser.Read("{");
			while (!parser.LookAhead("}"))
			{
				parser.Read("*");
				auto fieldName = parser.ReadWord();
				if (fieldName == "NODE_NAME")
				{
					result->Attributes[fieldName] = AseValue(parser.ReadStringLiteral());
				}
				else if (fieldName == "NODE_TM")
				{
					ParseAttributeBlock(result->Attributes, parser);
				}
				else if (fieldName == "MESH")
				{
					result->Mesh = ParseMesh(parser);
				}
				else if (fieldName == "MATERIAL_REF")
				{
					result->MaterialId = parser.ReadInt();
				}
				else
					SkipField(parser);
			}
			parser.Read("}");
			return result;
		}

		void AseFile::Parse(const String & content, bool flipYZ)
		{
			TokenReader parser(content);
			while (!parser.IsEnd())
			{
				parser.Read("*");
				auto fieldName = parser.ReadToken().Content;
				if (fieldName == "MATERIAL_LIST")
				{
					parser.Read("{");
					while (!parser.LookAhead("}"))
					{
						parser.Read("*");
						auto subField = parser.ReadWord();
						if (subField == "MATERIAL_COUNT")
						{
							int count = parser.ReadInt();
							Materials.SetSize(count);
						}
						else if (subField == "MATERIAL")
						{
							int id = parser.ReadInt();
							if (id >= Materials.Count())
								Materials.SetSize(id + 1);
							Materials[id] = ParseMaterial(parser);
						}
						else
							SkipField(parser);
					}
					parser.Read("}");
				}
				else if (fieldName == "GEOMOBJECT")
				{
					GeomObjects.Add(ParseGeomObject(parser));
				}
				else
					SkipField(parser);
			}
			if (flipYZ)
			{
				for (auto & obj : GeomObjects)
				{
					for (auto & vert : obj->Mesh->Vertices)
					{
						auto tmp = vert.z;
						vert.z = -vert.y;
						vert.y = tmp;
					}
				}
			}
		}

		void AseFile::LoadFromFile(const String & fileName, bool flipYZ)
		{
			Parse(File::ReadAllText(fileName), flipYZ);
		}

		void AseMesh::ConstructPerVertexFaceList(Basic::List<int> & faceCountAtVert, Basic::List<int> & vertFaceList) const
		{
			faceCountAtVert.SetSize(Vertices.Count());
			for (int i = 0; i < faceCountAtVert.Count(); i++)
				faceCountAtVert[i] = 0;
			for (int i = 0; i<Faces.Count(); i++)
			{
				faceCountAtVert[Faces[i].Ids[0]]++;
				faceCountAtVert[Faces[i].Ids[1]]++;
				faceCountAtVert[Faces[i].Ids[2]]++;
			}
			int scan = 0;
			for (int i = 0; i<faceCountAtVert.Count(); i++)
			{
				int s = faceCountAtVert[i];
				faceCountAtVert[i] = scan;
				scan += s;
			}
			vertFaceList.SetSize(scan);
			for (int i = 0; i<Faces.Count(); i++)
			{
				vertFaceList[faceCountAtVert[Faces[i].Ids[0]]++] = i;
				vertFaceList[faceCountAtVert[Faces[i].Ids[1]]++] = i;
				vertFaceList[faceCountAtVert[Faces[i].Ids[2]]++] = i;
			}
		}

		void AseMesh::RecomputeNormals()
		{
			Normals.Data.Clear();
			Normals.Faces.SetSize(Faces.Count());
			Dictionary<Vec3, int> normalMap;
			List<Vec3> faceNormals;
			faceNormals.SetSize(Faces.Count());
			for (int i = 0; i<Faces.Count(); i++)
			{
				Vec3 v1 = Vertices[Faces[i].Ids[0]];
				Vec3 v2 = Vertices[Faces[i].Ids[1]];
				Vec3 v3 = Vertices[Faces[i].Ids[2]];
				Vec3 ab, ac;
				Vec3::Subtract(ab, v2, v1);
				Vec3::Subtract(ac, v3, v1);
				Vec3 n;
				Vec3::Cross(n, ab, ac);
				float len = n.Length();
				if (len > 1e-6f)
					Vec3::Scale(n, n, 1.0f / len);
				else
					n = Vec3::Create(1.0f, 0.0f, 0.0f);
				if (n.Length() > 1.2f || n.Length() < 0.5f)
					n = Vec3::Create(1.0f, 0.0f, 0.0f);
				faceNormals[i] = n;
			}
			List<int> vertShare;
			List<int> vertFaces;
			ConstructPerVertexFaceList(vertShare, vertFaces);
			int start = 0;
			for (int i = 0; i<Faces.Count(); i++)
			{
				auto & face = Faces[i];
				auto & nface = Normals.Faces[i];
				for (int j = 0; j < 3; j++)
				{
					int vid = face.Ids[j];
					if (vid == -1)
						continue;
					if (vid == 0)
						start = 0;
					else
						start = vertShare[vid - 1];
					int count = 0;
					Vec3 n;
					n.SetZero();
					for (int k = start; k < vertShare[vid]; k++)
					{
						int fid = vertFaces[k];
						if (Faces[fid].SmoothGroup & face.SmoothGroup)
						{
							Vec3::Add(n, faceNormals[fid], n);
							count++;
						}
					}
					if (count == 0)
						n = faceNormals[i];
					else
					{
						Vec3::Scale(n, n, 1.0f / count);
						if (n.Length() < 0.5f)
						{
							n = faceNormals[i];
						}
						else
							n = n.Normalize();
					}
					if (!normalMap.TryGetValue(n, nface.Ids[j]))
					{
						Normals.Data.Add(n);
						nface.Ids[j] = Normals.Data.Count() - 1;
						normalMap[n] = Normals.Data.Count()-1;
					}
				}
			}
		}
	}
}