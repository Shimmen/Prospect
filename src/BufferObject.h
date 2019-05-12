#pragma once

#include <glad/glad.h>

enum class BufferObjectType
{
	Uniform = GL_UNIFORM_BUFFER,
	ShaderStorage = GL_SHADER_STORAGE_BUFFER,
	AtomicCounter = GL_ATOMIC_COUNTER_BUFFER,
	TransformFeedback = GL_TRANSFORM_FEEDBACK_BUFFER
};

template<typename T>
class BufferObject
{
public:

	BufferObject() = default;

	BufferObject(const BufferObject& other) = delete;
	BufferObject& operator=(const BufferObject& other) = delete;

	void BindBufferBase(BufferObjectType type, GLuint index);
	void UpdateGpuBuffer();

	T memory{};

private:

	void CreateBufferIfRequired();

	T gpuMemory{};
	GLuint bufferHandle = 0;

};

template <typename T>
void BufferObject<T>::CreateBufferIfRequired()
{
	if (!bufferHandle)
	{
		glCreateBuffers(1, &bufferHandle);
		glNamedBufferStorage(bufferHandle, sizeof(T), nullptr, GL_DYNAMIC_STORAGE_BIT);
	}
}


template <typename T>
void BufferObject<T>::BindBufferBase(BufferObjectType type, GLuint index)
{
	CreateBufferIfRequired();
	GLenum target = static_cast<GLenum>(type);
	glBindBufferBase(target, index, bufferHandle);
}

template <typename T>
void BufferObject<T>::UpdateGpuBuffer()
{
	CreateBufferIfRequired();
	if (memcmp(&memory, &gpuMemory, sizeof(T)) != 0)
	{
		gpuMemory = memory;
		glNamedBufferSubData(bufferHandle, 0, sizeof(T), &gpuMemory);
	}
}
