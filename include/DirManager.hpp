#ifndef BM_DIR_MANAGER_HPP
#define BM_DIR_MANAGER_HPP

#include <vector>
#include <string>

#include "Types.hpp"

namespace bm {
    struct DirManager {
        fs::path dir_path;
        std::vector<fs::path> files;
        std::vector<std::string> accepted_extensions;

        DirManager(fs::path&& dir_path, std::vector<std::string>&& accepted_extensions);
        void refresh();
    };
};

#endif