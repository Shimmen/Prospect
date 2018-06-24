#include "TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

//
// Data
//

static const glm::mat4 identity{ 1.0f };

static int nextIndex = 0;
static std::array<Transform, MAX_NUM_TRANSFORMS> transforms;
static std::array<Transform, MAX_NUM_TRANSFORMS> oldTransforms;

//
// Internal API
//

bool IdenticalTransformProperties(const Transform& a, const Transform& b)
{
	return a.position == b.position
		&& a.orientation == b.orientation
		&& a.scale == b.scale;
}

//
// Public API
//

void
TransformSystem::Init()
{
	// Setup a default transform (transform 0)
	int defaultTransform = Create();
	assert(defaultTransform == 0);
}

void
TransformSystem::Update()
{
	size_t numTransforms = nextIndex;
	for (size_t id = 0; id < numTransforms; ++id)
	{
		Transform& old = oldTransforms[id];
		Transform& curr = transforms[id];

		if (!IdenticalTransformProperties(old, curr))
		{
			old = curr;
		}
	}
}

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

	// Unless nothing has changed, create new matrices for the transform
	if (!IdenticalTransformProperties(old, curr))
	{
		auto scale       = glm::scale(identity, curr.scale);
		auto translation = glm::translate(identity, curr.position);
		auto rotation    = glm::toMat4(glm::normalize(curr.orientation));

		curr.matrix = translation * rotation * scale;
		curr.inverseMatrix = glm::inverse(curr.matrix);
		curr.normalMatrix = glm::transpose(glm::inverse(glm::mat3{ curr.matrix }));
	}
}
