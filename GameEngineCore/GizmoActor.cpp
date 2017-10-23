#include "GizmoActor.h"

namespace GameEngine
{
	void GizmoActor::LocalTransform_Changing(VectorMath::Matrix4 & value)
	{
		for (auto & gizmo : gizmos)
			gizmo.SetTransform(value);
	}
	void GizmoActor::SetGizmoMesh(int index, const Mesh & gizmoMesh, GizmoStyle style)
	{
		gizmos[index].SetMesh((CoreLib::StringBuilder() << GetTypeName() << "gizmo_" << index).ToString(), level, this, gizmoMesh);
		gizmos[index].SetGizmoStyle(style);
	}

	void GizmoActor::SetGizmoColor(int index, VectorMath::Vec4 color)
	{
		gizmos[index].SetColor(color);
	}

	void GizmoActor::GetDrawables(const GetDrawablesParameter & params)
	{
		for (auto & gizmo : gizmos)
			gizmo.GetDrawables(params);
	}

	void GizmoActor::OnLoad()
	{
		for (auto & gizmo : gizmos)
			gizmo.SetTransform(*LocalTransform);
		LocalTransform.OnChanging.Bind(this, &GizmoActor::LocalTransform_Changing);
	}

}
