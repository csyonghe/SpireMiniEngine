#ifndef GAME_ENGINE_GIZMO_ACTOR_H
#define GAME_ENGINE_GIZMO_ACTOR_H

#include "Actor.h"
#include "Gizmo.h"

namespace GameEngine
{
	class GizmoActor : public Actor
	{
	private:
		CoreLib::List<Gizmo> gizmos;
		void LocalTransform_Changing(VectorMath::Matrix4 & value);
	protected:
		void SetGizmoCount(int count)
		{
			gizmos.SetSize(count);
		}
		void SetGizmoMesh(int index, const Mesh & gizmoMesh, GizmoStyle style);
		void SetGizmoColor(int index, VectorMath::Vec4 color);
		int GetGizmoCount()
		{
			return gizmos.Count();
		}
		Gizmo & GetGizmo(int i)
		{
			return gizmos[i];
		}
	public:
		virtual void GetDrawables(const GetDrawablesParameter & params) override;
		virtual void OnLoad() override;
	};
}

#endif