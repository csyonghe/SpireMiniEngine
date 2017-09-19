#include "EnvMapActor.h"
#include "RenderContext.h"
#include "Engine.h"

namespace GameEngine
{
	void EnvMapActor::OnLoad()
	{
		envMapId = Engine::Instance()->GetRenderer()->GetSharedResource()->AllocEnvMap();
	}
	bool EnvMapActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("TintColor"))
		{
			parser.ReadToken();
			TintColor = ParseVec3(parser);
			return true;
		}
		if (parser.LookAhead("Radius"))
		{
			parser.ReadToken();
			Radius = (float)parser.ReadDouble();
			return true;
		}
		return false;
	}
}