/*
	Created by Yinghao He on 2025-05-15
*/
#include "Core/Util.hpp"
#include "Core/RendererScene.hpp"


int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    // auto renderer = RendererScene::CornellBox();
    auto renderer = RendererScene::TestScene();
    renderer->Run();

    return 0;
}