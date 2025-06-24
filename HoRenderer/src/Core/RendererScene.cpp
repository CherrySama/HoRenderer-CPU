/*
    Created by Yinghao He on 2025-05-24
*/
#include "RendererScene.hpp"
#include "Shape.hpp"
#include "Material.hpp"
#include "Filter.hpp"
#include "Transform.hpp"
// #include "Medium.hpp"
// #include "../Common/FileManager.hpp"

namespace RendererScene
{    
    std::shared_ptr<Renderer> CornellBox()
    {
        CameraParams camParams = {
            1.0f,                              
            900,                              
            40.0f,                             
            Vector3f(278.0f, 278.0f, -800.0f), 
            Vector3f(278.0f, 278.0f, 0.0f),   
            Vector3f(0.0f, 1.0f, 0.0f),       
            0.0f,                              
            1.0f                               
        };
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height, 16, 30);
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::GAUSSIAN);
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        auto emitTexture = std::make_shared<SolidTexture>(Vector3f(50.0f, 50.0f, 50.0f));
        auto redTexture = std::make_shared<SolidTexture>(Vector3f(0.65f, 0.05f, 0.05f));
        auto whiteTexture = std::make_shared<SolidTexture>(Vector3f(0.73f, 0.73f, 0.73f));
        auto greenTexture = std::make_shared<SolidTexture>(Vector3f(0.12f, 0.45f, 0.15f));

        auto emitMaterial = std::make_shared<DiffuseLight>(emitTexture);
        auto redMaterial = std::make_shared<Diffuse>(redTexture);
        auto whiteMaterial =  std::make_shared<Diffuse>(whiteTexture);
        auto greenMaterial = std::make_shared<Diffuse>(greenTexture);
        auto mirrorMaterial = std::make_shared<Metal>(Vector3f(0.8f,0.8f,0.8f));

        scene->Add(std::make_shared<Quad>(Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          greenMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          redMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(127.5f, 554.0f, 127.5f),
                                          Vector3f(300.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 300.0f),
                                          emitMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          whiteMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(555.0f, 555.0f, 555.0f),
                                          Vector3f(-555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, -555.0f),
                                          whiteMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 555.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          whiteMaterial));

        auto box1 = std::make_shared<Box>(Vector3f(0.0f,0.0f,0.0f),
                                         Vector3f(165.0f, 165.0f, 165.0f),
                                         whiteMaterial);
        auto rotate_box1 = Transform::rotate(box1, RotationAxis::Y,15.0f);
        auto translated_box1 = Transform::translate(rotate_box1, Vector3f(212.5f,82.5f,147.5f));
        scene->Add(translated_box1);
        // scene->Add(std::make_shared<HomogeneousMedium>(translated_box1, 0.01f, Vector3f(0)));

        auto box2 = std::make_shared<Box>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(165.0f, 330.0f, 165.0f),
                                          mirrorMaterial);
        auto rotate_box2 = Transform::rotate(box2, RotationAxis::Y,-18.0f);
        auto translated_box2 = Transform::translate(rotate_box2, Vector3f(347.5f, 165.0f, 377.5f));
        scene->Add(translated_box2);
        // scene->Add(std::make_shared<HomogeneousMedium>(translated_box2, 0.01f, Vector3f(1)));

        // scene->BuildBVH();
        auto renderer = std::make_shared<Renderer>(std::move(camera), std::move(integrator), std::move(sampler), std::move(scene));
        return renderer;
    }
}