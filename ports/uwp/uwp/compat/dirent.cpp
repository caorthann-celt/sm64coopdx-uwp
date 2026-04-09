#include "dirent.h"

#include <filesystem>
#include <string>
#include <cstring>

struct DIR {
    std::filesystem::directory_iterator it;
    std::filesystem::directory_iterator end;
    struct dirent ent;
};

DIR* opendir(const char* name) {
    if (name == nullptr) {
        return nullptr;
    }

    try {
        std::filesystem::path path(name);
        if (!std::filesystem::is_directory(path)) {
            return nullptr;
        }

        return new DIR { std::filesystem::directory_iterator(path), std::filesystem::directory_iterator(), {} };
    } catch (...) {
        return nullptr;
    }
}

struct dirent* readdir(DIR* dirp) {
    if (dirp == nullptr || dirp->it == dirp->end) {
        return nullptr;
    }

    const std::string name = dirp->it->path().filename().string();
    std::strncpy(dirp->ent.d_name, name.c_str(), sizeof(dirp->ent.d_name) - 1);
    dirp->ent.d_name[sizeof(dirp->ent.d_name) - 1] = '\0';
    ++dirp->it;
    return &dirp->ent;
}

int closedir(DIR* dirp) {
    delete dirp;
    return 0;
}
