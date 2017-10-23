#ifndef GAME_ENGINE_MODEL_H
#define GAME_ENGINE_MODEL_H

#include "Mesh.h"
#include "Skeleton.h"
#include "Drawable.h"
#include "Physics.h"

namespace GameEngine
{
	class Material;
	class Level;
	struct GetDrawablesParameter;

	class ModelDrawableInstance
	{
	public:
		bool isSkeletal = false;
		CoreLib::List<CoreLib::RefPtr<Drawable>> Drawables;
		bool IsEmpty()
		{
			return Drawables.Count() == 0;
		}
		void UpdateTransformUniform(VectorMath::Matrix4 localTransform);
		void UpdateTransformUniform(VectorMath::Matrix4 localTransform, Pose & pose, RetargetFile * retargetFile);
	};

	class ModelPhysicsInstance
	{
	private:
		PhysicsScene * scene = nullptr;
	public:
		bool isSkeletal = false;
		Skeleton * skeleton = nullptr;
		CoreLib::List<PhysicsObject*> objects;
		void SetTransform(VectorMath::Matrix4 localTransform);
		void SetTransform(VectorMath::Matrix4 localTransform, Pose & pose, RetargetFile * retargetFile);
        void SetChannels(PhysicsChannels channels);
		void RemoveFromScene();
		ModelPhysicsInstance(PhysicsScene * pScene)
			: scene(pScene)
		{}
		~ModelPhysicsInstance()
		{
			if (scene)
				RemoveFromScene();
		}
	};

	class Model : public CoreLib::RefObject
	{
	private:
		Mesh mesh;
		Skeleton skeleton;
		CoreLib::List<CoreLib::RefPtr<PhysicsModel>> physModels; // one physics model per bone
		CoreLib::List<Material*> materials;
		CoreLib::List<CoreLib::String> materialFileNames;
		CoreLib::String skeletonFileName, meshFileName;
		void InitPhysicsModel();
	public:
		Model() = default;
		Model(Mesh * pMesh, Material * material);
		Model(Mesh * pMesh, Skeleton * skeleton, Material * material);

		void LoadFromFile(Level * level, CoreLib::String fileName);
		void LoadFromString(Level * level, CoreLib::String content);
		void SaveToFile(CoreLib::String fileName);
		CoreLib::Graphics::BBox GetBounds()
		{
			return mesh.Bounds;
		}
		CoreLib::String ToString();
		Skeleton * GetSkeleton()
		{
			return &skeleton;
		}
		Mesh * GetMesh()
		{
			return &mesh;
		}
		ModelDrawableInstance GetDrawableInstance(const GetDrawablesParameter & params);
		CoreLib::RefPtr<ModelPhysicsInstance> CreatePhysicsInstance(PhysicsScene & physScene, Actor * actor, void * tag);
        CoreLib::RefPtr<ModelPhysicsInstance> CreatePhysicsInstance(PhysicsScene & physScene, Actor * actor, void * tag, PhysicsChannels channels);

	};
}

#endif