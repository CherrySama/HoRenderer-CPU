/*
	Created by Yinghao He on 2025-05-15
*/
#include "Core/Util.hpp"
#include "Core/RendererScene.hpp"


int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    try {
        // auto renderer = RendererScene::CornellBox();
        auto renderer = RendererScene::SpaichingenHill();
        renderer->Run();
    } catch (const std::exception& error) {
        std::cerr << "Renderer startup failed: " << error.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
