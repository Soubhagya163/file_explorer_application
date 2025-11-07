# 🗂️ Linux File Explorer (C++)

A simple console-based File Explorer built in **C++** for Linux systems.

## Features
- List files and directories
- Navigate through folders (`cd`, `ls`)
- Create, move, copy, delete files
- Search for files
- Manage file permissions
- Open files using `gedit` or `nano`

## How to Run
```bash
g++ -std=c++17 file_explorer.cpp -o file_explorer
./file_explorer

Commands

ls → list directory contents

cd <dir> → change directory

touch <file> → create file

open <file> → open file

mkdir <dir> → create directory

rm <file> → delete file

cp <src> <dest> → copy file

mv <src> <dest> → move file

search <name> → search files

chmod <perm> <file> → change permissions

exit → quit program
