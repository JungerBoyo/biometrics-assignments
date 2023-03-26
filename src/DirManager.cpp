#include "DirManager.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

using namespace bm;

DirManager::DirManager(fs::path&& dir_path, std::vector<std::string>&& accepted_extensions)         
    : accepted_extensions(std::move(accepted_extensions)), dir_path(std::move(dir_path)) {

    if (!fs::is_directory(this->dir_path)) {
        spdlog::error("{} is not directory", this->dir_path.c_str());
        return; 
    }
    refresh();
}
void DirManager::refresh() {
    files.clear();
    for (const auto& entry : fs::directory_iterator{dir_path}) {
        if (entry.is_regular_file()) {
            auto extension = entry.path().extension().string();
            std::transform(
                extension.begin(), 
                extension.end(), 
                extension.begin(), 
                [](u8 c) { return std::tolower(c); }
            );
            
            if (std::find(accepted_extensions.begin(), accepted_extensions.end(), extension) != accepted_extensions.end()) {
                files.emplace_back(std::move(entry.path()));                
            }
        }
    }
}