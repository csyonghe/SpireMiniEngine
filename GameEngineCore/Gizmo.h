#ifndef GAME_ENGINE_GIZMO_H
#define GAME_ENGINE_GIZMO_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	class Mesh;
	struct GetDrawablesParameter;
	class Level;
	class Actor;


	enum class GizmoStyle
	{
		Normal, Editor
	};

	class Gizmo : public CoreLib::RefObject
	{
	private:
		class Impl;
		CoreLib::UniquePtr<Impl> impl;
	public:
		Gizmo();
		Gizmo(CoreLib::String gizmoName, Level * level, Actor * ownerActor, const Mesh & mesh);
		Gizmo(Gizmo && other);
		Gizmo(const Gizmo &) = delete;
		~Gizmo();
		Gizmo & operator = (const Gizmo &) = delete;
		Gizmo & operator = (Gizmo&&);
		void SetMesh(CoreLib::String gizmoName, Level * level, Actor * ownerActor, const Mesh & mesh);
		void SetTransform(const VectorMath::Matrix4 & transform);
		void GetDrawables(const GetDrawablesParameter & params);
		void SetColor(VectorMath::Vec4 color);
		void SetVisible(bool value);
		bool GetVisible();
		void SetGizmoStyle(GizmoStyle style);
		GizmoStyle GetGizmoStyle();
	};

}

#endif