/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include "Util.hpp"
#include "RenderPass.hpp"
#include "Integrator.hpp"
#include "../Common/FileManager.hpp"

GLuint GetTextureRGB32F(int width, int height, const Integrator &integrator);

class Renderer{
public:
    Renderer(std::unique_ptr<Camera> cam, std::unique_ptr<Integrator> it, std::unique_ptr<Sampler> sam, std::unique_ptr<Scene> sc);
	void WindowInit();
	void PipelineConfiguration(FileManager *fm);
	void Run();

public:
	RenderPass pass1;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Integrator> integrator;
	std::unique_ptr<Sampler> sampler;
	std::unique_ptr<Scene> scene;

private:
	GLFWwindow *window;
	bool isfullscreen = false;
	int max_samples = 8;
    int width = 1600;
    int height = 900;
};