/*
	Created by Yinghao He on 2025-05-15
*/
#include "Core/Util.hpp"
#include "Core/RendererScene.hpp"


int testRenderer()
{
    auto renderer = RendererScene::TestScene();
    renderer->Run();

    return 0;
}

int main()
{
    // render();
    // testFileManager();
    testRenderer();
}