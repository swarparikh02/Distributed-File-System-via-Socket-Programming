#include <stdio.h> // Imports standard input/output functions for console messages and file handling
#include <stdlib.h> // Provides utilities for memory allocation and program termination
#include <sys/types.h> // Defines data types used in system calls, like pid_t for process IDs
#include <string.h> // Includes functions for string operations like copying and comparing
#include <errno.h> // Supplies error codes for handling system call failures
#include <sys/socket.h> // Provides socket-related functions for network communication
#include <unistd.h> // Includes POSIX system calls for process and file operations
#include <arpa/inet.h> // Offers functions for internet address manipulation
#include <netinet/in.h> // Defines structures for internet domain addresses
#include <sys/stat.h> // Enables file status queries and directory creation
#include <fcntl.h> // Allows file control operations like opening files
#include <dirent.h> // Facilitates directory operations, such as listing files

#define S1_PORT 4221 // Sets the port number for Server 1 to listen on
#define S2_PORT 4222 // Defines the port number for Server 2 (handles PDF files)
#define S3_PORT 4223 // Specifies the port number for Server 3 (handles text files)
#define S4_PORT 4224 // Assigns the port number for Server 4 (handles zip files)
#define SIZE_BUFFER 1024 // Establishes a constant for the buffer size used in data transfers

typedef enum { // Defines an enumeration for client command types
    INVALID_COMMD = 0, // Represents an unrecognized or invalid command
    UF_COMMD, // Indicates the "uploadf" command for uploading files
    DF_COMMD, // Represents the "downlf" command for downloading files
    RF_COMMD, // Denotes the "removef" command for deleting files
    DLT_COMMD, // Specifies the "downltar" command for downloading tar archives
    DISF_COMMD // Indicates the "dispfnames" command for listing file names
} FilterCommd; // Names the enumeration type for command filtering

char DIRT_BASEP[] = "/home/parikh37/S1"; // Sets the base directory path for Server 1 file operations

FilterCommd func_commdtype(const char *comm_d) { // Function to identify the command type from a string
    if (strcmp(comm_d, "uploadf") == 0) { // Checks if the command is "uploadf"
        return UF_COMMD; // Returns the upload Jonloadf command enum value
    }
    if (strcmp(comm_d, "downlf") == 0) { // Checks if the command is "downlf"
        return DF_COMMD; // Returns the download command enum value
    }
    if (strcmp(comm_d, "removef") == 0) { // Checks if the command is "removef"
        return RF_COMMD; // Returns the remove command enum value
    }
    if (strcmp(comm_d, "downltar") == 0) { // Checks if the command is "downltar"
        return DLT_COMMD; // Returns the download tar command enum value
    }
    if (strcmp(comm_d, "dispfnames") == 0) { // Checks if the command is "dispfnames"
        return DISF_COMMD; // Returns the display filenames command enum value
    }
    return INVALID_COMMD; // Returns invalid command for unrecognized input
}

void func_makedir(const char *t_p) { // Function to create directories recursively for a given path
    char tp_y[512]; // Creates a buffer to hold a copy of the input path
    strncpy(tp_y, t_p, sizeof(tp_y)); // Copies the input path into the buffer
    tp_y[sizeof(tp_y)-1] = '\0'; // Ensures the buffer is null-terminated
    for (char *k = tp_y + 1; *k; k++) { // Iterates through the path, starting after the first character
        if (*k == '/') { // Checks for a directory separator (slash)
            *k = 0; // Temporarily null-terminates to isolate a directory
            if (mkdir(tp_y, 0777) == -1 && errno != EEXIST) { // Creates the directory with full permissions
                printf("Failure occurred in making directory...\n"); // Prints error if directory creation fails
            }
            *k = '/'; // Restores the slash to continue processing
        }
    }
    if (mkdir(tp_y, 0777) == -1 && errno != EEXIST) { // Creates the final directory in the path
        printf("Making directory failed completely..."); // Prints error if final directory creation fails
    }
}

int func_forsubconnection(int p_rt) { // Establishes a connection to a sub-server on a specified port
    int s_cket = socket(AF_INET, SOCK_STREAM, 0); // Creates a TCP socket for IPv4
    if (s_cket < 0) { // Checks if socket creation failed
        printf("Failure in creating socket...\n"); // Prints error for socket creation failure
        return -1; // Returns -1 to indicate failure
    }
    struct sockaddr_in myadd_serv = {0}; // Initializes a structure for server address
    myadd_serv.sin_family = AF_INET; // Sets address family to IPv4
    myadd_serv.sin_port = htons(p_rt); // Converts port number to network byte order
    inet_pton(AF_INET, "127.0.0.1", &myadd_serv.sin_addr); // Sets the server address to localhost
    if (connect(s_cket, (struct sockaddr *)&myadd_serv, sizeof(myadd_serv)) < 0) { // Connects to the sub-server
        printf("Failure in connection....\n"); // Prints error if connection fails
        close(s_cket); // Closes the socket
        return -1; // Returns -1 to indicate failure
    }
    return s_cket; // Returns the connected socket descriptor
}

void func_to_storelocal(const char *d, const char *namef, long sz_f, int fd_c)
{ // Stores a file locally on Server 1
    func_makedir(d); // Creates necessary directories for the file path
    char path_f[512]; // Buffer to store the full file path
    snprintf(path_f, sizeof(path_f), "%s/%s", d, namef); // Constructs the full file path
    int fdt = open(path_f, O_WRONLY | O_CREAT | O_TRUNC, 0666); // Opens the file for writing, creating if needed
    if (fdt < 0) { // Checks if file opening failed
        printf("Failure in opening file for writing...\n"); // Prints error for file opening failure
        char buf_r[SIZE_BUFFER]; // Buffer for receiving file data
        long lt_byts = sz_f; // Tracks remaining bytes to receive
        while (lt_byts > 0) { // Loops until all bytes are received
            ssize_t nbyts = recv(fd_c, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk of data
            if (nbyts <= 0) { // Checks if receive failed
                break; // Exits the loop
            }
            lt_byts -= nbyts; // Updates remaining bytes
        }
        return; // Exits the function
    }
    char buff_r[SIZE_BUFFER]; // Buffer for receiving file data
    long lt_byts = sz_f; // Tracks remaining bytes to write
    while (lt_byts > 0) { // Loops until all bytes are written
        ssize_t nbyts = recv(fd_c, buff_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk of data
        if (nbyts <= 0) { // Checks if receive failed
            fprintf(stderr, "Failure occurred while storing file %s\n", path_f); // Prints error with file path
            break; // Exits the loop
        }
        ssize_t w_byts = 0; // Tracks bytes written in the current chunk
        while (w_byts < nbyts) { // Ensures all received bytes are written
            ssize_t wb = write(fdt, buff_r + w_byts, nbyts - w_byts); // Writes data to the file
            if (wb < 0) { // Checks if writing failed
                printf("Failure occurred in writing... \n"); // Prints error for write failure
                break; // Exits the write loop
            }
            w_byts += wb; // Updates written bytes
        }
        lt_byts -= nbyts; // Updates remaining bytes
    }
    close(fdt); // Closes the file descriptor
}

void func_givefile_toserver(int p_rt, const char *d_path, const char *namef, long sz_f, int fd_c)
{ // Forwards a file to another server
    int s_cket = func_forsubconnection(p_rt); // Connects to the specified sub-server
    if (s_cket < 0) { // Checks if connection failed
        fprintf(stderr, "Failure in connecting to server to forward file....%d\n", p_rt); // Prints error with port number
        char buf_r[SIZE_BUFFER]; // Buffer for receiving file data
        long lt_byts = sz_f; // Tracks remaining bytes to receive
        while (lt_byts > 0) { // Discards incoming data if connection fails
            ssize_t nbyts = recv(fd_c, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk
            if (nbyts <= 0) break; // Exits if receive fails
            lt_byts -= nbyts; // Updates remaining bytes
        }
        return; // Exits the function
    }
    char comm_d[512]; // Buffer for the command to send
    snprintf(comm_d, sizeof(comm_d), "store ~S1%s/%s", d_path, namef); // Constructs the store command
    int l_cd = strlen(comm_d) + 1; // Gets the command length including null terminator
    if (send(s_cket, &l_cd, sizeof(int), 0) <= 0 || send(s_cket, comm_d, l_cd, 0) <= 0)
    { // Sends command length and command
        fprintf(stderr, "Failure in transferring command to port.. %d\n", p_rt); // Prints error if command send fails
        close(s_cket); // Closes the sub-server socket
        char buf_r[SIZE_BUFFER]; // Buffer for discarding data
        long lt_byts = sz_f; // Tracks remaining bytes
        while (lt_byts > 0) { // Discards incoming data
            ssize_t nbyts = recv(fd_c, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk
            if (nbyts <= 0) break; // Exits if receive fails
            lt_byts -= nbyts; // Updates remaining bytes
        }
        return; // Exits the function
    }
    if (send(s_cket, &sz_f, sizeof(long), 0) <= 0)
    { // Sends the file size to the sub-server
        fprintf(stderr, "Failure in transferring size of file to port.. %d\n", p_rt); // Prints error if size send fails
        close(s_cket); // Closes the sub-server socket
        char buf_r[SIZE_BUFFER]; // Buffer for discarding data
        long lt_byts = sz_f; // Tracks remaining bytes
        while (lt_byts > 0) { // Discards incoming data
            ssize_t nbyts = recv(fd_c, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk
            if (nbyts <= 0) break; // Exits if receive fails
            lt_byts -= nbyts; // Updates remaining bytes
        }
        return; // Exits the function
    }
    char buff_r[SIZE_BUFFER]; // Buffer for transferring file data
    long lt_byts = sz_f; // Tracks remaining bytes to forward
    while (lt_byts > 0) { // Loops until all bytes are forwarded
        ssize_t nbyts = recv(fd_c, buff_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk from client
        if (nbyts <= 0) { // Checks if receive failed
            fprintf(stderr, "Failure found in transferring file ahead.. %s\n", namef); // Prints error with file name
            break; // Exits the loop
        }
        ssize_t t_sct = 0; // Tracks bytes sent to sub-server
        while (t_sct < nbyts) { // Ensures all received bytes are sent
            ssize_t t1 = send(s_cket, buff_r + t_sct, nbyts - t_sct, 0); // Sends a chunk to sub-server
            if (t1 <= 0) { // Checks if send failed
                fprintf(stderr, "Failure in sending file..%s\n", namef); // Prints error with file name
                break; // Exits the send loop
            }
            t_sct += t1; // Updates sent bytes
        }
        lt_byts -= nbyts; // Updates remaining bytes
    }
    close(s_cket); // Closes the sub-server socket
}

void func_uploadfcmd(int fd_c, char t_args[][256], int ct)
{ // Handles the "uploadf" command
    if (ct < 2) { // Checks if there are enough arguments
        return; // Exits if arguments are insufficient
    }
    char *d_path = t_args[ct - 1]; // Gets the directory path from the last argument
    if (strncmp(d_path, "~S1", 3) == 0) d_path += 3; // Strips "~S1" prefix if present
    char r_pt[512]; // Buffer for the full directory path
    snprintf(r_pt, sizeof(r_pt), "%s%s", DIRT_BASEP, d_path); // Constructs the full directory path
    int ct_f = ct - 2; // Calculates the number of files to process
    for (int k = 1; k <= ct_f; k++) { // Loops through each file argument
        char namef[256]; // Buffer for the file name
        int f_length; // Variable for the file name length
        if (recv(fd_c, &f_length, sizeof(int), 0) <= 0) { // Receives the file name length
            fprintf(stderr, "Failed uploadf command: Failure to receive filename length...\n"); // Prints error if receive fails
            continue; // Skips to the next file
        }
        if (f_length <= 0 || f_length > 256) { // Checks if the file name length is valid
            fprintf(stderr, "Failed uploadf command: get invalid filename length\n"); // Prints error for invalid length
            char buf_r[SIZE_BUFFER]; // Buffer for discarding data
            long lt_byts = 1024; // Sets a default size for discarding
            while (lt_byts > 0) { // Discards incoming data
                ssize_t nbyts = recv(fd_c, buf_r, SIZE_BUFFER, 0); // Receives a chunk
                if (nbyts <= 0) break; // Exits if receive fails
                lt_byts -= nbyts; // Updates remaining bytes
            }
            continue; // Skips to the next file
        }
        if (recv(fd_c, namef, f_length, 0) <= 0) { // Receives the file name
            fprintf(stderr, "Failed uploadf command: Unable to get filename..\n"); // Prints error if receive fails
            continue; // Skips to the next file
        }
        namef[f_length - 1] = '\0'; // Null-terminates the file name
        long sz_f; // Variable for the file size
        if (recv(fd_c, &sz_f, sizeof(long), 0) <= 0) { // Receives the file size
            fprintf(stderr, "Failed uploadf command: unable to get size of file for %s\n", namef); // Prints error with file name
            continue; // Skips to the next file
        }
        if (sz_f < 0)
        { // Checks if the file size is valid
            fprintf(stderr, "Failed uploadf command: get size is invalid for file %s\n", namef); // Prints error for invalid size
            continue; // Skips to the next file
        }
        char *ex_tt = strrchr(namef, '.'); // Finds the file extension
        int f_typ = 0; // Initializes file type identifier
        if (ex_tt) { // If an extension is found
            if (strcmp(ex_tt, ".c") == 0) { // Checks for C file extension
                f_typ = 1; // Sets type to C file
            }
            else if (strcmp(ex_tt, ".pdf") == 0) { // Checks for PDF file extension
                f_typ = 2; // Sets type to PDF file
            }
            else if (strcmp(ex_tt, ".txt") == 0) { // Checks for text file extension
                f_typ = 3; // Sets type to text file
            }
            else if (strcmp(ex_tt, ".zip") == 0) { // Checks for zip file extension
                f_typ = 4; // Sets type to zip file
            }
        }
        switch (f_typ)
        { // Routes file based on its type
            case 1: // Handles C files
                func_to_storelocal(r_pt, namef, sz_f, fd_c); // Stores C file locally
                break;
            case 2: // Handles PDF files
                func_givefile_toserver(S2_PORT, d_path, namef, sz_f, fd_c); // Forwards to Server 2
                break;
            case 3: // Handles text files
                func_givefile_toserver(S3_PORT, d_path, namef, sz_f, fd_c); // Forwards to Server 3
                break;
            case 4: // Handles zip files
                func_givefile_toserver(S4_PORT, d_path, namef, sz_f, fd_c); // Forwards to Server 4
                break;
            default: // Handles unsupported file types
                fprintf(stderr, "Failed uploadf command: unsupported filetype for %s\n", namef); // Prints error for unsupported type
                char buf_r[SIZE_BUFFER]; // Buffer for discarding data
                long lt_byts = sz_f; // Tracks remaining bytes
                while (lt_byts > 0) { // Discards incoming data
                    ssize_t nbyts = recv(fd_c, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk
                    if (nbyts <= 0) break; // Exits if receive fails
                    lt_byts -= nbyts; // Updates remaining bytes
                }
                break;
        }
    }
}

void func_downlfcmd(char t_args[][256], int ct, int fd_c)
{ // Handles the "downlf" command
    int ct_f = ct - 1; // Calculates the number of files to download
    for (int k = 1; k <= ct_f; k++) { // Loops through each file argument
        char *file_p = t_args[k]; // Gets the file path
        if (strncmp(file_p, "~S1", 3) == 0) file_p += 3; // Strips "~S1" prefix if present
        char *ex_tt = strrchr(file_p, '.'); // Finds the file extension
        long sz_f = -1; // Initializes file size to invalid
        char t_fpath[512]; // Buffer for the full file path
        snprintf(t_fpath, sizeof(t_fpath), "%s%s", DIRT_BASEP, file_p); // Constructs the full file path
        int f_typ = 0; // Initializes file type identifier
        if (ex_tt) { // If an extension is found
            if (strcmp(ex_tt, ".c") == 0) { // Checks for C file extension
                f_typ = 1; // Sets type to C file
            }
            else if (strcmp(ex_tt, ".pdf") == 0) { // Checks for PDF file extension
                f_typ = 2; // Sets type to PDF file
            }
            else if (strcmp(ex_tt, ".txt") == 0) { // Checks for text file extension
                f_typ = 3; // Sets type to text file
            }
            else if (strcmp(ex_tt, ".zip") == 0) { // Checks for zip file extension
                f_typ = 4; // Sets type to zip file
            }
        }
        switch(f_typ)
        { // Routes file retrieval based on type
            case 1: { // Handles C files locally
                int fdt = open(t_fpath, O_RDONLY); // Opens the file for reading
                if (fdt < 0) { // Checks if file opening failed
                    printf("Failure occurred in opening....\n"); // Prints error for open failure
                    send(fd_c, &sz_f, sizeof(long), 0); // Sends invalid size to client
                    break; // Exits the case
                }
                sz_f = lseek(fdt, 0, SEEK_END); // Gets file size by seeking to end
                lseek(fdt, 0, SEEK_SET); // Resets file pointer to start
                send(fd_c, &sz_f, sizeof(long), 0); // Sends file size to client
                char buf_r[SIZE_BUFFER]; // Buffer for reading file data
                long lt_byts = sz_f; // Tracks remaining bytes to send
                while (lt_byts > 0) { // Loops until all bytes are sent
                    ssize_t nbyts = read(fdt, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER)); // Reads a chunk
                    if (nbyts <= 0) break; // Exits if read fails
                    ssize_t st = 0; // Tracks bytes sent in the chunk
                    while (st < nbyts) { // Ensures all read bytes are sent
                        ssize_t s1 = send(fd_c, buf_r + st, nbyts - st, 0); // Sends a chunk to client
                        if (s1 <= 0) break; // Exits if send fails
                        st += s1; // Updates sent bytes
                    }
                    lt_byts -= nbyts; // Updates remaining bytes
                }
                close(fdt); // Closes the file
                break; // Exits the case
            }
            case 2: // Handles PDF files
            case 3: // Handles text files
            case 4: { // Handles zip files
                int p_rt = 0; // Initializes port for sub-server
                switch(f_typ) { // Selects port based on file type
                    case 2: p_rt = S2_PORT; break; // PDF files go to Server 2
                    case 3: p_rt = S3_PORT; break; // Text files go to Server 3
                    case 4: p_rt = S4_PORT; break; // Zip files go to Server 4
                }
                int s_cket = func_forsubconnection(p_rt); // Connects to the sub-server
                if (s_cket < 0) { // Checks if connection failed
                    send(fd_c, &sz_f, sizeof(long), 0); // Sends invalid size to client
                    break; // Exits the case
                }
                char comm_d[512]; // Buffer for the retrieve command
                snprintf(comm_d, sizeof(comm_d), "retrieve ~S1%s", file_p); // Constructs the retrieve command
                int c_length = strlen(comm_d) + 1; // Gets command length
                send(s_cket, &c_length, sizeof(int), 0); // Sends command length to sub-server
                send(s_cket, comm_d, c_length, 0); // Sends command to sub-server
                long s_sz; // Variable for file size from sub-server
                if (recv(s_cket, &s_sz, sizeof(long), 0) <= 0) { // Receives file size
                    send(fd_c, &sz_f, sizeof(long), 0); // Sends invalid size to client
                    close(s_cket); // Closes sub-server socket
                    break; // Exits the case
                }
                send(fd_c, &s_sz, sizeof(long), 0); // Forwards file size to client
                if (s_sz < 0) { // Checks if file size is invalid
                    close(s_cket); // Closes sub-server socket
                    break; // Exits the case
                }
                char buf_r[SIZE_BUFFER]; // Buffer for transferring file data
                long lt_byts = s_sz; // Tracks remaining bytes
                while (lt_byts > 0) { // Loops until all bytes are forwarded
                    ssize_t nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk
                    if (nbyts <= 0) break; // Exits if receive fails
                    ssize_t st = 0; // Tracks bytes sent to client
                    while (st < nbyts) { // Ensures all received bytes are sent
                        ssize_t s1 = send(fd_c, buf_r + st, nbyts - st, 0); // Sends a chunk to client
                        if (s1 <= 0) break; // Exits if send fails
                        st += s1; // Updates sent bytes
                    }
                    lt_byts -= nbyts; // Updates remaining bytes
                }
                close(s_cket); // Closes sub-server socket
                break; // Exits the case
            }
            default: // Handles unsupported file types
                fprintf(stderr, "Failure in downlf command: unsupported filetype for %s\n", file_p); // Prints error
                send(fd_c, &sz_f, sizeof(long), 0); // Sends invalid size to client
                break; // Exits the case
        }
    }
}

void func_removefcmd(char t_args[][256], int ct, int fd_c)
{ // Handles the "removef" command
    int ct_f = ct - 1; // Calculates the number of files to delete
    for (int k = 1; k <= ct_f; k++) { // Loops through each file argument
        char *file_p = t_args[k]; // Gets the file path
        if (strncmp(file_p, "~S1", 3) == 0) file_p += 3; // Strips "~S1" prefix if present
        char *ex_tt = strrchr(file_p, '.'); // Finds the file extension
        char t_fpath[512]; // Buffer for the full file path
        snprintf(t_fpath, sizeof(t_fpath), "%s%s", DIRT_BASEP, file_p); // Constructs the full file path
        int f_typ = 0; // Initializes file type identifier
        if (ex_tt) { // If an extension is found
            if (strcmp(ex_tt, ".c") == 0)
            { // Checks for C file extension
                f_typ = 1; // Sets type to C file
            }
            else if (strcmp(ex_tt, ".pdf") == 0)
            { // Checks for PDF file extension
                f_typ = 2; // Sets type to PDF file
            }
            else if (strcmp(ex_tt, ".txt") == 0)
            { // Checks for text file extension
                f_typ = 3; // Sets type to text file
            }
            else if (strcmp(ex_tt, ".zip") == 0)
            { // Checks for zip file extension
                f_typ = 4; // Sets type to zip file
            }
        }
        switch(f_typ)
        { // Routes file deletion based on type
            case 1: // Handles C files locally
                if (remove(t_fpath) != 0)
                { // Attempts to delete the file
                    printf("Failure in removing file..\n"); // Prints error if deletion fails
                }
                break; // Exits the case
            case 2: // Handles PDF files
            case 3: // Handles text files
            case 4: { // Handles zip files
                int p_rt = 0; // Initializes port for sub-server
                switch(f_typ) { // Selects port based on file type
                    case 2: p_rt = S2_PORT; break; // PDF files go to Server 2
                    case 3: p_rt = S3_PORT; break; // Text files go to Server 3
                    case 4: p_rt = S4_PORT; break; // Zip files go to Server 4
                }
                int s_cket = func_forsubconnection(p_rt); // Connects to the sub-server
                if (s_cket < 0) { // Checks if connection failed
                    fprintf(stderr, "Failure occurred in sub connecting...\n"); // Prints error
                    break; // Exits the case
                }
                char comm_d[512]; // Buffer for the delete command
                snprintf(comm_d, sizeof(comm_d), "delete ~S1%s", file_p); // Constructs the delete command
                int c_length = strlen(comm_d) + 1; // Gets command length
                send(s_cket, &c_length, sizeof(int), 0); // Sends command length to sub-server
                send(s_cket, comm_d, c_length, 0); // Sends command to sub-server
                close(s_cket); // Closes sub-server socket
                break; // Exits the case
            }
            default: // Handles unsupported file types
                fprintf(stderr, "Failure in removef command: unsupported filetype for %s\n", file_p); // Prints error
                break; // Exits the case
        }
    }
}

void func_downltarcmd(char t_args[][256], int ct, int fd_c)
{ // Handles the "downltar" command
    if (ct < 2) { // Checks if there are enough arguments
        long sz = -1; // Sets invalid size
        send(fd_c, &sz, sizeof(long), 0); // Sends invalid size to client
        return; // Exits the function
    }
    char *f_typ = t_args[1]; // Gets the file type for the tar archive
    char namet[32]; // Buffer for the tar file name
    int p_rt = 0; // Initializes port for sub-server
    int loc_v = 0; // Flag for local processing
    if (strcmp(f_typ, ".c") == 0) { // Checks for C file type
        loc_v = 1; // Sets flag for local processing
        strcpy(namet, "cfiles.tar"); // Names the tar file for C files
    } else if (strcmp(f_typ, ".pdf") == 0) { // Checks for PDF file type
        p_rt = S2_PORT; // Sets port for Server 2
        strcpy(namet, "pdf.tar"); // Names the tar file for PDFs
    } else if (strcmp(f_typ, ".txt") == 0) { // Checks for text file type
        p_rt = S3_PORT; // Sets port for Server 3
        strcpy(namet, "text.tar"); // Names the tar file for text files
    } else { // Handles unsupported file types
        long sz = -1; // Sets invalid size
        send(fd_c, &sz, sizeof(long), 0); // Sends invalid size to client
        return; // Exits the function
    }
    char tar_p[512]; // Buffer for the tar file path
    snprintf(tar_p, sizeof(tar_p), "/tmp/%s", namet); // Constructs the tar file path
    long sz_f = -1; // Initializes tar file size
    if (loc_v) { // If processing locally (C files)
        char s_cmd[1024]; // Buffer for the tar command
        snprintf(s_cmd, sizeof(s_cmd), "cd %s && find . -type f -name '*.c' -print | tar -cf %s -T -", DIRT_BASEP, tar_p); // Builds command to create tar
        if (system(s_cmd) != 0) { // Executes the tar command
            fprintf(stderr, "Failure in creating tar for file.. %s\n", tar_p); // Prints error if tar creation fails
        }
        int fdt = open(tar_p, O_RDONLY); // Opens the tar file for reading
        if (fdt < 0) { // Checks if opening failed
            printf("Failure in opening tar file..\n"); // Prints error
            sz_f = -1; // Sets invalid size
            send(fd_c, &sz_f, sizeof(long), 0); // Sends invalid size to client
            return; // Exits the function
        }
        sz_f = lseek(fdt, 0, SEEK_END); // Gets tar file size
        lseek(fdt, 0, SEEK_SET); // Resets file pointer to start
        send(fd_c, &sz_f, sizeof(long), 0); // Sends file size to client
        if (sz_f < 0) { // Checks if size is invalid
            close(fdt); // Closes the tar file
            remove(tar_p); // Deletes the temporary tar file
            return; // Exits the function
        }
        char buf_r[SIZE_BUFFER]; // Buffer for reading tar data
        long lt_byts = sz_f; // Tracks remaining bytes
        while (lt_byts > 0) { // Loops until all bytes are sent
            ssize_t nbyts = read(fdt, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER)); // Reads a chunk
            if (nbyts <= 0) break; // Exits if read fails
            ssize_t st = 0; // Tracks bytes sent
            while (st < nbyts) { // Ensures all read bytes are sent
                ssize_t s1 = send(fd_c, buf_r + st, nbyts - st, 0); // Sends a chunk to client
                if (s1 <= 0) break; // Exits if send fails
                st += s1; // Updates sent bytes
            }
            lt_byts -= nbyts; // Updates remaining bytes
        }
        close(fdt); // Closes the tar file
        remove(tar_p); // Deletes the temporary tar file
    } else { // If forwarding to a sub-server
        int s_cket = func_forsubconnection(p_rt); // Connects to the sub-server
        if (s_cket < 0) { // Checks if connection failed
            long sz = -1; // Sets invalid size
            send(fd_c, &sz, sizeof(long), 0); // Sends invalid size to client
            return; // Exits the function
        }
        char comm_d[512] = "maketar"; // Sets the maketar command
        int c_length = strlen(comm_d) + 1; // Gets command length
        send(s_cket, &c_length, sizeof(int), 0); // Sends command length to sub-server
        send(s_cket, comm_d, c_length, 0); // Sends command to sub-server
        if (recv(s_cket, &sz_f, sizeof(long), 0) <= 0) { // Receives tar file size
            fprintf(stderr, "Failure occurred to receive tar filesize from port %d\n", p_rt); // Prints error
            send(fd_c, &sz_f, sizeof(long), 0); // Sends invalid size to client
            close(s_cket); // Closes sub-server socket
            return; // Exits the function
        }
        send(fd_c, &sz_f, sizeof(long), 0); // Forwards file size to client
        if (sz_f < 0) { // Checks if size is invalid
            close(s_cket); // Closes sub-server socket
            return; // Exits the function
        }
        char buf_r[SIZE_BUFFER]; // Buffer for transferring tar data
        long lt_byts = sz_f; // Tracks remaining bytes
        while (lt_byts > 0) { // Loops until all bytes are forwarded
            ssize_t nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk
            if (nbyts <= 0) { // Checks if receive fails
                fprintf(stderr, "Failure occurred in receiving tar data from port %d\n", p_rt); // Prints error
                break; // Exits the loop
            }
            ssize_t st = 0; // Tracks bytes sent to client
            while (st < nbyts) { // Ensures all received bytes are sent
                ssize_t s1 = send(fd_c, buf_r + st, nbyts - st, 0); // Sends a chunk to client
                if (s1 <= 0) break; // Exits if send fails
                st += s1; // Updates sent bytes
            }
            lt_byts -= nbyts; // Updates remaining bytes
        }
        close(s_cket); // Closes sub-server socket
    }
}

void func_dispfnamescmd(char t_args[][256], int ct, int fd_c)
{ // Handles the "dispfnames" command
    char lt_c[4096] = {0}; // Buffer for C file names
    char lt_pdf[4096] = {0}; // Buffer for PDF file names
    char lt_txt[4096] = {0}; // Buffer for text file names
    char lt_zip[4096] = {0}; // Buffer for zip file names
    char t_fd[512] = {0}; // Buffer for the directory path
    const char *namep = ""; // Initializes directory path
    if (ct >= 2) { // Checks if a directory path is provided
        namep = t_args[1]; // Gets the directory path
        if (strncmp(namep, "~S1", 3) == 0) namep += 3; // Strips "~S1" prefix if present
        snprintf(t_fd, sizeof(t_fd), "%s%s", DIRT_BASEP, namep); // Constructs the full directory path
    } else {
        snprintf(t_fd, sizeof(t_fd), "%s", DIRT_BASEP); // Uses base directory if no path provided
    }
    DIR *d = opendir(t_fd); // Opens the directory
    if (d) { // If directory opened successfully
        struct dirent *dt; // Structure for directory entries
        char *lt_f[256]; // Array to store file names
        int cntr = 0; // Counter for file names
        while ((dt = readdir(d)) != NULL && cntr < 256)
        { // Reads directory entries
            if (dt->d_type == DT_REG) { // Checks for regular files
                lt_f[cntr] = strdup(dt->d_name); // Copies file name to array
                cntr++; // Increments file counter
            }
        }
        closedir(d); // Closes the directory
        for (int k = 0; k < cntr; k++) { // Loops through file names for sorting
            for (int m = k + 1; m < cntr; m++) { // Compares each file with subsequent ones
                if (strcmp(lt_f[k], lt_f[m]) > 0) { // If current file name is greater
                    char *tp = lt_f[k]; // Temporarily stores current file name
                    lt_f[k] = lt_f[m]; // Swaps file names
                    lt_f[m] = tp; // Completes the swap
                }
            }
        }
        for (int k = 0; k < cntr; k++) { // Loops through sorted file names
            char *ex_tt = strrchr(lt_f[k], '.'); // Finds file extension
            if (ex_tt && strcmp(ex_tt, ".c") == 0) { // Checks for C file extension
                strncat(lt_c, lt_f[k], sizeof(lt_c) - strlen(lt_c) - 2); // Appends C file name
                strncat(lt_c, "\n", sizeof(lt_c) - strlen(lt_c) - 1); // Adds newline
            }
            free(lt_f[k]); // Frees memory for file name
        }
    }
    int p_rts[3] = {S2_PORT, S3_PORT, S4_PORT}; // Array of sub-server ports
    char *l_ts[3] = {lt_pdf, lt_txt, lt_zip}; // Array of buffers for file lists
    const size_t lt_sizes[3] = {sizeof(lt_pdf), sizeof(lt_txt), sizeof(lt_zip)}; // Array of buffer sizes
    for (int k = 0; k < 3; k++) { // Loops through sub-servers
        int s_cket = func_forsubconnection(p_rts[k]); // Connects to sub-server
        if (s_cket < 0) { // Checks if connection failed
            printf("Failure occurred to connect to sub-server on port %d\n", p_rts[k]); // Prints error
            continue; // Skips to next sub-server
        }
        char comm_d[512]; // Buffer for the list command
        snprintf(comm_d, sizeof(comm_d), "list ~S1%s", namep); // Constructs the list command
        int c_length = strlen(comm_d) + 1; // Gets command length
        send(s_cket, &c_length, sizeof(int), 0); // Sends command length to sub-server
        send(s_cket, comm_d, c_length, 0); // Sends command to sub-server
        long lt_s = 0; // Initializes list size
        if (recv(s_cket, &lt_s, sizeof(long), 0) <= 0) 
        { // Receives list size
            printf("Failure occurred to receive list size from port %d\n", p_rts[k]); // Prints error
            close(s_cket); // Closes sub-server socket
            continue; // Skips to next sub-server
        }
        if (lt_s > 0 && lt_s < (long)lt_sizes[k])
        { // Checks if list size is valid
            long lt_byts = lt_s; // Tracks remaining bytes
            char *r = l_ts[k]; // Pointer to current file list buffer
            while (lt_byts > 0) { // Loops until all list data is received
                int nbyts = recv(s_cket, r, lt_byts, 0); // Receives a chunk of list data
                if (nbyts <= 0) { // Checks if receive fails
                    printf("Error occurred in receiving list data from port %d\n", p_rts[k]); // Prints error
                    break; // Exits the loop
                }
                r += nbyts; // Advances buffer pointer
                lt_byts -= nbyts; // Updates remaining bytes
            }
            l_ts[k][lt_s] = '\0'; // Null-terminates the file list
        }
        close(s_cket); // Closes sub-server socket
    }
    char all_lt[16384] = {0}; // Buffer for combined file lists
    snprintf(all_lt, sizeof(all_lt), "%s%s%s%s", lt_c, lt_pdf, lt_txt, lt_zip); // Combines all file lists
    long all_s = strlen(all_lt); // Gets total size of combined list
    send(fd_c, &all_s, sizeof(long), 0); // Sends list size to client
    if (all_s > 0) { // Checks if there is data to send
        send(fd_c, all_lt, all_s, 0); // Sends combined file list to client
    }
}

void prcclient(int fd_c, struct sockaddr_in *c_addr)
{ // Processes client requests
    printf("Client connected..\n"); // Prints message for new client connection
    while (1) { // Loops to handle multiple client commands
        int length; // Variable for command length
        ssize_t rcv_d = recv(fd_c, &length, sizeof(int), 0); // Receives command length
        if (rcv_d <= 0) break; // Exits if receive fails or client disconnects
        if (length <= 0 || length > 512) { // Checks if command length is valid
            printf("Entered command has invalid length..\n"); // Prints error for invalid length
            break; // Exits the loop
        }
        char comm_d[512]; // Buffer for the command string
        rcv_d = recv(fd_c, comm_d, length, 0); // Receives the command
        if (rcv_d != length) break; // Exits if command receive fails
        comm_d[length - 1] = '\0'; // Null-terminates the command
        printf("Received command: %s\n", comm_d); // Prints the received command
        char tok_s[20][256]; // Array for tokenized command arguments
        int ct = 0; // Counter for number of tokens
        char *tok_en = strtok(comm_d, " "); // Tokenizes the command by spaces
        while (tok_en && ct < 20) { // Loops to collect tokens
            strncpy(tok_s[ct], tok_en, 255); // Copies token to array
            tok_s[ct][255] = '\0'; // Ensures token is null-terminated
            ct++; // Increments token counter
            tok_en = strtok(NULL, " "); // Gets next token
        }
        if (ct == 0) continue; // Skips if no tokens were found
        FilterCommd c_type = func_commdtype(tok_s[0]); // Determines command type
        switch (c_type) { // Routes command to appropriate handler
            case UF_COMMD: // Handles upload command
                func_uploadfcmd(fd_c, tok_s, ct); // Processes upload command
                break;
            case DF_COMMD: // Handles download command
                func_downlfcmd(tok_s, ct, fd_c); // Processes download command
                break;
            case RF_COMMD: // Handles remove command
                func_removefcmd(tok_s, ct, fd_c); // Processes remove command
                break;
            case DLT_COMMD: // Handles download tar command
                func_downltarcmd(tok_s, ct, fd_c); // Processes download tar command
                break;
            case DISF_COMMD: // Handles display filenames command
                func_dispfnamescmd(tok_s, ct, fd_c); // Processes display filenames command
                break;
            default: // Handles unknown commands
                printf("Unknown command entered: %s\n", tok_s[0]); // Prints error for unknown command
                break; // Exits the case
        }
    }
    close(fd_c); // Closes the client socket
}

int main()
{ // Main function to run the server
    int l_fd = socket(AF_INET, SOCK_STREAM, 0); // Creates a TCP socket for Server 1
    if (l_fd < 0) 
    { // Checks if socket creation failed
        printf("Error occurred in listening socket...."); // Prints error
        return 1; // Exits with failure status
    }
    struct sockaddr_in myadd_serv = {0}; // Initializes server address structure
    myadd_serv.sin_family = AF_INET; // Sets address family to IPv4
    myadd_serv.sin_port = htons(S1_PORT); // Sets port to listen on
    myadd_serv.sin_addr.s_addr = INADDR_ANY; // Allows connections on any interface
    if (bind(l_fd, (struct sockaddr *)&myadd_serv, sizeof(myadd_serv)) < 0)
    { // Binds socket to address and port
        printf("Error occurred in bind connection"); // Prints error if bind fails
        close(l_fd); // Closes the socket
        return 1; // Exits with failure status
    }
    if (listen(l_fd, 5) < 0) { // Sets socket to listen with a backlog of 5
        printf("Error occurred in listen connection"); // Prints error if listen fails
        close(l_fd); // Closes the socket
        return 1; // Exits with failure status
    }
    printf("Server1 listening on port %d\n", S1_PORT); // Prints message that server is ready
    while (1) { // Enters infinite loop to accept client connections
        struct sockaddr_in c_addr; // Structure for client address
        socklen_t c_length = sizeof(c_addr); // Size of client address structure
        int fd_c = accept(l_fd, (struct sockaddr *)&c_addr, &c_length); // Accepts a client connection
        if (fd_c < 0)
        { // Checks if accept failed
            printf("Error occurred in accept connection"); // Prints error
            continue; // Skips to next iteration
        }
        if (!fork())
        { // Forks a child process to handle the client
            close(l_fd); // Closes listening socket in child
            prcclient(fd_c, &c_addr); // Processes client requests
            exit(0); // Exits child process
        }
        close(fd_c); // Closes client socket in parent
    }
    close(l_fd); // Closes listening socket (unreachable due to infinite loop)
    return 0; // Returns success (unreachable due to infinite loop)
}