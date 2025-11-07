// file_explorer.cpp
// Compile: g++ -std=c++17 file_explorer.cpp -o file_explorer

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace fs = std::filesystem;

void print_header() {
    std::cout << "=== Simple Linux File Explorer (C++ / Console) ===\n";
}

std::string perms_to_string(fs::perms p) {
    std::string s = "----------";
    auto set = [&](fs::perms bit, int idx, char c){
        if ((p & bit) != fs::perms::none) s[idx] = c;
    };
    set(fs::perms::owner_read, 0, 'r');
    set(fs::perms::owner_write,1,'w');
    set(fs::perms::owner_exec,2,'x');
    set(fs::perms::group_read,3,'r');
    set(fs::perms::group_write,4,'w');
    set(fs::perms::group_exec,5,'x');
    set(fs::perms::others_read,6,'r');
    set(fs::perms::others_write,7,'w');
    set(fs::perms::others_exec,8,'x');
    return s;
}

void list_directory(const fs::path &p) {
    try {
        std::cout << "Listing: " << p << "\n";
        std::cout << std::left << std::setw(40) << "Name" 
                  << std::setw(12) << "Type"
                  << std::setw(12) << "Size"
                  << std::setw(12) << "Perms" << "\n";
        std::cout << std::string(80,'-') << "\n";
        for (auto &entry : fs::directory_iterator(p)) {
            auto name = entry.path().filename().string();
            std::string type = entry.is_directory() ? "Directory" : "File";
            std::uintmax_t size = 0;
            try { if (fs::is_regular_file(entry.path())) size = fs::file_size(entry.path()); } catch(...) {}
            std::string perms = perms_to_string(entry.status().permissions());
            std::cout << std::left << std::setw(40) << name 
                      << std::setw(12) << type
                      << std::setw(12) << (entry.is_directory() ? "-" : std::to_string(size))
                      << std::setw(12) << perms << "\n";
        }
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error listing directory: " << e.what() << "\n";
    }
}

bool change_directory(fs::path &current, const std::string &target) {
    fs::path newp;
    if (target == "..") newp = current.parent_path();
    else if (fs::path(target).is_absolute()) newp = fs::path(target);
    else newp = current / target;

    try {
        if (fs::exists(newp) && fs::is_directory(newp)) {
            current = fs::canonical(newp);
            return true;
        } else {
            std::cerr << "Directory not found: " << newp << "\n";
            return false;
        }
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error changing directory: " << e.what() << "\n";
        return false;
    }
}

bool create_file(const fs::path &current, const std::string &name) {
    fs::path p = current / name;
    try {
        std::ofstream ofs(p);
        if (!ofs) {
            std::cerr << "Failed to create file: " << p << "\n";
            return false;
        }
        ofs.close();
        return true;
    } catch (...) {
        std::cerr << "Error creating file.\n";
        return false;
    }
}

bool create_directory(const fs::path &current, const std::string &name) {
    try {
        fs::path p = current / name;
        if (fs::create_directory(p)) return true;
        else { std::cerr << "Failed to create directory or already exists.\n"; return false; }
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error creating directory: " << e.what() << "\n";
        return false;
    }
}

bool delete_path(const fs::path &current, const std::string &name) {
    try {
        fs::path p = current / name;
        if (!fs::exists(p)) { std::cerr << "Path not found: " << p << "\n"; return false; }
        if (fs::is_directory(p)) {
            fs::remove_all(p);
        } else {
            fs::remove(p);
        }
        return true;
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error deleting path: " << e.what() << "\n";
        return false;
    }
}

bool copy_path(const fs::path &current, const std::string &src_name, const std::string &dest_name) {
    try {
        fs::path src = current / src_name;
        fs::path dest = current / dest_name;
        if (!fs::exists(src)) { std::cerr << "Source not found: " << src << "\n"; return false; }
        if (fs::is_directory(src)) {
            fs::copy(src, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        } else {
            fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
        }
        return true;
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error copying: " << e.what() << "\n";
        return false;
    }
}

bool move_path(const fs::path &current, const std::string &src_name, const std::string &dest_name) {
    try {
        fs::path src = current / src_name;
        fs::path dest = current / dest_name;
        if (!fs::exists(src)) { std::cerr << "Source not found: " << src << "\n"; return false; }
        fs::rename(src, dest);
        return true;
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error moving/renaming: " << e.what() << "\n";
        return false;
    }
}

void search_recursive(const fs::path &current, const std::string &query) {
    try {
        for (auto &entry : fs::recursive_directory_iterator(current)) {
            if (entry.path().filename().string().find(query) != std::string::npos) {
                std::cout << entry.path() << "\n";
            }
        }
    } catch (fs::filesystem_error &e) {
        std::cerr << "Search error: " << e.what() << "\n";
    }
}

fs::perms octal_to_perms(int octal) {
    fs::perms p = fs::perms::none;
    int owner = (octal / 100) % 10;
    int group = (octal / 10) % 10;
    int others = octal % 10;
    auto add_bits = [&](int val, fs::perms r, fs::perms w, fs::perms x) {
        if (val & 4) p |= r;
        if (val & 2) p |= w;
        if (val & 1) p |= x;
    };
    add_bits(owner, fs::perms::owner_read, fs::perms::owner_write, fs::perms::owner_exec);
    add_bits(group, fs::perms::group_read, fs::perms::group_write, fs::perms::group_exec);
    add_bits(others, fs::perms::others_read, fs::perms::others_write, fs::perms::others_exec);
    return p;
}

void show_permissions(const fs::path &current, const std::string &name) {
    fs::path p = current / name;
    try {
        if (!fs::exists(p)) { std::cerr << "Not found: " << p << "\n"; return; }
        auto st = fs::status(p);
        std::cout << "Permissions: " << perms_to_string(st.permissions()) << "\n";
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error reading permissions: " << e.what() << "\n";
    }
}

bool change_permissions(const fs::path &current, const std::string &name, int octal) {
    try {
        fs::path p = current / name;
        if (!fs::exists(p)) { std::cerr << "Not found: " << p << "\n"; return false; }
        fs::perms newp = octal_to_perms(octal);
        fs::permissions(p, newp, fs::perm_options::replace);
        return true;
    } catch (fs::filesystem_error &e) {
        std::cerr << "Error changing permissions: " << e.what() << "\n";
        return false;
    }
}

void print_help() {
    std::cout << "Commands:\n"
              << " ls                     - list current directory\n"
              << " pwd                    - show current directory\n"
              << " cd <dir>               - change directory (use .. to go up)\n"
              << " touch <file>           - create empty file\n"
              << " mkdir <dir>            - create directory\n"
              << " rm <name>              - delete file or directory (recursive)\n"
              << " cp <src> <dest>        - copy file or directory\n"
              << " mv <src> <dest>        - move or rename\n"
              << " find <name>            - search recursively\n"
              << " perms <name>           - show permissions\n"
              << " chmod <name> <octal>   - change permissions (e.g., 755)\n"
              << " help                   - show help\n"
              << " exit                   - exit program\n";
}

int main(){
    fs::path current = fs::current_path();
    print_header();
    print_help();

    while (true) {
        std::cout << "\n[" << current << "] $ ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "ls") list_directory(current);
        else if (cmd == "pwd") std::cout << current << "\n";
        else if (cmd == "cd") { std::string d; iss >> d; change_directory(current, d); }
        else if (cmd == "touch") { std::string f; iss >> f; create_file(current, f); }
        else if (cmd == "mkdir") { std::string d; iss >> d; create_directory(current, d); }
        else if (cmd == "rm") { std::string n; iss >> n; delete_path(current, n); }
        else if (cmd == "cp") { std::string s,d; iss >> s >> d; copy_path(current, s, d); }
        else if (cmd == "mv") { std::string s,d; iss >> s >> d; move_path(current, s, d); }
        else if (cmd == "find") { std::string q; iss >> q; search_recursive(current, q); }
        else if (cmd == "perms") { std::string n; iss >> n; show_permissions(current, n); }
        else if (cmd == "chmod") { std::string n; int o; iss >> n >> o; change_permissions(current, n, o); }
        else if (cmd == "help") print_help();
        else if (cmd == "exit") break;
        else std::cout << "Unknown command. Type 'help'.\n";
    }

    std::cout << "Bye.\n";
    return 0;
}

