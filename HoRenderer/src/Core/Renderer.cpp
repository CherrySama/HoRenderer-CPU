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

    if (!camera || !integrator || !sampler || !scene) {
        throw std::invalid_argument("Renderer dependencies must not be null");
    }

    width = camera->image_width;
    height = camera->image_height;
    if (!WindowInit()) {
        throw std::runtime_error("Failed to initialize the renderer window");
    }

    FileManager* fm = FileManager::getInstance();
    bool pipeline_ready = PipelineConfiguration(fm);
    FileManager::DestroyInstance();
    if (!pipeline_ready) {
        CleanupOpenGLResources();
        glfwDestroyWindow(window);
        window = nullptr;
        glfwTerminate();
        throw std::runtime_error("Failed to configure the rendering pipeline");
    }
}

Renderer::~Renderer()
{
    if (window != nullptr) {
        glfwMakeContextCurrent(window);
        CleanupOpenGLResources();
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

bool Renderer::WindowInit()
{
	// glfw: initialize and configure
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW..." << std::endl;
		return false;
	}
    
    // OpenGL setting
#if defined(__APPLE__)
    // macOS provides the OpenGL 4.1 Core Profile as its highest native version.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_REFRESH_RATE, 60);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // the size of window will be unresizable
    
	// window setting 
    window = glfwCreateWindow(width, height, "HoRenderer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
		return false;
	}
    glfwMakeContextCurrent(window);
    
    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
		glfwDestroyWindow(window);
		window = nullptr;
		glfwTerminate();
		return false;
	}

	glfwSetKeyCallback(window, [](GLFWwindow* target, int key, int, int action, int) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(target, GLFW_TRUE);
		}
	});

	return true;
}

bool Renderer::PipelineConfiguration(FileManager *fm)
{
	if (fm == nullptr)
	{
		std::cout << "fm is null ptr" << std::endl;
		return false;
	}
    fm->init();
    // pass1: Mix current frame and history frames
	pass1.width = width;
	pass1.height = height;
    if (!pass1.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                            fm->getShaderPath("MixFrameShader.frag").c_str())) {
        return false;
    }
    pass1.colorAttachments.push_back(CreateTextureRGB32F(width, height));
    if (pass1.colorAttachments.back() == 0 || !pass1.BindData()) {
        return false;
    }
    // pass2: Save history frames 
    pass2.width = width;
    pass2.height = height;
    if (!pass2.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                            fm->getShaderPath("LastFrameShader.frag").c_str())) {
        return false;
    }
    lastFrame = CreateTextureRGB32F(width, height);
    if (lastFrame == 0) {
        return false;
    }
    pass2.colorAttachments.push_back(lastFrame);
    if (!pass2.BindData()) {
        return false;
    }
    // pass3: Finally output to the screen
    // On macOS Retina displays, the default framebuffer can be larger than
    // the logical window size used by the renderer's image buffers.
    glfwGetFramebufferSize(window, &pass3.width, &pass3.height);
    if (!pass3.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                            fm->getShaderPath("OutputShader.frag").c_str()) ||
        !pass3.BindData(true)) {
        return false;
    }

    nowFrame = CreateTextureRGB32F(width, height);
    return nowFrame != 0;
}

void Renderer::Run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwWindowShouldClose(window)) {
            break;
        }

        auto frame_start = std::chrono::steady_clock::now();

        integrator->RenderImage(*camera, *scene, *sampler, frameCounter);
        if (enable_denoising) {
            float* pixel_data = const_cast<float*>(integrator->GetFloatPixels());
            denoiser.Apply(pixel_data, width, height);
        }

        glBindTexture(GL_TEXTURE_2D, nowFrame);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, integrator->GetFloatPixels());
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

        glfwGetFramebufferSize(window, &pass3.width, &pass3.height);
        pass3.Draw(pass2.colorAttachments);
        
        glfwSwapBuffers(window);

        auto frame_end = std::chrono::steady_clock::now();
        dt = std::chrono::duration<float>(frame_end - frame_start).count();
        fps = dt > 0.0f ? 1.0f / dt : 0.0f;
        std::cout << "\r";
        std::cout << std::fixed << std::setprecision(2) << "FPS : " << fps << "    FrameCounter: " << frameCounter << "    Application average: " << 1000.0f * dt << " ms/frame";
    }
	std::cout << std::endl;
}

void Renderer::CleanupOpenGLResources()
{
    if (!pass1.colorAttachments.empty()) {
        glDeleteTextures(static_cast<GLsizei>(pass1.colorAttachments.size()), pass1.colorAttachments.data());
        pass1.colorAttachments.clear();
    }
    if (lastFrame != 0) {
        glDeleteTextures(1, &lastFrame);
        lastFrame = 0;
    }
    if (nowFrame != 0) {
        glDeleteTextures(1, &nowFrame);
        nowFrame = 0;
    }
    pass2.colorAttachments.clear();
    pass1.Clean();
    pass2.Clean();
    pass3.Clean();
}

GLuint CreateTextureRGB32F(int w, int h)
{
    GLuint renderTexture = 0;
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
