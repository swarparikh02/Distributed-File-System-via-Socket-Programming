#include <stdio.h> // Include standard input/output library for functions like printf
#include <arpa/inet.h> // Include networking library for socket-related functions
#include <sys/stat.h> // Include library for file status functions like access
#include <stdlib.h> // Include standard library for memory allocation and exit
#include <unistd.h> // Include library for POSIX system calls like close and read
#include <fcntl.h> // Include library for file control options like open
#include <string.h> // Include library for string manipulation functions
#include <errno.h> // Include library for error handling

#define S_PORT 4221 // Define the server port number for connection
#define SIZE_BUFFER 1024 // Define the buffer size for reading/writing data
#define S_IPADD "127.0.0.1" // Define the server IP address (localhost)

enum
{ // Define an enumeration for command types
    INVALID_COMMD = 0, // Invalid command
    UF_COMMD, // Upload file command
    DF_COMMD, // Download file command
    RF_COMMD, // Remove file command
    DLT_COMMD, // Download tar file command
    DISF_COMMD // Display filenames command
};

int dest_path_check(const char *track_path) // Function to check if destination path is valid
{
    char store_cpypath[512]; // Create a buffer to store a copy of the path
    strncpy(store_cpypath, track_path, sizeof(store_cpypath)-1); // Copy the path safely
    store_cpypath[sizeof(store_cpypath)-1] = '\0'; // Ensure null termination

    char *slh_c = strrchr(store_cpypath, '/'); // Find the last '/' in the path
    if (slh_c) // If a '/' is found
    {
        *slh_c = '\0'; // Terminate the string at the last '/' to get directory path
    } else // If no '/' is found
    {
        return 1; // Return 1, assuming the path is in the current directory
    }

    if (access(store_cpypath, W_OK) != 0) // Check if the directory is writable
    {
        return 0; // Return 0 if the directory is not writable
    }
    return 1; // Return 1 if the directory is writable
}

void func_togettarfile(int s_cket, const char *name_tr, long s_file) // Function to receive and save a tar file
{
    if (!dest_path_check(name_tr)) // Check if the destination path is valid
    {
        printf("Path doesn't found for this tarname: %s\n", name_tr); // Print error if path is invalid
        char buf_r[SIZE_BUFFER]; // Buffer for receiving data
        long lt_byts = s_file; // Track remaining bytes to receive
        while (lt_byts > 0) // Loop until all bytes are received
        {
            int nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receive data
            if (nbyts <= 0) break; // Break if no more data or error
            lt_byts -= nbyts; // Update remaining bytes
        }
        return; // Exit function if path is invalid
    }
    int file_d = open(name_tr, O_WRONLY | O_CREAT | O_TRUNC, 0666); // Open file for writing, create if not exists
    if (file_d < 0) // Check if file opening failed
    {
        printf("Failure in creating tar of file...\n"); // Print error message
        long lt_byts = s_file; // Track remaining bytes
        char buf_r[SIZE_BUFFER]; // Buffer for receiving data
        while (lt_byts > 0) // Loop to discard incoming data
        {
            int nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receive data
            if (nbyts <= 0) break; // Break if no more data or error
            lt_byts -= nbyts; // Update remaining bytes
        }
        return; // Exit function if file creation failed
    }
    char buf_r[SIZE_BUFFER]; // Buffer for receiving file data
    long lt_byts = s_file; // Track remaining bytes to receive
    int get_byts = 0; // Track total bytes received
    while (lt_byts > 0) // Loop until all bytes are received
    {
        int nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receive data
        if (nbyts <= 0) // Check if receiving failed
        {
            printf("Failure in getting tar file data for tarname: %s, to get %d bytes\n", name_tr, get_byts); // Print error
            break; // Exit loop on error
        }
        if (write(file_d, buf_r, nbyts) != nbyts) // Write received data to file
        {
            printf("Failure in writing tar file of tarname: %s, added %d bytes\n", name_tr, get_byts); // Print error
            break; // Exit loop on write failure
        }
        get_byts += nbyts; // Update total bytes received
        lt_byts -= nbyts; // Update remaining bytes
    }
    close(file_d); // Close the file
    if (lt_byts == 0) // Check if all bytes were received
    {
        printf("Successfully downloaded file %s with (%d get bytes)\n", name_tr, get_byts); // Print success message
    } else // If download was incomplete
    {
        printf("Incomplete download of  %s, still expected %ld bytes, get only this %d bytes\n", name_tr, s_file, get_byts); // Print error
    }
}

void func_tosendfile(int s_cket, const char *namef) // Function to send a file to the server
{
    int file_d = open(namef, O_RDONLY); // Open file for reading
    if (file_d < 0) // Check if file opening failed
    {
        printf("Error occurred in opening file....\n"); // Print error message
        return; // Exit function
    }
    const char *name_basep = strrchr(namef, '/') ? strrchr(namef, '/') + 1 : namef; // Get the base filename
    int lname_f = strlen(name_basep) + 1; // Calculate length of filename including null terminator
    if (send(s_cket, &lname_f, sizeof(int), 0) <= 0) // Send filename length
    {
        printf("Failure in sending length of filename for this basename: %s\n", name_basep); // Print error
        close(file_d); // Close file
        return; // Exit function
    }
    if (send(s_cket, name_basep, lname_f, 0) <= 0) // Send filename
    {
        printf("Failure in sending name of file for this basename: %s\n", name_basep); // Print error
        close(file_d); // Close file
        return; // Exit function
    }
    off_t s_file = lseek(file_d, 0, SEEK_END); // Get file size
    lseek(file_d, 0, SEEK_SET); // Reset file pointer to start
    if (send(s_cket, &s_file, sizeof(off_t), 0) <= 0) // Send file size
    {
        printf("Failure in sending size of file for this basename: %s\n", name_basep); // Print error
        close(file_d); // Close file
        return; // Exit function
    }
    char buf_r[SIZE_BUFFER]; // Buffer for reading file data
    ssize_t nbyts; // Variable to store number of bytes read
    while ((nbyts = read(file_d, buf_r, SIZE_BUFFER)) > 0) // Read file in chunks
    {
        if (send(s_cket, buf_r, nbyts, 0) != nbyts) // Send chunk to server
        {
            printf("Failure in sending file for this basename: %s\n", name_basep); // Print error
            break; // Exit loop on send failure
        }
    }
    if (nbyts < 0) // Check if reading file failed
    {
        printf("Failure in reading file for this basename: %s\n", name_basep); // Print error
    }
    close(file_d); // Close file
}

void func_togetandprint_filenames(int s_cket, long s_byts) // Function to receive and print filenames
{
    char *buf_r = malloc(s_byts + 1); // Allocate buffer for receiving filenames
    if (!buf_r) // Check if memory allocation failed
    {
        printf("Failure occurred in allocating memory...\n"); // Print error
        char buff_r[SIZE_BUFFER]; // Temporary buffer for discarding data
        long lt_byts = s_byts; // Track remaining bytes
        while (lt_byts > 0) // Loop to discard incoming data
        {
            int nbyts = recv(s_cket, buff_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receive data
            if (nbyts <= 0) break; // Break if no more data or error
            lt_byts -= nbyts; // Update remaining bytes
        }
        return; // Exit function
    }
    long lt_byts = s_byts; // Track remaining bytes to receive
    char *t_p = buf_r; // Pointer to current position in buffer
    while (lt_byts > 0) // Loop until all bytes are received
    {
        int nbyts = recv(s_cket, t_p, lt_byts, 0); // Receive data
        if (nbyts <= 0) // Check if receiving failed
        {
            printf("Failure in getting data...\n"); // Print error
            break; // Exit loop
        }
        t_p += nbyts; // Move pointer forward
        lt_byts -= nbyts; // Update remaining bytes
    }
    buf_r[s_byts] = '\0'; // Null-terminate the buffer
    printf("%s", buf_r); // Print received filenames
    free(buf_r); // Free allocated memory
}

void func_togetfile(int s_cket, const char *name_basep, long s_file) // Function to receive and save a file
{
    if (!dest_path_check(name_basep)) // Check if destination path is valid
    {
        printf("Path found doesn't exist: %s\n", name_basep); // Print error if path is invalid
        char buf_r[SIZE_BUFFER]; // Buffer for discarding data
        long lt_byts = s_file; // Track remaining bytes
        while (lt_byts > 0) // Loop to discard incoming data
        {
            int nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receive data
            if (nbyts <= 0) break; // Break if no more data or error
            lt_byts -= nbyts; // Update remaining bytes
        }
        return; // Exit function
    }
    int file_d = open(name_basep, O_WRONLY | O_CREAT | O_TRUNC, 0666); // Open file for writing
    if (file_d < 0) // Check if file opening failed
    {
        printf("Error in creating file...\n"); // Print error
        long lt_byts = s_file; // Track remaining bytes
        char buf_r[SIZE_BUFFER]; // Buffer for discarding data
        while (lt_byts > 0) // Loop to discard incoming data
        {
            int nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receive data
            if (nbyts <= 0) break; // Break if no more data or error
            lt_byts -= nbyts; // Update remaining bytes
        }
        return; // Exit function
    }
    char buf_r[SIZE_BUFFER]; // Buffer for receiving file data
    long lt_byts= s_file; // Track remaining bytes
    while (lt_byts > 0) // Loop until all bytes are received
    {
        int nbyts = recv(s_cket, buf_r, (lt_byts < SIZE_BUFFER ? lt_byts : SIZE_BUFFER), 0); // Receive data
        if (nbyts <= 0) // Check if receiving failed
        {
            printf("Failure in getting data of file for this file name: %s\n", name_basep); // Print error
            break; // Exit loop
        }
        if (write(file_d, buf_r, nbyts) != nbyts) // Write received data to file
        {
            printf("Failure in writing file for this file name: %s\n", name_basep); // Print error
            break; // Exit loop
        }
        lt_byts -= nbyts; // Update remaining bytes
    }
    close(file_d); // Close the file
    printf("File is downloaded for this file name: %s\n", name_basep); // Print success message
}

int main() // Main function
{
    int s_cket = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (s_cket < 0) // Check if socket creation failed
    {
        printf("Failure in creating socket....\n"); // Print error
        exit(1); // Exit program
    }
    struct sockaddr_in myadd_serv; // Structure for server address
    myadd_serv.sin_family = AF_INET; // Set address family to IPv4
    myadd_serv.sin_port = htons(S_PORT); // Set port number (convert to network byte order)
    inet_pton(AF_INET, S_IPADD, &myadd_serv.sin_addr); // Convert IP address to binary form
    if (connect(s_cket, (struct sockaddr*)&myadd_serv, sizeof(myadd_serv)) < 0) // Connect to server
    {
        printf("Failure in socket connection...\n"); // Print error
        close(s_cket); // Close socket
        exit(1); // Exit program
    }
    char tk_lne[512]; // Buffer for user input
    while (1) // Main loop for user interaction
    {
        printf("s25client$ "); // Print command prompt
        fflush(stdout); // Flush output buffer
        if (!fgets(tk_lne, sizeof(tk_lne), stdin)) // Read user input
        {
            break; // Exit loop on EOF
        }
        tk_lne[strcspn(tk_lne, "\n")] = 0; // Remove newline from input
        if (strlen(tk_lne) == 0) // Skip empty input
        {
            continue; // Continue to next iteration
        }
        char tp_y[512]; // Buffer for tokenizing input
        strcpy(tp_y, tk_lne); // Copy input for tokenizing
        char *get_tokn = strtok(tp_y, " "); // Get first token (command)
        char *comm_d = get_tokn; // Store command
        int c_argt = 0; // Argument count
        char *t_argt[10]; // Array to store arguments
        t_argt[c_argt++] = get_tokn; // Store command as first argument
        while ((get_tokn = strtok(NULL, " "))) // Get remaining tokens
        {
            t_argt[c_argt++] = get_tokn; // Store each argument
        }
        int l_cmmd = strlen(tk_lne) + 1; // Calculate command length including null terminator
        if (send(s_cket, &l_cmmd, sizeof(int), 0) <= 0 || send(s_cket, tk_lne, l_cmmd, 0) <= 0) // Send command length and command
        {
            printf("Failure in passing entered Command: %s\n", tk_lne); // Print error
            break; // Exit loop
        }
        int type_commd = INVALID_COMMD; // Initialize command type
        if (strcmp(comm_d, "uploadf") == 0) // Check for upload command
        {
            type_commd = UF_COMMD; // Set command type
        }
        else if (strcmp(comm_d, "downlf") == 0) // Check for download command
        {
            type_commd = DF_COMMD; // Set command type
        }
        else if (strcmp(comm_d, "removef") == 0) // Check for remove file command
        {
            type_commd = RF_COMMD; // Set command type
        }
        else if (strcmp(comm_d, "downltar") == 0) // Check for download tar command
        {
            type_commd = DLT_COMMD; // Set command type
        }
        else if (strcmp(comm_d, "dispfnames") == 0) // Check for display filenames command
        {
            type_commd = DISF_COMMD; // Set command type
        }
        switch (type_commd) // Process command based on type
        {
            case UF_COMMD: // Handle upload file command
            {
                int ct_file = c_argt - 2; // Calculate number of files to upload
                if (ct_file < 1 || ct_file > 3) // Check if file count is valid
                {
                    printf("In entered command you have entered invalid accepted file number,  you can enter upto 3 files maximum to upload...\n"); // Print error
                    continue; // Skip to next iteration
                }
                char *p_dest = t_argt[c_argt - 1]; // Get destination path
                if (strncmp(p_dest, "~S1", 3) != 0) // Check if destination path starts with ~S1
                {
                    printf("Found wrong path always destination path must start with ~S1 for correct working of command execution...\n"); // Print error
                    continue; // Skip to next iteration
                }
                for (int k = 1; k < c_argt - 1; k++) // Loop through files to upload
                {
                    if (access(t_argt[k], R_OK) != 0) // Check if file is readable
                    {
                        printf("Unable to find file %s or it is not readable...\n", t_argt[k]); // Print error
                        continue; // Skip to next file
                    }
                    func_tosendfile(s_cket, t_argt[k]); // Send file to server
                }
                fsync(s_cket); // Ensure all data is sent
                printf("Successful execution of command is done..\n"); // Print success message
                break; // Exit switch
            }
            case DF_COMMD: // Handle download file command
            {
                int ct_file = c_argt - 1; // Calculate number of files to download
                if (ct_file < 1 || ct_file > 2) // Check if file count is valid
                {
                    printf("In entered command you have entered invalid accepted file number,  you can enter upto 2 files maximum to download...\n"); // Print error
                    continue; // Skip to next iteration
                }
                for (int k = 0; k < ct_file; k++) // Loop through files to download
                {
                    char *p_file = t_argt[k + 1]; // Get file path
                    char *name_basep = strrchr(p_file, '/') ? strrchr(p_file, '/') + 1 : p_file; // Get base filename
                    long s_file; // Variable for file size
                    if (recv(s_cket, &s_file, sizeof(long), 0) <= 0) // Receive file size
                    {
                        printf("Failure in getting size of file %s\n", name_basep); // Print error
                        continue; // Skip to next file
                    }
                    printf("Get filesize for %s: %ld\n", name_basep, s_file); // Print file size
                    if (s_file < 0) // Check if file exists
                    {
                        printf("File not found: %s\n", name_basep); // Print error
                        continue; // Skip to next file
                    }
                    func_togetfile(s_cket, name_basep, s_file); // Download file
                }
                printf("Successful execution of command is done..\n"); // Print success message
                break; // Exit switch
            }
            case RF_COMMD: // Handle remove file command
            {
                int ct_file = c_argt - 1; // Calculate number of files to remove
                if (ct_file < 1 || ct_file > 2) // Check if file count is valid
                {
                    printf("In entered command you have entered invalid accepted file number,  you can enter upto 2 files maximum to remove file...\n"); // Print error
                    continue; // Skip to next iteration
                }
                printf("Successful execution of command is done..\n"); // Print success message
                break; // Exit switch
            }
            case DLT_COMMD: // Handle download tar command
            {
                if (c_argt != 2) // Check if correct number of arguments
                {
                    printf("You have entered invalid syntax: usage downltar <filetype> (i.e .c,.pdf,.txt)\n"); // Print error
                    continue; // Skip to next iteration
                }
                char *ty_file = t_argt[1]; // Get file type
                char name_tr[32]; // Buffer for tar filename
                if (strcmp(ty_file, ".c") == 0) // Check if file type is .c
                {
                    strcpy(name_tr, "cfiles.tar"); // Set tar filename for .c files
                }
                else if (strcmp(ty_file, ".pdf") == 0) // Check if file type is .pdf
                {
                    strcpy(name_tr, "pdf.tar"); // Set tar filename for .pdf files
                }
                else if (strcmp(ty_file, ".txt") == 0) // Check if file type is .txt
                {
                    strcpy(name_tr, "text.tar"); // Set tar filename for .txt files
                }
                else // If file type is unsupported
                {
                    printf("It encountered unsupported filetype: %s\n", ty_file); // Print error
                    continue; // Skip to next iteration
                }
                long s_file; // Variable for tar file size
                if (recv(s_cket, &s_file, sizeof(long), 0) <= 0) // Receive tar file size
                {
                    printf("Failure in getting size of tar file for %s\n", name_tr); // Print error
                    continue; // Skip to next iteration
                }
                printf("Get size of tar file for %s: %ld\n", name_tr, s_file); // Print tar file size
                if (s_file < 0) // Check if tar file exists
                {
                    printf("Unable to get tar file for %s\n", ty_file); // Print error
                    continue; // Skip to next iteration
                }
                func_togettarfile(s_cket, name_tr, s_file); // Download tar file
                printf("Successful execution of command is done..\n"); // Print success message
                break; // Exit switch
            }
            case DISF_COMMD: // Handle display filenames command
            {
                if (c_argt != 2) // Check if correct number of arguments
                {
                    printf("You entered is invalid syntax you need to enter dispfnames path(~S1/folder1/folder2)\n"); // Print error
                    continue; // Skip to next iteration
                }
                long s_z; // Variable for size of filenames data
                if (recv(s_cket, &s_z, sizeof(long), 0) <= 0) // Receive size of filenames
                {
                    printf("Failure in getting filenames..\n"); // Print error
                    continue; // Skip to next iteration
                }
                if (s_z < 0) // Check if size is valid
                {
                    printf("Failure in executing as size is less then 0\n"); // Print error
                    continue; // Skip to next iteration
                }
                func_togetandprint_filenames(s_cket, s_z); // Receive and print filenames
                printf("Successful execution of command is done..\n"); // Print success message
                break; // Exit switch
            }
            default: // Handle invalid command
                printf("You have entered invalid command\n"); // Print error
                break; // Exit switch
        }
    }
    close(s_cket); // Close socket
    return 0; // Exit program
} 