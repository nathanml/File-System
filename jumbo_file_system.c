#include "jumbo_file_system.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "math.h"

// C does not have a bool type, so I created one that you can use
typedef char bool_t;
#define TRUE 1
#define FALSE 0

static block_num_t current_dir;
struct block_num_t *allocatedBlockNums[MAX_DATA_BLOCKS];


// optional helper function you can implement to tell you if a block is a dir node or an inode
static bool_t is_dir(block_num_t block_num) {
    struct block tmp;
    if(read_block(block_num, &tmp)==-1)
        perror("Block Read Error");
  return tmp.is_dir;
}



/* jfs_mount
 *   prepares the DISK file on the _real_ file system to have file system
 *   blocks read and written to it.  The application _must_ call this function
 *   exactly once before calling any other jfs_* functions.  If your code
 *   requires any additional one-time initialization before any other jfs_*
 *   functions are called, you can add it here.
 * filename - the name of the DISK file on the _real_ file system
 * returns 0 -1 on error; errors should only occur due to
 *   errors in the underlying disk syscalls.
 */
int jfs_mount(const char* filename) {
  int ret = bfs_mount(filename);
  current_dir = 1;
  //Read superblock
  struct block root;
  if(read_block(current_dir, &root)==-1)
    perror("Error Reading Block");
  root.is_dir = TRUE;
  root.contents.dirnode.num_entries=0;
  if(write_block(current_dir, &root)==-1)
      perror("Error Writing Block");

  return ret;
}


/* jfs_mkdir
 *   creates a new subdirectory in the current directory
 * directory_name - name of the new subdirectory
 * returns 0 on success or one of the following error codes on failure:
 *   E_EXISTS, E_MAX_NAME_LENGTH, E_MAX_DIR_ENTRIES, E_DISK_FULL
 */
int jfs_mkdir(const char* directory_name) {
    struct block currentBlock;
    if(read_block(current_dir, &currentBlock) ==-1)
        perror("Error Reading Block");
    //If directory contains too many directory entries
    if(currentBlock.contents.dirnode.num_entries==MAX_DIR_ENTRIES)
        return E_MAX_DIR_ENTRIES;
    //If name is too long
    if(strlen(directory_name) > MAX_NAME_LENGTH)
        return E_MAX_NAME_LENGTH;
    //Iterate through current directory contents
    for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
    {
        //If existing directory block name matches name
        if(strcmp(currentBlock.contents.dirnode.entries[i].name,directory_name)==0)
            return E_EXISTS;
    }

    //Allocate block for directory on disk
    block_num_t newBlockNum = allocate_block();
    if((int) newBlockNum == 0) {
        newBlockNum = allocate_block();
        if(newBlockNum==0)
            return E_DISK_FULL;
    }
    struct block newBlock;
    if(read_block(newBlockNum, &newBlock)==-1)
        perror("Error Reading Block");
    newBlock.is_dir = TRUE;
    newBlock.contents.dirnode.num_entries = 0;

    //Set directory block as child of current directory block
    for(int i=0; i< MAX_NAME_LENGTH; i++)
    {
        if(i>=strlen(directory_name))
            currentBlock.contents.dirnode.entries[currentBlock.contents.dirnode.num_entries].name[i] = '\0';
        else currentBlock.contents.dirnode.entries[currentBlock.contents.dirnode.num_entries].name[i] = directory_name[i];
    }
    currentBlock.contents.dirnode.entries[currentBlock.contents.dirnode.num_entries].block_num = newBlockNum;
    //Increment num directories in currentDir
    currentBlock.contents.dirnode.num_entries++;

    if(write_block(newBlockNum, &newBlock)==-1)
        perror("Error Writing Block");
    if(write_block(current_dir, &currentBlock)==-1)
        perror("Error Writing Block");
    return 0;
}


/* jfs_chdir
 *   changes the current directory to the specified subdirectory, or changes
 *   the current directory to the root directory if the directory_name is NULL
 * directory_name - name of the subdirectory to make the current
 *   directory; if directory_name is NULL then the current directory
 *   should be made the root directory instead
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_NOT_DIR
 */
int jfs_chdir(const char* directory_name) {
    if(directory_name!=NULL)
    {
        //Read current block
        struct block currentBlock; //Block to store current directory
        if(read_block(current_dir, &currentBlock))
            perror("Error Reading Block");
        for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
        {
            //If input name matches
            if(strcmp(currentBlock.contents.dirnode.entries[i].name, directory_name)==0)
            {
                //If requested entry is a directory
                if(is_dir(currentBlock.contents.dirnode.entries[i].block_num)==TRUE)
                {
                    current_dir = currentBlock.contents.dirnode.entries[i].block_num;
                    return 0;
                }
                //Else throw error
                else return E_NOT_DIR;
            }
        }
        //If loop terminates, throw directory not found error
        return E_NOT_EXISTS;
    }
    else current_dir = 1;
    struct block newBlock;
    if(read_block(current_dir, &newBlock)==-1)
        perror("Block read error");
    return 0;
}


/* jfs_ls
 *   finds the names of all the files and directories in the current directory
 *   and writes the directory names to the directories argument and the file
 *   names to the files argument
 * directories - array of strings; the function will set the strings in the
 *   array, followed by a NULL pointer after the last valid string; the strings
 *   should be malloced and the caller will free them
 * file - array of strings; the function will set the strings in the
 *   array, followed by a NULL pointer after the last valid string; the strings
 *   should be malloced and the caller will free them
 * returns 0 on success or one of the following error codes on failure:
 *   (this function should always succeed)
 */
int jfs_ls(char* directories[MAX_DIR_ENTRIES+1], char* files[MAX_DIR_ENTRIES+1]) {
    struct block currentBlock;
    int numDirectories = 0;
    int numFiles = 0;
    if(read_block(current_dir, &currentBlock)==-1)
        perror("Error Reading Block");

    //If current directory is empty
    if(currentBlock.contents.dirnode.num_entries==0)
    {
        directories[0] = NULL;
        files[0] = NULL;
        return 0;
    }

    //Else iterate through entries
    for(int i=0; i < currentBlock.contents.dirnode.num_entries; i++)
    {
        if(is_dir(currentBlock.contents.dirnode.entries[i].block_num))
        {
            char *dirName = malloc(MAX_NAME_LENGTH + 1);
            strcpy(dirName, currentBlock.contents.dirnode.entries[i].name);
            directories[numDirectories] = dirName;
            numDirectories++;
        }
        else {
            char *fileName = malloc(MAX_NAME_LENGTH + 1);
            strcpy(fileName, currentBlock.contents.dirnode.entries[i].name);
            files[numFiles] = fileName;
            numFiles++;
        }
    }
    directories[numDirectories] = NULL;
    files[numFiles] = NULL;
    return 0;
}


/* jfs_rmdir
 *   removes the specified subdirectory of the current directory
 * directory_name - name of the subdirectory to remove
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_NOT_DIR, E_NOT_EMPTY
 */
int jfs_rmdir(const char* directory_name) {
    if(directory_name==NULL)
        return E_NOT_DIR;
    struct block currentBlock;
    if(read_block(current_dir, &currentBlock) == -1)
        perror("Error reading block");

    //Iterate through child entries
    for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
    {
        //If input name matches
        if(strcmp(currentBlock.contents.dirnode.entries[i].name, directory_name)==0)
        {
            //If requested entry is a directory
            if(is_dir(currentBlock.contents.dirnode.entries[i].block_num))
            {
                //Read directory to determine if there are subdirecotries
                struct block blockToRemove;
                if(read_block(currentBlock.contents.dirnode.entries[i].block_num, &blockToRemove)==-1)
                    perror("Error reading block");
                //If requested directory has contents
                if(blockToRemove.contents.dirnode.num_entries>0)
                {
                    return E_NOT_EMPTY;
                }

                //Release block from disk
                if(release_block(currentBlock.contents.dirnode.entries[i].block_num)==-1){
                    perror("Error Releasing Block");
                }

                //Once free, remove entry from list by setting entry[n] -> entry[n+1]
                for(int j=i; j < currentBlock.contents.dirnode.num_entries-1; j++)
                    currentBlock.contents.dirnode.entries[j] = currentBlock.contents.dirnode.entries[j + 1];

                //Decrement number of entries
                currentBlock.contents.dirnode.num_entries--;
                //Write changes
                write_block(current_dir, &currentBlock);
                return 0;
            }
            //Else throw error
            else return E_NOT_DIR;
        }
    }
    //If loop completes, directory does not exist as subdirectory of current folder
    return E_NOT_EXISTS;
}


/* jfs_creat
 *   creates a new, empty file with the specified name
 * file_name - name to give the new file
 * returns 0 on success or one of the following error codes on failure:
 *   E_EXISTS, E_MAX_NAME_LENGTH, E_MAX_DIR_ENTRIES, E_DISK_FULL
 */
int jfs_creat(const char* file_name) {
    //Check String Length
    if(strlen(file_name) > MAX_NAME_LENGTH)
        return E_MAX_NAME_LENGTH;
    struct block currentBlock;
    if(read_block(current_dir, &currentBlock)==-1)
        perror("Error reading block");

    //Check Num Entries
    if(currentBlock.contents.dirnode.num_entries == MAX_DIR_ENTRIES)
        return E_MAX_DIR_ENTRIES;

    //Check if file exists
    for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
    {
        if(strcmp(currentBlock.contents.dirnode.entries[i].name, file_name)==0 &&
            !is_dir(currentBlock.contents.dirnode.entries[i].block_num)){
            return E_EXISTS;
        }
    }

    //Allocate Block
    block_num_t inodeBlockNum = allocate_block();
    //If allocation failed, disk full
    if((int) inodeBlockNum == 0) {
        inodeBlockNum = allocate_block();
        if(inodeBlockNum==0)
            return E_DISK_FULL;
    }

    //Read data into block iNode
    struct block inode;
    if(read_block(inodeBlockNum, &inode)==-1)
        perror("Error Reading Block");

    //Initialize block data and write
    inode.is_dir=FALSE;
    inode.contents.inode.file_size = 0;
    if(write_block(inodeBlockNum, &inode)==-1)
        perror("Error Writing Block");

    //Set directory block as child of current directory block
    for(int i=0; i< MAX_NAME_LENGTH; i++)
    {
        if(i>=strlen(file_name))
            currentBlock.contents.dirnode.entries[currentBlock.contents.dirnode.num_entries].name[i] = '\0';
        else currentBlock.contents.dirnode.entries[currentBlock.contents.dirnode.num_entries].name[i] = file_name[i];
    }
    currentBlock.contents.dirnode.entries[currentBlock.contents.dirnode.num_entries].block_num = inodeBlockNum;
    //Increment num entries
    currentBlock.contents.dirnode.num_entries++;

    //Write parent block changes back to disk
    if(write_block(current_dir, &currentBlock)==-1)
        perror("Error writing block");
  return 0;
}


/* jfs_remove
 *   deletes the specified file and all its data (note that this cannot delete
 *   directories; use rmdir instead to remove directories)
 * file_name - name of the file to remove
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR
 */
int jfs_remove(const char* file_name) {
    if(file_name==NULL)
        return E_NOT_DIR;
    struct block currentBlock;
    if(read_block(current_dir, &currentBlock) == -1)
        perror("Error reading block");

    //Iterate through child entries
    for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
    {
        //If input name matches
        if(strcmp(currentBlock.contents.dirnode.entries[i].name, file_name)==0)
        {
            //If requested entry is a directory, throw error
            if(is_dir(currentBlock.contents.dirnode.entries[i].block_num))
                return E_IS_DIR;
            //Else throw error
            else {
                struct block blockToRemove;
                if(read_block(currentBlock.contents.dirnode.entries[i].block_num, &blockToRemove)==-1)
                    perror("Error reading block");

                //Release all data blocks
                for(int j=0; j< MAX_DATA_BLOCKS; j++)
                {
                    if(release_block(blockToRemove.contents.inode.data_blocks[j])!=0)
                        perror("Error Releasing Data Block");
                }

                //Release iNode
                if(release_block(currentBlock.contents.dirnode.entries[i].block_num)!=0)
                    perror("Error Releasing INode");

                //Remove entry from list by setting entry[n] -> entry[n+1]
                for(int j=i; j < currentBlock.contents.dirnode.num_entries-1; j++)
                    currentBlock.contents.dirnode.entries[j] = currentBlock.contents.dirnode.entries[j + 1];

                //Decrement number of entries
                currentBlock.contents.dirnode.num_entries--;

                //Write changes to parent block
                if(write_block(current_dir, &currentBlock)==-1)
                    perror("Error Writing Block");

                return 0;
            }
        }
    }
    //If loop completes, file not found
    return E_NOT_EXISTS;
}


/* jfs_stat
 *   returns the file or directory stats (see struct stat for details)
 * name - name of the file or directory to inspect
 * buf  - pointer to a struct stat (already allocated by the caller) where the
 *   stats will be written
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS
 */
int jfs_stat(const char* name, struct stats* buf) {
    struct block currentBlock;
    if(read_block(current_dir, &currentBlock) == -1)
        perror("Error Reading Block");

    for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
    {
        //If input name matches
        if(strcmp(currentBlock.contents.dirnode.entries[i].name, name)==0)
        {
            //Read child block
            struct block childBlock;
            if(read_block(currentBlock.contents.dirnode.entries[i].block_num, &childBlock)==-1)
                perror("Error reading block");

            //Get stats
            buf->is_dir = !is_dir(currentBlock.contents.dirnode.entries[i].block_num);
            buf->block_num = currentBlock.contents.dirnode.entries[i].block_num;
            strcpy(buf->name, currentBlock.contents.dirnode.entries[i].name);

            if(!is_dir(currentBlock.contents.dirnode.entries[i].block_num))
            {
                buf->file_size = childBlock.contents.inode.file_size;
                if(childBlock.contents.inode.file_size==0)
                    buf->num_data_blocks = 0;
                //Calculate num data blocks
                else buf->num_data_blocks = 1 + childBlock.contents.inode.file_size/BLOCK_SIZE;
            }
            return 0;
        }
    }
    //If loop terminates, name not found
    return E_NOT_EXISTS;
}


/* jfs_write
 *   appends the data in the buffer to the end of the specified file
 * file_name - name of the file to append data to
 * buf - buffer containing the data to be written (note that the data could be
 *   binary, not text, and even if it is text should not be assumed to be null
 *   terminated)
 * count - number of bytes in buf (write exactly this many)
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR, E_MAX_FILE_SIZE, E_DISK_FULL
 */
int jfs_write(const char* file_name, const void* buf, unsigned short count) {
    struct block currentBlock;
    struct block iNode;
    if(read_block(current_dir, &currentBlock)==-1)
        perror("Error reading block");

    for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
    {
        if(strcmp(file_name,currentBlock.contents.dirnode.entries[i].name)==0)
        {
            //Read iNode
            if(read_block(currentBlock.contents.dirnode.entries[i].block_num, &iNode)==-1)
                perror("Error reading iNode");

            //If node is directory
            if(iNode.is_dir)
                return E_IS_DIR;

            //Check File Size
            if(iNode.contents.inode.file_size+count>MAX_FILE_SIZE)
                return E_MAX_FILE_SIZE;

            int currentBlockIndex = iNode.contents.inode.file_size / BLOCK_SIZE;
            int newBlockIndex = (iNode.contents.inode.file_size + count)/BLOCK_SIZE;

            for(int i=currentBlockIndex; i<=newBlockIndex; i++)
            {
                //If block not yet allocated
                if(iNode.contents.inode.data_blocks[i]==0)
                {
                    //Allocate Block
                    block_num_t dataBlockNum = allocate_block();
                    //If allocation fails
                    if((int) dataBlockNum == 0)
                    {
                        //Release all previously allocated blocks
                        for(int j=i; j>=1; j--)
                        {
                            if(release_block(currentBlock.contents.dirnode.entries[j].block_num)==-1)
                                perror("Error Releasing Block");
                        }
                        return E_DISK_FULL;
                    }
                    //Add BlockNum to Data Blocks
                    else iNode.contents.inode.data_blocks[currentBlockIndex + i] = dataBlockNum;
                }
            }

            int bytesRemaining = count;
            for(int i=currentBlockIndex; i<=newBlockIndex; i++)
            {
                //Index of the last byte in
                char dataBlock[BLOCK_SIZE];
                for(int i=0; i<BLOCK_SIZE; i++)
                    dataBlock[i] = '\0';
                //First loop iteration -> Append data directly
                if(i==currentBlockIndex)
                {
                    //Read Current Contents of Block into Byte Array
                    if(read_block(iNode.contents.inode.data_blocks[i], &dataBlock) == -1)
                        perror("Error Reading Block");
                    //Current Index
                    int currentByteIndex = iNode.contents.inode.file_size % BLOCK_SIZE;
                    //Space remaining on current block
                    int bytesToWrite = BLOCK_SIZE - currentByteIndex;
                    //Copy memory to data block and write
                    memcpy(dataBlock+currentByteIndex, buf, bytesToWrite);
                    if(write_block(iNode.contents.inode.data_blocks[0], &dataBlock) == -1)
                        perror("Error Writing Block");
                    bytesRemaining -= bytesToWrite;
                    //Increment Buffer for subsequent blocks
                    if(bytesRemaining>0)
                        buf+=bytesToWrite;
                }
                //Else append data starting from the beginning of the block
                else{
                    int bytesToWrite = bytesRemaining < BLOCK_SIZE ? bytesRemaining : BLOCK_SIZE;
                    memcpy(dataBlock, buf, bytesToWrite);
                    if(write_block(iNode.contents.inode.data_blocks[i], &dataBlock) == -1)
                        perror("Error Writing Block");
                    bytesRemaining -= BLOCK_SIZE;
                }
            }
            iNode.contents.inode.file_size+=count;

            //Write iNode changes
            if(write_block(currentBlock.contents.dirnode.entries[i].block_num, &iNode)==-1)
                perror("Error Writing Block");
            if(write_block(current_dir, &currentBlock)==-1)
                perror("Error Writing Block");

            return 0;
        }
    }
    //Loop terminates, then file not found
    return E_NOT_EXISTS;
}


/* jfs_read
 *   reads the specified file and copies its contents into the buffer, up to a
 *   maximum of *ptr_count bytes copied (but obviously no more than the file
 *   size, either)
 * file_name - name of the file to read
 * buf - buffer where the file data should be written
 * ptr_count - pointer to a count variable (allocated by the caller) that
 *   contains the size of buf when it's passed in, and will be modified to
 *   contain the number of bytes actually written to buf (e.g., if the file is
 *   smaller than the buffer) if this function is successful
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR
 */
int jfs_read(const char* file_name, void* buf, unsigned short* ptr_count) {
    struct block currentBlock;
    if(read_block(current_dir, &currentBlock)==-1)
        perror("Error reading block");

    for(int i=0; i<currentBlock.contents.dirnode.num_entries; i++)
    {
        //If name found
        if(strcmp(file_name,currentBlock.contents.dirnode.entries[i].name)==0) {
            //Read iNode
            struct block iNode;
            if (read_block(currentBlock.contents.dirnode.entries[i].block_num, &iNode) == -1)
                perror("Error reading iNode");

            //If node is directory
            if (iNode.is_dir)
                return E_IS_DIR;
            if(iNode.contents.inode.file_size==0) {
                *ptr_count = 0;
                return 0;
            }

            //Get min of ptr_count & fileSize
            int bytesLefToRead = (*ptr_count < iNode.contents.inode.file_size) ? *ptr_count : iNode.contents.inode.file_size;
            //Calculate Num Blocks to read
            int numDataBlocks = bytesLefToRead/BLOCK_SIZE;

            //Loop through data blocks to read
            for(int i=0; i<=numDataBlocks; i++)
            {
                char dataBlock[BLOCK_SIZE];
                for(int i=0; i<BLOCK_SIZE; i++)
                    dataBlock[i] = '\0';
                if(read_block(iNode.contents.inode.data_blocks[i], &dataBlock)==-1)
                    perror("Error Reading Block");

                //If final block -> Write only bytes remaining
                if(bytesLefToRead<=BLOCK_SIZE)
                {
                    //Read data block contents to buffer
                    memcpy(buf, dataBlock, bytesLefToRead);
                    bytesLefToRead-=BLOCK_SIZE;
                }
                else{
                    //Read data block contents to buffer
                    memcpy(buf, dataBlock, BLOCK_SIZE);
                    bytesLefToRead-=BLOCK_SIZE;
                    buf+=BLOCK_SIZE;
                }
            }
            //Adjust pointer count
            *ptr_count = (*ptr_count < iNode.contents.inode.file_size) ? *ptr_count : iNode.contents.inode.file_size;
            buf -= *ptr_count;
            return 0;
        }

    }
    //If loop completes, file does not exist
    return E_NOT_EXISTS;
}


/* jfs_unmount
 *   makes the file system no longer accessible (unless it is mounted again).
 *   This should be called exactly once after all other jfs_* operations are
 *   complete; it is invalid to call any other jfs_* function (except
 *   jfs_mount) after this function complete.  Basically, this closes the DISK
 *   file on the _real_ file system.  If your code requires any clean up after
 *   all other jfs_* functions are done, you may add it here.
 * returns 0 on success or -1 on error; errors should only occur due to
 *   errors in the underlying disk syscalls.
 */
int jfs_unmount() {
  int ret = bfs_unmount();
  for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
      release_block(i);
  }
  current_dir = 1;
  return 0;
}
