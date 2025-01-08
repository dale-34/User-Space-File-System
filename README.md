File Systems

SYNOPSIS
Implements a userspace filesystem daemon using FUSE API to access data in WAD format.  

DAEMON Commands
- .getattr: Calls isDirectory or isContent and defines the necessary info for stat struct. 
- .mknod: Calls createFile
- .mkdir: Calls createDirectory
- .read: Calls getContents
- .write: Calls writeToFile
- .readdir: Calls getDirectory. Iterates through vector to fill buffer. 
