#include "jumbo_file_system.c"

void testMaxEntries(int *res);

int main()
{
    int res = 0;
    testMaxEntries(&res);
    return res;
}

void testMaxEntries(int *res)
{
    //Fill the DISK
    for(int i=0; i<NUM_BLOCKS;i++)
    {
        //If MAX DIR ENTRIES
        if(jfs_mkdir(&"d"[ i])==E_MAX_DIR_ENTRIES)
        {
            //CD INTO SUBDIRECTORY AND CONTINUE
            jfs_chdir(&"d"[ i-1]);
            jfs_mkdir(&"d"[ i]);
        }
    }
    if(jfs_creat("file")==E_DISK_FULL)
        *res = TRUE;
    else *res = FALSE;
}
