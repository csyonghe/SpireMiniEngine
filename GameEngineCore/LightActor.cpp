#include "LightActor.h"
#include "Engine.h"
#include "Material.h"

using namespace VectorMath;
using namespace CoreLib;

namespace GameEngine
{
	void LightActor::LocalTransform_Changing(VectorMath::Matrix4 & value)
	{
		localTransformChanged = true;
		if (model)
			CoreLib::Graphics::TransformBBox(Bounds, value, model->GetBounds());
		if (physInstance)
			physInstance->SetTransform(value);
	}
	Vec3 LightActor::GetDirection()
	{
		auto localTransform = LocalTransform.GetValue();
		return Vec3::Create(localTransform.m[1][0], localTransform.m[1][1], localTransform.m[1][2]).Normalize();
	}
	bool LightActor::ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser)
	{
		if (Actor::ParseField(fieldName, parser))
			return true;
		if (fieldName.ToLower() == "direction")
		{
			auto dir = ParseVec3(parser).Normalize();
			Vec3 x;
			GetOrthoVec(x, dir);
			Vec3 z;
			z = Vec3::Cross(dir, x);
			Matrix4 localTrans;
			localTrans = LocalTransform.GetValue();
			localTrans.m[0][0] = x.x;
			localTrans.m[0][1] = x.y;
			localTrans.m[0][2] = x.z;
			localTrans.m[1][0] = dir.x;
			localTrans.m[1][1] = dir.y;
			localTrans.m[1][2] = dir.z;
			localTrans.m[2][0] = z.x;
			localTrans.m[2][1] = z.y;
			localTrans.m[2][2] = z.z;
			LocalTransform = localTrans;
			return true;
		}
		return false;
	}
	void LightActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (params.IsEditorMode)
		{
			auto insertDrawable = [&](Drawable * d)
			{
				d->CastShadow = false;
				d->Bounds = Bounds;
				params.sink->AddDrawable(d);
			};
			if (model)
			{
				GetDrawablesParameter newParams = params;
				newParams.UseSkeleton = false;
				if (modelInstance.IsEmpty())
					modelInstance = model->GetDrawableInstance(newParams);
				if (localTransformChanged)
				{
					modelInstance.UpdateTransformUniform(*LocalTransform);
					localTransformChanged = false;
				}
				for (auto &d : modelInstance.Drawables)
					insertDrawable(d.Ptr());
			}
		}
	}
	void LightActor::OnLoad()
	{
		if (Engine::Instance()->GetEditor())
		{
			auto gizmoMesh = level->LoadMesh(GetTypeName() + "_gizmo", CreateGizmoMesh());
			gizmoMaterial = *(level->LoadMaterial("Gizmo.material"));
			gizmoMaterial.SetVariable("solidColor", DynamicVariable(Vec3::Create(0.3f, 0.3f, 1.0f)));
			model = new GameEngine::Model(gizmoMesh, &gizmoMaterial);
			LocalTransform.OnChanging.Bind(this, &LightActor::LocalTransform_Changing);
			SetLocalTransform(*LocalTransform);
			physInstance = model->CreatePhysicsInstance(level->GetPhysicsScene(), this, nullptr);
			physInstance->SetTransform(*LocalTransform);
			modelInstance.Drawables.Clear();
		}
	}
}

