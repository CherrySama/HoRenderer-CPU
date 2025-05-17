/*
    Created by Yinghao He on 2025-05-15
*/
#include "Renderer.hpp"

void Renderer::WindowInit()
{
	// glfw: initialize and configure
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW..." << std::endl;
		return;
	}
    
    // 设置OpenGL版本
	glfwWindowHint(GLFW_SAMPLES, max_samples);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_REFRESH_RATE, 60);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // the size of window will be unresizable
    
	// window setting 
	GLFWmonitor* monitor = isfullscreen ? glfwGetPrimaryMonitor() : NULL;
    window = glfwCreateWindow(width, height, "HoRenderer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
		return;
	}
    glfwMakeContextCurrent(window);
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 隐藏鼠标光标
    
    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
		return;
	}
}

void Renderer::SceneConfig()
{
}

void Renderer::PipelineConfiguration()
{
	pass1.width = width;
    pass1.height = height;
    pass1.ShaderConfig("../../../../src/Shader/VertexShader.vert", "../../../../src/Shader/FirstPass.frag");
}

void Renderer::Run()
{
}

GLuint GetTextureRGB32F(int width, int height)
{
    return GLuint();
}
