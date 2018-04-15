#include "TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

int
TransformSystem::Create()
{
	assert(nextIndex < MAX_NUM_TRANSFORMS);
	int transformID = nextIndex++;
	return transformID;
}

Transform&
TransformSystem::Get(int transformID)
{
	Transform& transform = transforms[transformID];
	return transform;
}

void
TransformSystem::UpdateMatrices(int transformID)
{
	const Transform& old = oldTransforms[transformID];
	Transform& curr = transforms[transformID];

	static const glm::mat4 identity{ 1.0f };

	if (old.position != curr.position || old.orientation != curr.orientation || old.scale != curr.scale)
	{
		auto scale       = glm::scale(identity, curr.scale);
		auto translation = glm::translate(identity, curr.position);
		auto rotation    = glm::toMat4(glm::normalize(curr.orientation));

		curr.matrix = translation * rotation * scale;
		curr.inverseMatrix = glm::inverse(curr.matrix);
		curr.normalMatrix = glm::transpose(glm::inverse(glm::mat3{ curr.matrix }));
	}
}
