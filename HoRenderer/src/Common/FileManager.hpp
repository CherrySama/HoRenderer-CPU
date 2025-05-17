/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <Windows.h>

class FileManager {
private:
    // Singleton Pattern
    static FileManager* instance;
    
    std::string projectRoot;
    std::unordered_map<std::string, std::string> pathCache;
    
    FileManager();

public:
    static FileManager* getInstance();
    
    // Initialization Path
    void init();
    
    // Get paths to different types of resources
    std::string getShaderPath(const std::string& filename);
    
    // Print the current working directory and executable file path
    // void printPaths();
};