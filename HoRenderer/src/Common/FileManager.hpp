/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include <string>
#include <unordered_map>
#include <Windows.h>

class FileManager {
private:
    FileManager() = default;

    // Singleton Pattern
    static FileManager* instance;
    
    std::string projectRoot;
    std::unordered_map<std::string, std::string> pathCache;
    

public:
    static FileManager *getInstance();
    static void DestroyInstance();
    
    // Initialization Path
    void init();
    
    // Get paths to different types of resources
    std::string getShaderPath(const std::string &filename);
    std::string getTexturePath(const std::string &filename);
    std::string getMaterialPath(const std::string &filename);
};