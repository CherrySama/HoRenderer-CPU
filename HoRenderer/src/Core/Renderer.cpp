/*
    Created by Yinghao He on 2025-05-15
*/
#include "Renderer.hpp"

Renderer::Renderer()
{
	WindowInit();
	CameraParams camParams = {16.0f / 9.0f, 2.0f, 1.0f, 1600};
	camera = std::make_unique<Camera>(Vector3f(0.0f, 0.0f, 0.0f));
	camera->Create(camParams);
	integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height);
	scene = std::make_unique<Scene>(); 
	PipelineConfiguration(FileManager::getInstance());
	SceneConfig();
}

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
	scene->Add(std::make_shared<Sphere>(Vector3f(0, 0,-1), 0.5));
    scene->Add(std::make_shared<Sphere>(Vector3f(0,-100.5,-1), 100));
	// scene->Add(std::make_shared<Box>(Vector3f(0.0f, 0.0f, -3.0f),      
    //                                 Vector3f(2.0f, 1.0f, 3.0f)));
    // scene->Add(std::make_shared<Quad>(
    //                                 Vector3f(0, -2, 0), // Center point is at the y=-2 plane
    //                                 Vector3f(0, 1, 0),  // Normal vector up
    //                                 Vector3f(0, 0, 1),  // Forward vector
    //                                 20.0f,              // width
    //                                 20.0f));            // length
}

void Renderer::PipelineConfiguration(FileManager *fm)
{
	if (fm == nullptr)
	{
		std::cout << "fm is null ptr" << std::endl;
		return;
	}
	fm->init();
	pass1.width = camera->image_width;
	pass1.height = camera->image_height;
	pass1.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
					   fm->getShaderPath("FirstPass.frag").c_str());
}

void Renderer::test(FileManager *fm)
{

}

void Renderer::Run()
{
	// start to render
	integrator->RenderImage(*camera, *scene);
	GLuint renderTexture = GetTextureRGB32F(camera->image_width, camera->image_height, *integrator);
	pass1.BindData(true);

	    // Render Loop 
    while (!glfwWindowShouldClose(window)) {
        // Input Processing
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Buffer Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        pass1.Draw({renderTexture});
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
	
	glDeleteTextures(1, &renderTexture);
    glfwDestroyWindow(window);
    glfwTerminate();
}

GLuint GetTextureRGB32F(int width, int height, const Integrator &integrator)
{
	// Creating Textures
    GLuint renderTexture;
    glGenTextures(1, &renderTexture);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    // Setting texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Loading image data into a texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, integrator.GetPixels());
    glGenerateMipmap(GL_TEXTURE_2D);

	return renderTexture;
}
