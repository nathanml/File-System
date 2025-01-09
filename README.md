# File-System
A file system implemented in C.

## Instructions
From the root directory, run the following command in the command line:
```
make
./command_line
```
This will open a shell that will allow you to execute file system commands.

If an error occurs when running, the virtual disk may be full. Run this 
command to clear the disk.
```
make clean
./command_line
```
The following file system commands can be executed with this program.
```
jfs_mkdir: Create a directory. Dirname sole argument.
jfs_chdir: Change directory. Dirname sole argument.
jfs_ls: List contents of current directory
jfs_rmdir: Remove directory. Dirnamae sole argument.
jfs_creat: Create a new file. Filename sole argument.
jfs_remove: Delete a file. Filename sole argument.
jfs_stat: Retrieve information about file or directory. File or dirname is only argument.
jfs_write: Write to a file. First argument filename. Second argument content to write.
jfs_read: Reads from file. First argument filename.
```
