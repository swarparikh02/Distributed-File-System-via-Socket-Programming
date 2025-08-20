# SocketDFS: Distributed File System via Socket Programming

## Overview
This project implements a distributed file system using socket programming in C, developed for the COMP-8567 Advanced Systems Programming course at the University of Windsor (Summer 2025). Clients connect to a primary server (S1) to perform file operations on .c, .pdf, .txt, and .zip files. S1 stores .c files locally and routes .pdf to S2, .txt to S3, and .zip to S4 transparently. Supports concurrent clients via forking, with commands for upload, download, delete, tar archiving (.c/.pdf/.txt only), and directory listing.

Key features from code:
- TCP sockets for inter-server and client-server communication.
- Recursive directory creation for file paths.
- Error handling for invalid commands, paths, and file types.
- Alphabetical sorting of file lists in `dispfnames`.

Learning outcomes applied:
- Applied OS concepts (forking, sockets) to solve systems programming problems in Unix/Linux.
- Designed solutions using kernel services for file I/O and networking.
- Handled informal specs to create formal implementations with documentation.

## Features
- **Upload (uploadf)**: Upload 1-3 files to S1, auto-distributed by type; creates folders if needed (e.g., `uploadf file.txt ~/S1/folder/`).
- **Download (downlf)**: Download 1-2 files from S1 (fetches from secondary servers transparently).
- **Remove (removef)**: Delete 1-2 files from the appropriate server.
- **Tar Download (downltar)**: Create and download tar of all files of a type (.c, .pdf, .txt) from the system (e.g., `downltar .pdf`); no support for .zip.
- **Display Names (dispfnames)**: List sorted file names by type in a directory, aggregated across servers (e.g., `dispfnames ~/S1/folder/`).
- Concurrent client handling via forking in S1.
- Client validates syntax before sending commands.

## Architecture
- **S1 (Primary Server)**: Listens on port 4221, forks per client, stores .c files in `/home/parikh37/S1`, routes others via sockets to S2-S4.
- **S2 (PDF Server)**: Port 4222, stores in `/home/parikh37/S2`, supports tar creation as `pdf.tar`.
- **S3 (TXT Server)**: Port 4223, stores in `/home/parikh37/S3`, supports tar as `text.tar`.
- **S4 (ZIP Server)**: Port 4224, stores in `/home/parikh37/S4`, no tar support.
- **Client (s25client)**: Connects to S1 at 127.0.0.1:4221, infinite loop for commands, handles local file I/O and validation.
- All components use 1024-byte buffers for data transfer.

## Setup Instructions
1. **Prerequisites**: Unix/Linux with GCC. No external libraries needed (uses standard C sockets, file I/O).
2. **Compilation** (in separate terminals for each):
gcc S1.c -o S1
gcc S2.c -o S2
gcc S3.c -o S3
gcc S4.c -o S4
gcc s25client.c -o s25client
3. **Running Servers** (start S2-S4 first, then S1):
./S2  # Listens on 4222
./S3  # Listens on 4223
./S4  # Listens on 4224
./S1  # Listens on 4221 for clients, connects to 4222-4224
Update directory paths in code if not using `/home/parikh37/`.
4. **Running Client**:./s25client

Enter commands at prompt; assumes localhost (edit S_IPADD in code for remote).

## Usage
- Run servers on separate terminals/machines.
- In client: Enter commands like `uploadf example.txt example.pdf ~/S1/test/`.
- Paths are relative to `~/S1` (adjusted internally).
- Exit client with Ctrl+C.

## Technologies
- **Language**: C
- **Networking**: TCP Sockets (AF_INET, SOCK_STREAM)
- **Concurrency**: Forking (in S1)
- **File Handling**: open/read/write, mkdir, tar via system calls, directory traversal with dirent.h

## Limitations
- Hardcoded ports/IP (localhost in client).
- Tar not supported for .zip (S4 returns -1).
- Buffer size fixed at 1024; no encryption.

## License
Educational project under MIT License. Fork and contribute!
