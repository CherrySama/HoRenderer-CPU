/*
	Created by Yinghao He on 2025-05-15
*/
#include "Core/Util.hpp"
#include "Core/RenderPass.hpp"
#include "Core/Integrator.hpp"
#include "Core/Camera.hpp"
#include "Core/Scene.hpp"
#include "Common/FileManager.hpp"
#include "Core/Renderer.hpp"

int render()
{
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    CameraParams camParams = {16.0f / 9.0f, 2.0f, 1.0f, 1600};
    Camera cam(Vector3f(0));
    cam.Create(camParams);

    std::cout << cam.image_width << std::endl;
    std::cout << cam.image_height << std::endl;

    // int width = 1600;
    // int height = 900;
    GLFWwindow* window = glfwCreateWindow(cam.image_width, cam.image_height, "HoRenderer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    FileManager* fm = FileManager::getInstance();
    fm->init();

    // RenderPass renderPass;
    auto renderPass = std::make_unique<RenderPass>();
    renderPass->width = cam.image_width;
    renderPass->height = cam.image_height;
    renderPass->ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                            fm->getShaderPath("FirstPass.frag").c_str());

    // World
    Scene world;
    world.Add(std::make_shared<Sphere>(Vector3f(0, 0,-1), 0.5f));
    world.Add(std::make_shared<Sphere>(Vector3f(0,-100.5,-1), 100.0f));

    Integrator integrator(cam.image_width, cam.image_height);
    integrator.RenderImage(cam, world);

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cam.image_width, cam.image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, integrator.GetPixels());
    glGenerateMipmap(GL_TEXTURE_2D);

    renderPass->BindData(true);

    // Render Loop 
    while (!glfwWindowShouldClose(window)) {
        // Input Processing
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Buffer Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        renderPass->Draw({renderTexture});
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleaning up resources
    // delete[] pixels;
    glDeleteTextures(1, &renderTexture);
    renderPass.reset();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

int testFileManager()
{   
    // Init FileManager
    FileManager* fm = FileManager::getInstance();
    fm->init();
    
    // Test to get the shader path
    std::cout << "\n===== Testing the Shader Path =====" << std::endl;
    std::string vertPath = fm->getShaderPath("VertexShader.vert");
    std::string fragPath = fm->getShaderPath("FirstPass.frag");
    
    std::cout << "Vertex Shader Path: " << vertPath << std::endl;
    std::cout << "Fragment shader path: " << fragPath << std::endl;
    
    // Test the path cache function (getting the same path again should use the cache)
    std::cout << "\n===== Test path cache =====" << std::endl;
    std::string vertPath2 = fm->getShaderPath("VertexShader.vert");
    std::cout << "Get the vertex shader path again: " << vertPath2 << std::endl;
    std::cout << "Are the two paths the same?: " << (vertPath == vertPath2 ? "Yes" : "No") << std::endl;
    
    return 0;
}

int testRenderer()
{
    Renderer renderer;
    renderer.Run();

    return 0;
}

int main()
{
    // render();
    // testFileManager();
    testRenderer();
}