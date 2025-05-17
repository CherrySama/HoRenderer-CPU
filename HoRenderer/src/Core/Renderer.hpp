/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include "Util.hpp"
#include "RenderPass.hpp"

GLuint GetTextureRGB32F(int width, int height);

class Renderer{
public:
	void WindowInit();
	void SceneConfig();
	void PipelineConfiguration();
	void Run();

public:
	RenderPass pass1;

private:
	GLFWwindow *window;
	bool isfullscreen = false;
	int max_samples = 8;
    int width = 1200;
    int height = 900;
};