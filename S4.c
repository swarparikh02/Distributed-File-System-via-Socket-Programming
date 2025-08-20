#include <stdio.h> // Imports standard input/output functions for console and file operations
#include <stdlib.h> // Brings in utilities for memory management and program control
#include <arpa/inet.h> // Provides networking functions for socket-based communication
#include <fcntl.h> // Enables file control operations like opening and setting permissions
#include <dirent.h> // Allows directory operations, such as listing files
#include <errno.h> // Defines error codes for handling system call errors
#include <sys/stat.h> // Supplies functions for file status and directory creation
#include <string.h> // Includes tools for string operations like copying and comparing
#include <unistd.h> // Provides POSIX system calls for file and process management

#define SIZE_BUFFER 1024 // Sets a constant for the buffer size used in data transfers
#define S_PORT 4224 // Specifies the port number (4224) for the server to listen on
char DIRT_BASEP[] = "/home/parikh37/S4"; // Sets the base directory for storing and retrieving files

enum FilterCommd
{ // Defines an enumeration for client command types
    STORE_COMMD, // Represents the "store" command to save a file
    GETFILE_COMMD, // Represents the "retrieve" command to fetch a file
    DELETE_COMMD, // Represents the "delete" command to remove a file
    GETTAR_COMMD, // Represents the "maketar" command to create a tar archive
    PRINTFN_COMMD, // Represents the "list" command to list files in a directory
    INVALID_COMMD // Represents an unrecognized or invalid command
};

void func_makedir(char *t_p)
{ // Function to create directories recursively for a given path
    char tp_y[512]; // Creates a local buffer to hold a copy of the input path
    strcpy(tp_y, t_p); // Copies the input path into the local buffer
    for (char *t_p = tp_y + 1; *t_p; t_p++)
    { // Iterates through the path, skipping the first character
        if (*t_p == '/')
        { // Checks for a directory separator (slash)
            *t_p = 0; // Temporarily null-terminates the string to isolate a directory
            mkdir(tp_y, 0777); // Creates the directory with full permissions
            *t_p = '/'; // Restores the slash to continue processing
        }
    }
    mkdir(tp_y, 0777); // Creates the final directory in the path
}

enum FilterCommd func_commdtype(const char *comm_d)
{ // Function to determine the command type from a string
    if (strcmp(comm_d, "store") == 0)
    { // Checks if the command is "store"
        return STORE_COMMD; // Returns the store command enum value
    }
    if (strcmp(comm_d, "retrieve") == 0)
    { // Checks if the command is "retrieve"
        return GETFILE_COMMD; // Returns the retrieve command enum value
    }
    if (strcmp(comm_d, "delete") == 0)
    { // Checks if the command is "delete"
        return DELETE_COMMD; // Returns the delete command enum value
    }
    if (strcmp(comm_d, "maketar") == 0)
    { // Checks if the command is "maketar"
        return GETTAR_COMMD; // Returns the maketar command enum value
    }
    if (strcmp(comm_d, "list") == 0)
    { // Checks if the command is "list"
        return PRINTFN_COMMD; // Returns the list command enum value
    }
    return INVALID_COMMD; // Returns invalid command for unrecognized input
}

int main()
{ // Main function to run the server
    int s_fdt = socket(AF_INET, SOCK_STREAM, 0); // Creates a TCP socket for IPv4 communication
    if (s_fdt < 0)
    { // Checks if socket creation failed
        printf("Failure in socket..."); // Prints an error message for socket failure
        exit(EXIT_FAILURE); // Terminates the program with a failure status
    }
    struct sockaddr_in mys_addr; // Declares a structure to hold server address details
    mys_addr.sin_family = AF_INET; // Sets the address family to IPv4
    mys_addr.sin_port = htons(S_PORT); // Converts the port number to network byte order
    mys_addr.sin_addr.s_addr = INADDR_ANY; // Allows the server to accept connections on any interface
    if (bind(s_fdt, (struct sockaddr*)&mys_addr, sizeof(mys_addr)) < 0)
    { // Binds the socket to the address and port
        printf("Failure in bind execution..."); // Prints an error if binding fails
        close(s_fdt); // Closes the socket
        exit(EXIT_FAILURE); // Exits the program with a failure status
    }
    if (listen(s_fdt, 5) < 0)
    { // Sets the socket to listen for incoming connections, with a backlog of 5
        printf("Failure in listen execution...."); // Prints an error if listen fails
        close(s_fdt); // Closes the socket
        exit(EXIT_FAILURE); // Exits the program with a failure status
    }
    printf("Server-4 is ready it accepts only zip files...\n"); // Informs that the server is ready for zip file operations
    while (1)
    { // Enters an infinite loop to handle client connections
        int fd_file = accept(s_fdt, NULL, NULL); // Accepts a new client connection
        if (fd_file < 0) { // Checks if accepting the connection failed
            printf("Failure in accept execution..."); // Prints an error for accept failure
            continue; // Skips to the next iteration of the loop
        }
        int file_sz; // Declares a variable to store the size of the incoming command
        if (recv(fd_file, &file_sz, sizeof(int), 0) <= 0)
        { // Receives the command size from the client
            close(fd_file); // Closes the client socket if receive fails
            continue; // Skips to the next iteration
        }
        char comm_d[512]; // Buffer to store the command string
        if (recv(fd_file, comm_d, file_sz, 0) != file_sz)
        { // Receives the command string
            close(fd_file); // Closes the client socket if receive fails
            continue; // Skips to the next iteration
        }
        comm_d[file_sz - 1] = '\0'; // Null-terminates the command string
        char tp_y[512]; // Buffer to hold a copy of the command for tokenization
        strcpy(tp_y, comm_d); // Copies the command into the buffer
        char *get_tok = strtok(tp_y, " "); // Extracts the first token (command type)
        if (!get_tok) { // Checks if tokenization failed
            close(fd_file); // Closes the client socket
            continue; // Skips to the next iteration
        }
        enum FilterCommd typ_cmd = func_commdtype(get_tok); // Determines the command type
        switch (typ_cmd) { // Switches based on the command type
            case STORE_COMMD:
            { // Handles the "store" command
                char *file_r = strtok(NULL, ""); // Extracts the file path from the remaining command string
                if (!file_r)
                { // Checks if the file path is missing
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                long sz_file; // Declares a variable to store the file size
                if (recv(fd_file, &sz_file, sizeof(long), 0) <= 0)
                { // Receives the file size from the client
                    fprintf(stderr, "Failure to receive file size...\n"); // Prints an error if receive fails
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                if (sz_file < 0)
                { // Checks if the file size is invalid
                    fprintf(stderr, "It is Invalid File size..\n"); // Prints an error for invalid size
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                char t_fpath[512]; // Buffer to store the full file path
                snprintf(t_fpath, sizeof(t_fpath), "%s%s", DIRT_BASEP, file_r + 3); // Constructs the full path by appending to base directory
                char buf_dr[512]; // Buffer to store the directory path
                strcpy(buf_dr, t_fpath); // Copies the full path for directory extraction
                char *c_slh = strrchr(buf_dr, '/'); // Finds the last slash in the path
                if (c_slh) { // If a slash is found
                    *c_slh = '\0'; // Null-terminates to isolate the directory path
                    func_makedir(buf_dr); // Creates the necessary directories
                }
                int ffd = open(t_fpath, O_WRONLY | O_CREAT | O_TRUNC, 0666); // Opens the file for writing, creating it if needed
                if (ffd < 0)
                { // Checks if file opening failed
                    printf("Failure in opening file.."); // Prints an error message
                    char buf_r[SIZE_BUFFER]; // Buffer for receiving file data
                    long lt_byts = sz_file; // Tracks remaining bytes to receive
                    while (lt_byts > 0) { // Loops until all bytes are received
                        ssize_t nbyts = recv(fd_file, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk of data
                        if (nbyts <= 0) { // Checks if receive failed
                            break; // Exits the loop
                        }
                        lt_byts -= nbyts; // Updates remaining bytes
                    }
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                long lt_byts = sz_file; // Tracks remaining bytes to write
                char buf_r[SIZE_BUFFER]; // Buffer for receiving file data
                while (lt_byts > 0)
                { // Loops until all bytes are written
                    ssize_t nbyts = recv(fd_file, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receives a chunk of data
                    if (nbyts <= 0)
                    { // Checks if receive failed
                        fprintf(stderr, "Failure in recv execution %s\n", t_fpath); // Prints an error with the file path
                        break; // Exits the loop
                    }
                    ssize_t w_byts = write(ffd, buf_r, nbyts); // Writes the received data to the file
                    if (w_byts != nbyts)
                    { // Checks if writing failed
                        printf("Failure occur in writing..\n"); // Prints an error message
                        break; // Exits the loop
                    }
                    lt_byts -= nbyts; // Updates remaining bytes
                }
                close(ffd); // Closes the file
                close(fd_file); // Closes the client socket
                break; // Exits the case
            }
            case GETFILE_COMMD:
            { // Handles the "retrieve" command
                char *file_r = strtok(NULL, ""); // Extracts the file path from the command
                if (!file_r) { // Checks if the file path is missing
                    long sz_file = -1; // Sets an invalid file size
                    send(fd_file, &sz_file, sizeof(long), 0); // Sends the invalid size to the client
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                char t_fpath[512]; // Buffer to store the full file path
                snprintf(t_fpath, sizeof(t_fpath), "%s%s", DIRT_BASEP, file_r + 3); // Constructs the full path
                int ffd = open(t_fpath, O_RDONLY); // Opens the file for reading
                long sz_file = -1; // Initializes file size to invalid
                if (ffd >= 0) { // If the file was opened successfully
                    sz_file = lseek(ffd, 0, SEEK_END); // Gets the file size by seeking to the end
                    lseek(ffd, 0, SEEK_SET); // Resets the file pointer to the start
                } else 
                { // If file opening failed
                    printf("Failure in opening file...\n"); // Prints an error message
                }
                send(fd_file, &sz_file, sizeof(long), 0); // Sends the file size to the client
                if (sz_file < 0)
                { // Checks if the file size is invalid
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                char buf_r[SIZE_BUFFER]; // Buffer for reading file data
                ssize_t nbyts; // Tracks the number of bytes read
                while ((nbyts = read(ffd, buf_r, SIZE_BUFFER)) > 0)
                { // Reads the file in chunks
                    ssize_t st_f = send(fd_file, buf_r, nbyts, 0); // Sends the chunk to the client
                    if (st_f != nbyts) { // Checks if sending failed
                        fprintf(stderr, "Failure occurred in sending... %s\n", t_fpath); // Prints an error with the file path
                        break; // Exits the loop
                    }
                }
                if (nbyts < 0)
                { // Checks if reading failed
                    printf("Failure in reading...\n"); // Prints an error message
                }
                close(ffd); // Closes the file
                break; // Exits the case
            }
            case DELETE_COMMD: 
            { // Handles the "delete" command
                char *file_r = strtok(NULL, ""); // Extracts the file path from the command
                if (!file_r) { // Checks if the file path is missing
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                char t_fpath[512]; // Buffer to store the full file path
                snprintf(t_fpath, sizeof(t_fpath), "%s%s", DIRT_BASEP, file_r + 3); // Constructs the full path
                if (remove(t_fpath) != 0) 
                { // Attempts to delete the file
                    printf("Failure in removing file..."); // Prints an error if deletion fails
                }
                break; // Exits the case
            }
            case GETTAR_COMMD: 
            { // Handles the "maketar" command
                long sz_f = -1;//intilise file size for error to -1
                send(fd_file, &sz_f, sizeof(long), 0);//send the invalid filesize to client to indicate error
                break;//exits the case
            }
            case PRINTFN_COMMD: 
            { // Handles the "list" command
                char *pt_r = strtok(NULL, ""); // Extracts the directory path from the command
                if (!pt_r) { // Checks if the directory path is missing
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                char t_fpath[512]; // Buffer to store the full directory path
                snprintf(t_fpath, sizeof(t_fpath), "%s%s", DIRT_BASEP, pt_r + 3); // Constructs the full directory path
                DIR *d = opendir(t_fpath); // Opens the directory
                if (!d)
                { // Checks if opening the directory failed
                    printf("Failure in opening directory...."); // Prints an error message
                    long sz_lt = 0; // Sets an empty list size
                    send(fd_file, &sz_lt, sizeof(long), 0); // Sends the empty size to the client
                    close(fd_file); // Closes the client socket
                    break; // Exits the case
                }
                struct dirent *dt; // Declares a structure to hold directory entries
                char *lst_f[256]; // Array to store file names
                int ct = 0; // Counter for the number of files
                while ((dt = readdir(d)) != NULL && ct<256) 
                { // Reads each directory entry
                    if (dt->d_type != DT_REG) 
                    {
                        continue; // Skips non-regular files
                    }
                    lst_f[ct++] = strdup(dt->d_name); // Copies the file name into the array
                }
                closedir(d); // Closes the directory
                for (int k = 0; k < ct; k++) 
                { // Loops through the file list for sorting
                    for (int m = k + 1; m < ct; m++)
                    { // Compares each file with subsequent ones
                        if (strcmp(lst_f[k], lst_f[m]) > 0)
                        { // If the current file name is greater
                            char *tp = lst_f[k]; // Temporarily stores the current file name
                            lst_f[k] = lst_f[m]; // Swaps the file names
                            lst_f[m] = tp; // Completes the swap
                        }
                    }
                }
                char lst_buf[4096] = {0}; // Buffer to store the sorted file list
                for (int k = 0; k < ct; k++)
                { // Loops through the sorted files
                    strcat(lst_buf, lst_f[k]); // Appends the file name to the buffer
                    strcat(lst_buf, "\n"); // Adds a newline separator
                    free(lst_f[k]); // Frees the memory for the file name
                }
                long lst_size = strlen(lst_buf); // Gets the size of the file list string
                send(fd_file, &lst_size, sizeof(long), 0); // Sends the list size to the client
                send(fd_file, lst_buf, lst_size, 0); // Sends the file list to the client
                break; // Exits the case
            }
            case INVALID_COMMD: // Handles invalid commands
            default:
                fprintf(stderr, "It is invalid command found: %s\n", get_tok); // Prints an error with the invalid command
                break; // Exits the case
        }
        close(fd_file); // Closes the client socket after processing
    }
    close(s_fdt); // Closes the server socket (unreachable due to infinite loop)
    return 0; // Returns success (unreachable due to infinite loop)
}