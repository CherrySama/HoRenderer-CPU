/*
	Created by Yinghao He on 2025-05-15
*/
#include "RenderPass.hpp"

RenderPass::~RenderPass()
{
	glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    if (fbo != 0) 
        glDeleteFramebuffers(1, &fbo);
}

void RenderPass::BindData(bool finalPass)
{
    if (!finalPass) {
		glGenFramebuffers(1, &fbo);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	float quadVertices[] = {
		//positions        //texCoords
		-1.0f, -1.0f,      0.0f, 1.0f,  // Bottom left
		1.0f, -1.0f,       1.0f, 1.0f,  // Bottom right
		-1.0f,  1.0f,      0.0f, 0.0f,  // Top left
		
		1.0f, -1.0f,       1.0f, 1.0f,  // Bottom right
		1.0f,  1.0f,       1.0f, 0.0f,  // Top right
		-1.0f,  1.0f,      0.0f, 0.0f   // Top left
	};

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	if (!finalPass) {
		std::vector<unsigned int> attachments;
		for (int i = 0; i < colorAttachments.size(); i++) {
			glBindTexture(GL_TEXTURE_2D, colorAttachments[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorAttachments[i], 0);
			attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glDrawBuffers(attachments.size(), &attachments[0]);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass::ShaderConfig(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    m_shader.ShaderConfig(vertexPath, fragmentPath, geometryPath);
}

void RenderPass::Draw(const std::vector<unsigned int>& texPassArray) {
	glUseProgram(m_shader.GetID());
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	for (int i = 0; i < texPassArray.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, texPassArray[i]);
		std::string uName = "texPass" + std::to_string(i);
		m_shader.SetInt(uName.c_str(), i);
	}
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

