

//Tis is project of Yili Zou(yxz163931) and Jing Zhou(jxz160330)
//To design a UNIX V6 file system to remove the 16 MB limitation on file size.
//This file system  will allow user  access to the file system of a foreign operating system.
// to compile this program, use 'g++ main.cpp MetaDirectory.cpp -o fsaccessFS'
//to run the program, use './fsaccessFS'




#include <string>


/*
 * Constant assigned for block size
 */
#define BLOCK_SIZE 512
//Constant number of directory entries in a data block
#define numDirectoryEntry (BLOCK_SIZE/sizeof(Directory))


//super block structure
struct superBlock{
	unsigned short isize;
	unsigned short fsize;
	unsigned short nfree;
	unsigned short free[100];
	unsigned short ninode;
	unsigned short inode[100];
	char flock;
	char ilock;
	char fmod;
	unsigned short time[2];
	superBlock(){
		isize = 0 ;
		fsize = 0;
		nfree = 0;
		for(int i=0;i<100;i++){
			free[i] = 0;
			inode[i] = 0;
		}
		ninode = 0;
		flock = '0';
		ilock = '0';
		fmod = '0';
		time[0] = time[1] = 0;
	}
};

//Inode structure
struct iNode{
	unsigned short flags;
	char nlinks;
	char uid;
	char gid;
	char size0;
	unsigned short size1;
	unsigned short addr[8];
	unsigned short actime[2];
	unsigned short modtime[2];

	iNode(){
		flags = 0;
		nlinks = '0';
		uid = '0';
		gid ='0';
		size0 ='0';
		size1 =0;
		for(int j=0;j<8;j++) addr[j] =0;
		actime[0] = actime[1] =0;
		modtime[0] = modtime[1] =0;
	}
};

//Directory Structure
struct Directory{
	unsigned short inodeNumber;
	char fileName[14];

	Directory(){
		inodeNumber =0;
		for(int k=0;k<14;k++) fileName[k] =0;
	}
};


