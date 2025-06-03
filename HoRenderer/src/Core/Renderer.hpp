/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include "Util.hpp"
#include "RenderPass.hpp"
#include "Integrator.hpp"
#include "../Common/FileManager.hpp"

GLuint CreateTextureRGB32F(int w, int h);

class Renderer{
public:
    Renderer(std::unique_ptr<Camera> cam, std::unique_ptr<Integrator> it, std::unique_ptr<Sampler> sam, std::unique_ptr<Scene> sc);
    ~Renderer();

    void WindowInit();
	void PipelineConfiguration(FileManager *fm);
	void Run();

public:
    RenderPass pass1, pass2, pass3;
    GLuint lastFrame, nowFrame;
    clock_t t1, t2;
	float dt, fps;
    unsigned int frameCounter;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Integrator> integrator;
	std::unique_ptr<Sampler> sampler;
	std::unique_ptr<Scene> scene;

private:
	GLFWwindow *window;
    int width;
    int height;
};