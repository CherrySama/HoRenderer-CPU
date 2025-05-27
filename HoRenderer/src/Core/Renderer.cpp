/*
    Created by Yinghao He on 2025-05-15
*/
#include "Renderer.hpp"


Renderer::Renderer(std::unique_ptr<Camera> cam, std::unique_ptr<Integrator> it, std::unique_ptr<Sampler> sam, std::unique_ptr<Scene> sc)
{
    camera = std::move(cam);
    integrator = std::move(it);
    sampler = std::move(sam);
    scene = std::move(sc);

    frameCounter = 0;
    width = camera->image_width;
    height = camera->image_height;
    WindowInit();
	PipelineConfiguration(FileManager::getInstance());
}

void Renderer::WindowInit()
{
	// glfw: initialize and configure
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW..." << std::endl;
		return;
	}
    
    // OpenGL setting
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_REFRESH_RATE, 60);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // the size of window will be unresizable
    
	// window setting 
    window = glfwCreateWindow(width, height, "HoRenderer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
		return;
	}
    glfwMakeContextCurrent(window);
    
    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
		return;
	}
}

void Renderer::PipelineConfiguration(FileManager *fm)
{
	if (fm == nullptr)
	{
		std::cout << "fm is null ptr" << std::endl;
		return;
	}
    fm->init();
    // pass1: Mix current frame and history frames
	pass1.width = width;
	pass1.height = height;
    pass1.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                       fm->getShaderPath("MixFrameShader.frag").c_str());
    pass1.colorAttachments.push_back(CreateTextureRGB32F(width, height));
    pass1.BindData();
    // pass2: Save history frames 
    pass2.width = width;
    pass2.height = height;
    pass2.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                       fm->getShaderPath("LastFrameShader.frag").c_str());
    lastFrame = CreateTextureRGB32F(width, height);
    pass2.colorAttachments.push_back(lastFrame);
    pass2.BindData();
    // pass3: Finally output to the screen
    pass3.width = width;
    pass3.height = height;
    pass3.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                       fm->getShaderPath("OutputShader.frag").c_str());
    pass3.BindData(true);

    nowFrame = CreateTextureRGB32F(width, height);
}

void Renderer::Run() {
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        t2 = clock();
		dt = (double)(t2 - t1) / CLOCKS_PER_SEC;
		fps = 1.0 / dt;
		std::cout << "\r";
		std::cout << std::fixed << std::setprecision(2) << "FPS : " << fps << "    FrameCounter: " << frameCounter;
		t1 = t2;

        integrator->RenderSingleSample(*camera, *scene, *sampler, frameCounter);
        glBindTexture(GL_TEXTURE_2D, nowFrame);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGBA, GL_FLOAT, integrator->GetFloatPixels());

        pass1.m_shader.Use();
        pass1.m_shader.SetUnInt("frameCounter", frameCounter++);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, nowFrame);
        pass1.m_shader.SetInt("nowFrame", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, lastFrame);
        pass1.m_shader.SetInt("lastFrame", 1);
        
        pass1.Draw({});  
        
        pass2.Draw(pass1.colorAttachments);

        pass3.Draw(pass2.colorAttachments);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteTextures(1, &lastFrame);
    glDeleteTextures(1, &nowFrame);
    glfwDestroyWindow(window);
    glfwTerminate();
}

GLuint CreateTextureRGB32F(int w, int h)
{
    GLuint renderTexture;
    glGenTextures(1, &renderTexture);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return renderTexture;
}