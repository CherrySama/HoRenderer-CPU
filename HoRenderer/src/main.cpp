/*
	Created by Yinghao He on 2025-05-15
*/
#include "Core/Util.hpp"
#include "Core/RendererScene.hpp"


int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    auto renderer = RendererScene::TestScene();
    // auto renderer = RendererScene::CornellBox();
    renderer->Run();

    return 0;
}