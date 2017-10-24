#include "Engine.h"
#include "Level.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"
#include "MeshBuilder.h"
#include "CameraActor.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::IO;
    using namespace VectorMath;

	String IndentText(String src)
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

	Level::Level(const CoreLib::String & fileName)
	{
		LoadFromText(File::ReadAllText(fileName));
		FileName = fileName;
	}
	void Level::LoadFromText(CoreLib::String text)
	{
		Text::TokenReader parser(text);
		auto errorRecover = [&]() 
		{
			while (!parser.IsEnd())
			{
				if (Engine::Instance()->IsRegisteredActorClass(parser.NextToken().Content))
					break;
				parser.ReadToken();
			}
		};
		while (!parser.IsEnd())
		{
			auto pos = parser.NextToken().Position;
			if (parser.LookAhead("hidden"))
			{
				parser.ReadToken();
				Text::CodePosition beginPos = parser.NextToken().Position;
				beginPos.Pos -= parser.NextToken().Content.Length();
				parser.ReadToken();
				parser.Read("{");
				Text::CodePosition endPos;
				int braceLevel = 1;
				while (braceLevel > 0)
				{
					if (parser.LookAhead("}"))
					{
						braceLevel--;
						endPos = parser.ReadToken().Position;
					}
					else if (parser.LookAhead("{"))
					{
						braceLevel++;
						parser.ReadToken();
					}
					else
						parser.ReadToken();
				}
				HiddenSections.Add(text.SubString(beginPos.Pos, endPos.Pos + 1 - beginPos.Pos));
				continue;
			}
            auto actorClass = parser.NextToken().Content;
			auto actor = Engine::Instance()->ParseActor(this, parser);
			if (!actor)
			{
                if (!Engine::Instance()->IsRegisteredActorClass(actorClass))
                    Print("Unknown actor class '%S' at line %d. Do you forget to register the actor class?\n", actorClass.ToWString(), pos.Line);
                else
				    Print("Error parsing object at line %d, ignoring the object.\n", pos.Line);
				errorRecover();
			}
			else
			{
				try
				{
					if (Actors.ContainsKey(actor->Name.GetValue()))
					{
						Print("error: an actor named '%S' already exists, ignoring second definition at line %d.\n",
                            actor->Name.GetValue().ToWString(), pos.Line);
						errorRecover();
					}
					else
					{
						RegisterActor(actor.Ptr());
						if (actor->GetEngineType() == EngineActorType::Camera)
							CurrentCamera = actor.As<CameraActor>();
					}
				}
				catch (Exception e)
				{
					Print("OnLoad() error: an actor named '%S' failed to load, message: '%S'.\n", actor->Name.GetValue().ToWString(), e.Message.ToWString());
					errorRecover();
				}
			}
		}
		Print("Num materials: %d\n", Materials.Count());
	}
	void Level::SaveToFile(CoreLib::String fileName)
	{
		StringBuilder sb;
		for (auto & actor : Actors)
			actor.Value->SerializeToText(sb);
		for (auto & sect : HiddenSections)
			sb << "hidden " << sect << "\n";
		File::WriteAllText(fileName, IndentText(sb.ProduceString()));
		FileName = fileName;
	}
	Level::~Level()
	{
		for (auto & actor : Actors)
			UnregisterActor(actor.Value.Ptr());
        Materials = decltype(Materials)();
        Models = decltype(Models)();
        Meshes = decltype(Meshes)();
        Skeletons = decltype(Skeletons)();
        Animations = decltype(Animations)();
        RetargetFiles = decltype(RetargetFiles)();
        Actors = decltype(Actors)();

	}
	void Level::RegisterActor(Actor * actor)
	{
		Actors.Add(actor->Name.GetValue(), actor);
		actor->OnLoad();
		actor->RegisterUI(Engine::Instance()->GetUiEntry());
	}
	void Level::UnregisterActor(Actor*actor)
	{
		actor->OnUnload();
        auto actorName = actor->Name.GetValue();
		Actors[actorName] = nullptr;
		Actors.Remove(actorName);
	}
	Mesh * Level::LoadMesh(CoreLib::String fileName)
	{
		RefPtr<Mesh> result = nullptr;
		if (!Meshes.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			if (actualName.Length())
			{
				result = new Mesh();
				result->LoadFromFile(actualName);
				Meshes[fileName] = result;
			}
			else
			{
				Print("error: cannot load mesh \'%S\'\n", fileName.ToWString());
				return nullptr;
			}
		}
		return result.Ptr();
	}
	Mesh * Level::LoadMesh(CoreLib::String name, Mesh m)
	{
		RefPtr<Mesh> result = nullptr;
		if (!Meshes.TryGetValue(name, result))
		{
			result = new Mesh(_Move(m));
			Meshes.Add(_Move(name), result);
		}
		return result.Ptr();
	}
	Mesh * Level::LoadErrorMesh()
	{
		MeshBuilder mb;
		mb.PushRotation(Vec3::Create(0.0f, 0.0f, 1.0f), Math::Pi * 0.25f);
		mb.AddBox(Vec3::Create(-100.0f, -30.0f, -30.0f), Vec3::Create(100.0f, 30.0f, 30.0f));
		mb.PopTransform();
		mb.PushRotation(Vec3::Create(0.0f, 0.0f, 1.0f), -Math::Pi * 0.25f);
		mb.AddBox(Vec3::Create(-100.0f, -30.0f, -30.0f), Vec3::Create(100.0f, 30.0f, 30.0f));
		return LoadMesh("ErrorMesh", mb.ToMesh());
	}
	Model * Level::LoadModel(CoreLib::String fileName)
	{
		RefPtr<Model> result = nullptr;
		if (!Models.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			if (actualName.Length())
			{
				result = new Model();
				result->LoadFromFile(this, actualName);
				Models[fileName] = result;
			}
			else
			{
				Print("error: cannot load model \'%S\'\n", fileName.ToWString());
				return nullptr;
			}
		}
		return result.Ptr();
	}
	Model * Level::LoadErrorModel()
	{
		if (errorModel)
			return errorModel.Ptr();
		errorModel = new Model(LoadErrorMesh(), LoadErrorMaterial());
		return errorModel.Ptr();
	}
	Skeleton * Level::LoadSkeleton(const CoreLib::String & fileName)
	{
		RefPtr<Skeleton> result = nullptr;
		if (!Skeletons.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			if (actualName.Length())
			{
				result = new Skeleton();
				result->LoadFromFile(actualName);
				Skeletons[fileName] = result;
			}
			else
			{
				Print("error: cannot load skeleton \'%S\'\n", fileName.ToWString());
				return nullptr;
			}
		}
		return result.Ptr();
	}
	RetargetFile * Level::LoadRetargetFile(const CoreLib::String & fileName)
	{
		auto result = RetargetFiles.TryGetValue(fileName);
		if (!result)
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			if (actualName.Length())
			{
				RetargetFile file;
				file.LoadFromFile(actualName);
				RetargetFiles[fileName] = _Move(file);
			}
			else
				return nullptr;
		}
		return RetargetFiles.TryGetValue(fileName);
	}
	Material * Level::LoadMaterial(const CoreLib::String & fileName)
	{
		RefPtr<Material> result = nullptr;
		if (!Materials.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Material);
			if (actualName.Length())
			{
				result = new Material();
				result->LoadFromFile(actualName);
				Materials[fileName] = result;
			}
			else if (fileName != "Error.material")
			{
				Print("error: cannot load material \'%S\'\n", fileName.ToWString());
				return LoadMaterial("Error.material");
			}
			else
				return nullptr;
		}
		return result.Ptr();
	}
	Material * Level::LoadErrorMaterial()
	{
		return LoadMaterial("Error.material");
	}
	Material * Level::CreateNewMaterial()
	{
		Material* mat = new Material();
		Materials[String("$materialInstance") + String(Materials.Count())] = mat;
		return mat;
	}
	SkeletalAnimation * Level::LoadSkeletalAnimation(const CoreLib::String & fileName)
	{
		RefPtr<SkeletalAnimation> result = nullptr;
		if (!Animations.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			if (actualName.Length())
			{
				result = new SkeletalAnimation();
				result->LoadFromFile(actualName);
				Animations[fileName] = result;
			}
			else
			{
				Print("error: cannot load animation \'%S\'\n", fileName.ToWString());
				return nullptr;
			}
		}
		return result.Ptr();
	}
	Actor * Level::FindActor(const CoreLib::String & name)
	{
		RefPtr<Actor> result;
		Actors.TryGetValue(name, result);
		return result.Ptr();
	}
}
