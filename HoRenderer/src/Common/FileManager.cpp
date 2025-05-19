/*
    Created by Yinghao He on 2025-05-16
*/
// FileManager.cpp
#include "FileManager.hpp"
#define PATH_SEPARATOR "/"

FileManager* FileManager::instance = nullptr;

FileManager::FileManager() : projectRoot("") {
}

FileManager* FileManager::getInstance() {
    if (instance == nullptr) {
        instance = new FileManager();
    }
    return instance;
}

void FileManager::init() {
    // Get the executable file path
    char execPath[1024] = {0};
    GetModuleFileName(NULL, execPath, 1024);
    
    // exe file is in build/windows/x64/release 
    std::filesystem::path exePath(execPath);
    auto parentPath = exePath.parent_path(); // release
    parentPath = parentPath.parent_path();   // x64
    parentPath = parentPath.parent_path();   // windows
    parentPath = parentPath.parent_path();   // build
    projectRoot = parentPath.parent_path().generic_string(); // HoRenderer
    
    // std::cout << "Project root: " << projectRoot << std::endl;
}

std::string FileManager::getShaderPath(const std::string& filename) {
    std::string key = "shader_" + filename;
    if (pathCache.find(key) != pathCache.end()) {
        return pathCache[key];
    }
    
    std::filesystem::path shaderPath = std::filesystem::path(projectRoot) / "src" / "Shader" / filename;
    std::string path = shaderPath.generic_string();
    pathCache[key] = path;
    return path;
}

