#ifndef GAME_ENGINE_MODEL_H
#define GAME_ENGINE_MODEL_H

#include "Mesh.h"
#include "Skeleton.h"
#include "Drawable.h"

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

	class Model : public CoreLib::RefObject
	{
	private:
		Mesh mesh;
		Skeleton skeleton;
		CoreLib::List<Material*> materials;
		CoreLib::List<CoreLib::String> materialFileNames;
		CoreLib::String skeletonFileName, meshFileName;
	public:
		Model() = default;
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
		
	};
}

#endif