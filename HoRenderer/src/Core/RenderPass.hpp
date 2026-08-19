/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include "Util.hpp"
#include "Shader.hpp"

class RenderPass {
public:
	unsigned int fbo = 0;
	unsigned int vao = 0;
	unsigned int vbo = 0;
	std::vector<unsigned int> colorAttachments;
	Shader m_shader;
	int width = 0;
	int height = 0;

public:
	~RenderPass();
	bool BindData(bool finalPass = false);
	bool ShaderConfig(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
	void Clean();
	void Draw(const std::vector<unsigned int>& texPassArray);
};
