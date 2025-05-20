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