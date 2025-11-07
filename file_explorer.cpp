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
using namespace std;

namespace fs = filesystem;

void print_header() {
    cout << "=== Simple Linux File Explorer (C++ / Console) ===\n";
}

string perms_to_string(fs::perms p) {
    string s = "----------";
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
        cout << "Listing: " << p << "\n";
        cout << left << setw(40) << "Name" 
             << setw(12) << "Type"
             << setw(12) << "Size"
             << setw(12) << "Perms" << "\n";
        cout << string(80,'-') << "\n";
        for (auto &entry : fs::directory_iterator(p)) {
            auto name = entry.path().filename().string();
            string type = entry.is_directory() ? "Directory" : "File";
            uintmax_t size = 0;
            try { if (fs::is_regular_file(entry.path())) size = fs::file_size(entry.path()); } catch(...) {}
            string perms = perms_to_string(entry.status().permissions());
            cout << left << setw(40) << name 
                 << setw(12) << type
                 << setw(12) << (entry.is_directory() ? "-" : to_string(size))
                 << setw(12) << perms << "\n";
        }
    } catch (fs::filesystem_error &e) {
        cerr << "Error listing directory: " << e.what() << "\n";
    }
}

bool change_directory(fs::path &current, const string &target) {
    fs::path newp;
    if (target == "..") newp = current.parent_path();
    else if (fs::path(target).is_absolute()) newp = fs::path(target);
    else newp = current / target;

    try {
        if (fs::exists(newp) && fs::is_directory(newp)) {
            current = fs::canonical(newp);
            return true;
        } else {
            cerr << "Directory not found: " << newp << "\n";
            return false;
        }
    } catch (fs::filesystem_error &e) {
        cerr << "Error changing directory: " << e.what() << "\n";
        return false;
    }
}

bool create_file(const fs::path &current, const string &name) {
    fs::path p = current / name;
    try {
        ofstream ofs(p);
        if (!ofs) {
            cerr << "Failed to create file: " << p << "\n";
            return false;
        }
        ofs.close();
        return true;
    } catch (...) {
        cerr << "Error creating file.\n";
        return false;
    }
}

bool create_directory(const fs::path &current, const string &name) {
    try {
        fs::path p = current / name;
        if (fs::create_directory(p)) return true;
        else { cerr << "Failed to create directory or already exists.\n"; return false; }
    } catch (fs::filesystem_error &e) {
        cerr << "Error creating directory: " << e.what() << "\n";
        return false;
    }
}

bool delete_path(const fs::path &current, const string &name) {
    try {
        fs::path p = current / name;
        if (!fs::exists(p)) { cerr << "Path not found: " << p << "\n"; return false; }
        if (fs::is_directory(p)) {
            fs::remove_all(p);
        } else {
            fs::remove(p);
        }
        return true;
    } catch (fs::filesystem_error &e) {
        cerr << "Error deleting path: " << e.what() << "\n";
        return false;
    }
}

bool copy_path(const fs::path &current, const string &src_name, const string &dest_name) {
    try {
        fs::path src = current / src_name;
        fs::path dest = current / dest_name;
        if (!fs::exists(src)) { cerr << "Source not found: " << src << "\n"; return false; }
        if (fs::is_directory(src)) {
            fs::copy(src, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        } else {
            fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
        }
        return true;
    } catch (fs::filesystem_error &e) {
        cerr << "Error copying: " << e.what() << "\n";
        return false;
    }
}

bool move_path(const fs::path &current, const string &src_name, const string &dest_name) {
    try {
        fs::path src = current / src_name;
        fs::path dest = current / dest_name;
        if (!fs::exists(src)) { cerr << "Source not found: " << src << "\n"; return false; }
        fs::rename(src, dest);
        return true;
    } catch (fs::filesystem_error &e) {
        cerr << "Error moving/renaming: " << e.what() << "\n";
        return false;
    }
}

void search_recursive(const fs::path &current, const string &query) {
    try {
        for (auto &entry : fs::recursive_directory_iterator(current)) {
            if (entry.path().filename().string().find(query) != string::npos) {
                cout << entry.path() << "\n";
            }
        }
    } catch (fs::filesystem_error &e) {
        cerr << "Search error: " << e.what() << "\n";
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

void show_permissions(const fs::path &current, const string &name) {
    fs::path p = current / name;
    try {
        if (!fs::exists(p)) { cerr << "Not found: " << p << "\n"; return; }
        auto st = fs::status(p);
        cout << "Permissions: " << perms_to_string(st.permissions()) << "\n";
    } catch (fs::filesystem_error &e) {
        cerr << "Error reading permissions: " << e.what() << "\n";
    }
}

bool change_permissions(const fs::path &current, const string &name, int octal) {
    try {
        fs::path p = current / name;
        if (!fs::exists(p)) { cerr << "Not found: " << p << "\n"; return false; }
        fs::perms newp = octal_to_perms(octal);
        fs::permissions(p, newp, fs::perm_options::replace);
        return true;
    } catch (fs::filesystem_error &e) {
        cerr << "Error changing permissions: " << e.what() << "\n";
        return false;
    }
}

void print_help() {
    cout << "Commands:\n"
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
        cout << "\n[" << current << "] $ ";
        string line;
        if (!getline(cin, line)) break;
        if (line.empty()) continue;
        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ls") list_directory(current);
        else if (cmd == "pwd") cout << current << "\n";
        else if (cmd == "cd") { string d; iss >> d; change_directory(current, d); }
        else if (cmd == "touch") { string f; iss >> f; create_file(current, f); }
        else if (cmd == "mkdir") { string d; iss >> d; create_directory(current, d); }
        else if (cmd == "rm") { string n; iss >> n; delete_path(current, n); }
        else if (cmd == "cp") { string s,d; iss >> s >> d; copy_path(current, s, d); }
        else if (cmd == "mv") { string s,d; iss >> s >> d; move_path(current, s, d); }
        else if (cmd == "find") { string q; iss >> q; search_recursive(current, q); }
        else if (cmd == "perms") { string n; iss >> n; show_permissions(current, n); }
        else if (cmd == "chmod") { string n; int o; iss >> n >> o; change_permissions(current, n, o); }
        else if (cmd == "help") print_help();
        else if (cmd == "exit") break;
        else cout << "Unknown command. Type 'help'.\n";
    }

    cout << "Bye.\n";
    return 0;
}

