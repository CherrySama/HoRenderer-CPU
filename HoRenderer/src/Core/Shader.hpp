/*
	Created by Yinghao He on 2025-05-15
*/
#include "Util.hpp"

class Shader
{
public:
	Shader();
	~Shader();
	void ShaderConfig(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);

	void Use();
	void UnUse();
	unsigned int GetID() const;

	void SetBool(const std::string& name, bool value) const;
	void SetUnInt(const std::string& name, int value) const;
	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	void SetVec2(const std::string& name, const glm::vec2& value) const;
	void SetVec2(const std::string& name, float x, float y) const;
	void SetVec3(const std::string& name, const glm::vec3& value) const;
	void SetVec3(const std::string& name, float x, float y, float z) const;
	void SetMat3(const std::string& name, const glm::mat3& mat) const;
	void SetMat4(const std::string& name, const glm::mat4& mat) const;

private:
	unsigned int m_ID = 0u;

	void CheckCompileErrors(GLuint shader, std::string type);
};