// OS-specific code
#include "Platform.h"
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#endif

constexpr char const* config_folder_name = "Chimp8";
constexpr char const* config_name = "Chimp8.ini";

constexpr int idle_sleep = 1000;
constexpr int win_idle_sleep = 1;

std::string get_program_path() {
#ifdef _WIN32
    WCHAR program_path[MAX_PATH];
    GetModuleFileNameW(NULL, program_path, MAX_PATH);
    PathRemoveFileSpecW(program_path);

    char program_path_conv[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, program_path, -1, program_path_conv, MAX_PATH, NULL, NULL);

    return std::string(program_path_conv);
#else
    char program_path[PATH_MAX];
    if (readlink("/proc/self/exe", program_path, PATH_MAX) == -1) {
        throw std::runtime_error("Couldn't find program path");
    }

    std::string program_path_str(program_path);
    return program_path_str.substr(0, program_path_str.find_last_of("/"));
#endif
}

std::string get_config_path() {
#ifdef _WIN32
    std::string config_path = get_program_path() + "\\" + config_name;
    return config_path;
#else
    std::string config_home;
    
    if (const char* config_home_raw = std::getenv("XDG_CONFIG_HOME")) {
        config_home = std::string(config_home_raw);
    }
    else {
        if (const char* user_home = std::getenv("HOME")) {
            config_home = std::string(user_home) + "/.config";
        }
        else {
            throw std::runtime_error("Couldn't find appropriate location to load config");
        }
    }

    std::string config_path = config_home + "/" + config_folder_name + "/" + config_name;
    return config_path;
#endif
}

void main_sleep() {
    #ifdef _WIN32
    Sleep(win_idle_sleep);
    #else
    usleep(idle_sleep);
    #endif
}
