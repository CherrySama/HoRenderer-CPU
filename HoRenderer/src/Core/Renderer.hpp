/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include "Util.hpp"
#include "RenderPass.hpp"
#include "Integrator.hpp"
#include "Denoiser.hpp"
#include "../Common/FileManager.hpp"

GLuint CreateTextureRGB32F(int w, int h);

class Renderer{
public:
    Renderer(std::unique_ptr<Camera> cam, std::unique_ptr<Integrator> it, std::unique_ptr<Sampler> sam, std::unique_ptr<Scene> sc);
    ~Renderer();

	bool WindowInit();
	bool PipelineConfiguration(FileManager *fm);
	void Run();

public:
    RenderPass pass1, pass2, pass3;
    GLuint lastFrame = 0;
    GLuint nowFrame = 0;
    float dt = 0.0f;
    float fps = 0.0f;
    unsigned int frameCounter = 0;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Integrator> integrator;
	std::unique_ptr<Sampler> sampler;
    std::unique_ptr<Scene> scene;
    Denoiser denoiser;
    bool enable_denoising = true;

private:
	void CleanupOpenGLResources();

	GLFWwindow *window = nullptr;
    int width;
    int height;
};
