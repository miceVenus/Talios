#ifndef FCNTL_H
#define FCNTL_H

#define O_RONLY         000        // Open for reading only
#define O_WONLY         001        // Open for writing only
#define O_RDWR          002        // Open for reading and writing
#define O_ACCMODE       003        // Mask for access mode bits
#define O_CREAT         00100      // Create file if it does not exist
#define O_EXCL          00200      // Exclusive use flag (fail if file exists with O_CREAT)
#define O_NOCTTY        00400      // Do not assign controlling terminal
#define O_TRUNC         001000     // Truncate file to zero length
#define O_APPEND        002000     // Set append mode
#define O_NONBLOCK      004000     // Non-blocking mode
#define O_EXEC          0010000    // Open for execute only
#define O_SEARCH        0020000    // Open directory for search only
#define O_DIRECTORY     0040000    // Fail if not a directory
#define O_NOFOLLOW      00100000   // Do not follow symbolic links


extern int open (char * file_name, int oflag);


#endif