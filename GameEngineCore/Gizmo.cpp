#include "Gizmo.h"
#include "Model.h"
#include "Material.h"
#include "Level.h"
#include "RendererService.h"
#include "Engine.h"

using namespace VectorMath;

namespace GameEngine
{
	class Gizmo::Impl
	{
	private:
		CoreLib::RefPtr<Model> model = nullptr;
		CoreLib::RefPtr<Drawable> drawable;
		CoreLib::RefPtr<ModelPhysicsInstance> physInstance;
		ModelDrawableInstance modelInstance;
		Material gizmoMaterial;
		CoreLib::Graphics::BBox bounds;
		VectorMath::Matrix4 localTransform;
		bool transformUpdated = false;
		bool visible = true;
		GizmoStyle gizmoStyle = GizmoStyle::Normal;
	public:
		Impl()
		{
			VectorMath::Matrix4::CreateIdentityMatrix(localTransform);
		}
		Impl(CoreLib::String gizmoName, Level * level, Actor * ownerActor, const Mesh & mesh)
		{
			VectorMath::Matrix4::CreateIdentityMatrix(localTransform);
			SetMesh(gizmoName, level, ownerActor, mesh);
		}
		void SetMesh(CoreLib::String gizmoName, Level * level, Actor * ownerActor, const Mesh & mesh)
		{
			auto gizmoMesh = level->LoadMesh(gizmoName, mesh);
			gizmoMaterial = *(level->LoadMaterial("Gizmo.material"));
			gizmoMaterial.SetVariable("solidColor", DynamicVariable(Vec3::Create(0.3f, 0.3f, 1.0f)));
			model = new GameEngine::Model(gizmoMesh, &gizmoMaterial);
            physInstance = model->CreatePhysicsInstance(level->GetPhysicsScene(), ownerActor, nullptr, PhysicsChannels::Visiblity);
			modelInstance.Drawables.Clear();
			SetTransform(localTransform);
		}
		void SetTransform(const VectorMath::Matrix4 & transform)
		{
			localTransform = transform;
			CoreLib::Graphics::TransformBBox(bounds, transform, model->GetBounds());
            physInstance->SetTransform(transform);
			transformUpdated = true;
		}
		void SetColor(VectorMath::Vec4 color)
		{
			gizmoMaterial.SetVariable("solidColor", color.xyz());
			gizmoMaterial.SetVariable("alpha", color.w);
		}
		void GetDrawables(const GetDrawablesParameter & params)
		{
			if (visible && model)
			{
				if (gizmoStyle == GizmoStyle::Editor && !params.IsEditorMode)
					return;
				if (modelInstance.IsEmpty())
					modelInstance = model->GetDrawableInstance(params);
				if (transformUpdated)
				{
					modelInstance.UpdateTransformUniform(localTransform);
					transformUpdated = false;
				}
				auto insertDrawable = [&](Drawable * d)
				{
					d->CastShadow = false;
					d->Bounds = bounds;
					params.sink->AddDrawable(d);
				};
				for (auto &d : modelInstance.Drawables)
					insertDrawable(d.Ptr());
			}
		}
		void SetVisible(bool val)
		{
			visible = val;
            if (visible)
                physInstance->SetChannels(PhysicsChannels::Visiblity);
            else
                physInstance->SetChannels(PhysicsChannels::None);

		}
		bool GetVisible()
		{
			return visible;
		}
		void SetGizmoStyle(GizmoStyle style)
		{
			gizmoStyle = style;
		}
		GizmoStyle GetGizmoStyle()
		{
			return gizmoStyle;
		}
	};

	Gizmo::Gizmo()
	{
		impl = new Gizmo::Impl();
	}
	Gizmo::Gizmo(CoreLib::String gizmoName, Level * level, Actor * ownerActor, const Mesh & mesh)
	{
		impl = new Gizmo::Impl(gizmoName, level, ownerActor, mesh);
	}

	Gizmo::Gizmo(Gizmo && other) = default;

	Gizmo::~Gizmo() = default;

	Gizmo & Gizmo::operator=(Gizmo &&) = default;

	void Gizmo::SetMesh(CoreLib::String gizmoName, Level * level, Actor * ownerActor, const Mesh & mesh)
	{
		impl->SetMesh(gizmoName, level, ownerActor, mesh);
	}

	void Gizmo::SetTransform(const VectorMath::Matrix4 & transform)
	{
		impl->SetTransform(transform);
	}

	void Gizmo::GetDrawables(const GetDrawablesParameter & params)
	{
		impl->GetDrawables(params);
	}

	void Gizmo::SetColor(VectorMath::Vec4 color)
	{
		impl->SetColor(color);
	}

	void Gizmo::SetVisible(bool value)
	{
		impl->SetVisible(value);
	}

	bool Gizmo::GetVisible()
	{
		return impl->GetVisible();
	}

	void Gizmo::SetGizmoStyle(GizmoStyle style)
	{
		impl->SetGizmoStyle(style);
	}

	GizmoStyle Gizmo::GetGizmoStyle()
	{
		return impl->GetGizmoStyle();
	}

}

