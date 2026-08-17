/*
    Created by Yinghao He on 2025-05-16
*/
// FileManager.cpp
#include "FileManager.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace {

std::filesystem::path GetExecutablePath()
{
#if defined(_WIN32)
    std::vector<char> buffer(1024);
    while (true) {
        DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }

        if (length < buffer.size() - 1) {
            return std::filesystem::path(std::string(buffer.data(), length));
        }

        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    uint32_t buffer_size = 1024;
    while (buffer_size <= 1024 * 1024) {
        std::vector<char> buffer(buffer_size);
        uint32_t actual_size = buffer_size;
        if (_NSGetExecutablePath(buffer.data(), &actual_size) == 0) {
            return std::filesystem::path(buffer.data());
        }

        buffer_size = actual_size;
    }
    return {};
#elif defined(__linux__)
    std::vector<char> buffer(PATH_MAX);
    ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) {
        return {};
    }

    return std::filesystem::path(std::string(buffer.data(), static_cast<size_t>(length)));
#else
    return {};
#endif
}

bool IsProjectRoot(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_directory(path / "src" / "Shader", error) &&
           std::filesystem::is_directory(path / "assets", error);
}

std::filesystem::path FindProjectRoot(std::filesystem::path start)
{
    if (start.empty()) {
        return {};
    }

    std::error_code error;
    start = std::filesystem::absolute(start, error);
    if (error) {
        return {};
    }

    while (true) {
        if (IsProjectRoot(start)) {
            return start;
        }

        const auto parent = start.parent_path();
        if (parent == start) {
            break;
        }
        start = parent;
    }

    return {};
}

}

FileManager* FileManager::instance = nullptr;

FileManager* FileManager::getInstance() {
    if (instance == nullptr) {
        instance = new FileManager();
    }
    return instance;
}

void FileManager::DestroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

void FileManager::init() {
    const auto executable_path = GetExecutablePath();
    std::error_code error;
    const auto current_path = std::filesystem::current_path(error);

    std::filesystem::path search_path;
    if (!executable_path.empty()) {
        search_path = executable_path.parent_path();
    } else if (!error) {
        search_path = current_path;
    }

    auto root = FindProjectRoot(search_path);
    if (root.empty() && !error) {
        root = FindProjectRoot(current_path);
    }

    if (root.empty()) {
        root = search_path.empty() ? current_path : search_path;
    }

    projectRoot = root.generic_string();
}

std::string FileManager::getShaderPath(const std::string& filename) {
    std::filesystem::path shaderPath = std::filesystem::path(projectRoot) / "src" / "Shader" / filename;
    return shaderPath.generic_string();
}

std::string FileManager::getTexturePath(const std::string &filename) {
    std::filesystem::path texturePath = std::filesystem::path(projectRoot) / "assets" / "textures" / filename;
    return texturePath.generic_string();
}

std::string FileManager::getModelPath(const std::string &filename) {
    std::filesystem::path materialPath = std::filesystem::path(projectRoot) / "assets" / "models" / filename;
    return materialPath.generic_string();
}

std::string FileManager::getEnvBGPath(const std::string &filename) {
    std::filesystem::path materialPath = std::filesystem::path(projectRoot) / "assets" / "scenes" / filename;
    return materialPath.generic_string();
}
