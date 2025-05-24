/*
    Created by Yinghao He on 2025-05-15
*/
#include "Renderer.hpp"
#include "Material.hpp"
#include "Shape.hpp"

Renderer::Renderer()
{
    CameraParams camParams = { 16.0f / 9.0f,
                               1600,
                               20.0f,
                               Vector3f(13.0f, 2.0f, 3.0f),
                               Vector3f(0.0f, 0.0f, 0.0f),
                               Vector3f(0.0f, 1.0f, 0.0f),
                               0.6f,
                               10.0f};
    
    camera = std::make_unique<Camera>();
    camera->Create(camParams);
    width = camera->image_width;
    height = camera->image_height;
    
	integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height);
	sampler = std::make_unique<Sampler>(64);
    scene = std::make_unique<Scene>();
    
    WindowInit();
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
    auto ground_material = std::make_shared<DiffuseBRDF>(Vector3f(0.5f, 0.5f, 0.5f), 0.3f);
    scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f), 
											Vector3f(0.0f, 1.0f, 0.0f),  
											Vector3f(0.0f, 0.0f, 1.0f),  
											100.0f,                      
											100.0f,                      
											ground_material));

    for (int a = -6; a < 6; a++) {
        for (int b = -6; b < 6; b++) {
            auto choose_mat = sampler->random_float();
            Vector3f center(a + 0.9f * sampler->random_float(),
                            0.2f,
                            b + 0.9f * sampler->random_float());

            if (glm::length(center - Vector3f(4.0f, 0.2f, 0.0f)) > 0.9f) {
                std::shared_ptr<Material> sphere_material;

                if (choose_mat < 0.8f) {
                    // diffuse
					float r = sampler->random_float();
                    float g = sampler->random_float();  
                    float b = sampler->random_float();
                    Vector3f albedo = Vector3f(r * r, g * g, b * b);
                    sphere_material = std::make_shared<DiffuseBRDF>(albedo);
                    scene->Add(std::make_shared<Sphere>(center, 0.2f, sphere_material));
                } else if (choose_mat < 0.95f) {
                    // metal
                    Vector3f albedo(sampler->random_float(0.5f, 1.0f),
                                    sampler->random_float(0.5f, 1.0f),
                                    sampler->random_float(0.5f, 1.0f));
                    float fuzz = sampler->random_float(0.0f, 0.5f);
					sphere_material = std::make_shared<Metal>(albedo, fuzz);
					scene->Add(std::make_shared<Sphere>(center, 0.2f, sphere_material));
                } else {
                    // glass
                    sphere_material = std::make_shared<Dielectric>(1.5f);
                    scene->Add(std::make_shared<Sphere>(center, 0.2f, sphere_material));
				}
			}
		}
    }

    auto material1 = std::make_shared<Dielectric>(1.5f);
    scene->Add(std::make_shared<Sphere>(Vector3f(0.0f, 1.0f, 0.0f), 1.0f, material1));

    auto material2 = std::make_shared<DiffuseBRDF>(Vector3f(0.4f, 0.2f, 0.1f));
    scene->Add(std::make_shared<Sphere>(Vector3f(-4.0f, 1.0f, 0.0f), 1.0f, material2));

    auto material3 = std::make_shared<Metal>(Vector3f(0.7f, 0.6f, 0.5f), 0.0f);
    scene->Add(std::make_shared<Sphere>(Vector3f(4.0f, 1.0f, 0.0f), 1.0f, material3));
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

void Renderer::Run()
{
	// start to render
	integrator->RenderImage(*camera, *scene, *sampler);
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
