/*
	Created by Yinghao He on 2025-05-15
*/
#include "Core/Util.hpp"
#include "Core/RenderPass.hpp"
#include "Core/Integrator.hpp"
#include "Common/FileManager.hpp"

int render()
{
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    int width = 1200;
    int height = 900;
    GLFWwindow* window = glfwCreateWindow(width, height, "raytracer", NULL, NULL);
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

    RenderPass renderPass;
    renderPass.width = width;
    renderPass.height = height;
    renderPass.ShaderConfig(fm->getShaderPath("VertexShader.vert").c_str(),
                            fm->getShaderPath("FirstPass.frag").c_str());

    // Creating Textures
    GLuint renderTexture;
    glGenTextures(1, &renderTexture);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    // Setting texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    Integrator integrator(width, height);
    integrator.RenderImage();


    // Loading image data into a texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, integrator.GetPixels());
    glGenerateMipmap(GL_TEXTURE_2D);

    renderPass.BindData(true);

    // Render Loop 
    while (!glfwWindowShouldClose(window)) {
        // Input Processing
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Buffer Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        renderPass.Draw({renderTexture});
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleaning up resources
    // delete[] pixels;
    glDeleteTextures(1, &renderTexture);
    renderPass.Clean();

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

int main()
{
    render();
    // testFileManager();
}