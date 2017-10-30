

//Tis is project of Yili Zou(yxz163931) and Jing Zhou(jxz160330)
//To design a UNIX V6 file system to remove the 16 MB limitation on file size.
//This file system  will allow user  access to the file system of a foreign operating system.
// to compile this program, use 'g++ main.cpp MetaDirectory.cpp -o fsaccessFS'
//to run the program, use './fsaccessFS'

#include <fstream>
#include <string>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include<cstring>
#include<stdlib.h>
#include <cstring>
#include "MetaDirectory.h"
#include "Constants.h"

using namespace std;


class Cpin {
	fstream fileSystem,eStream;
	string external, internal;
	int numberOfBlocksNeeded,fileSize;
public:
	bool checkParameters(string inp){
	bool validCP = false;
	string eFile,iFile;
	int space[2];
	space[0] = inp.find(" ");
	space[1] = inp.find(" ",space[0]+1);
	space[2] = inp.find(" ",space[1]+1);
	eFile = inp.substr(space[0]+1,space[1]-space[0]-1);
	iFile = inp.substr(space[1]+1,space[2]-space[1]-1);

	if ((eFile.length() <=0) | (iFile.length()<=0) | (space[0]<=0) | (space[1]<=0)){
		cout <<"Please enter valid argument" <<endl;
		validCP = false;
	}else{
		setExternalFile(eFile);
		setInternalFile(iFile);
		validCP = true;
	}
	return validCP;
}


void copyFile(string inp,string path){
string src,des;
try{
	if(checkParameters(inp)){
		src = getExternalFile();
		des = getInternalFile();
		fileSystem.open(path.c_str(),ios::binary | ios::in | ios::out);
		if(fileSystem.is_open()){
			eStream.open(getExternalFile().c_str(),ios::in | ios::ate);
			if(eStream.is_open()){
				if(isSmallFile()){ //Small File
					copySmallFile();
					cout <<"File has been copied" <<endl;
				}else{ //Large File
					if(copyLargeFile())	cout <<"File has been copied" <<endl;
					else cout <<"Error with copying files" <<endl;
				}
			eStream.clear();
			eStream.close();
			}else cout<<"Could not find file" <<endl;
		} else cout<<"File system does not exist" <<endl;
		fileSystem.clear();
		fileSystem.close();
	}
 }catch(exception& e){
	 cout <<"Exception occured at Cpin :: Copy File" <<endl;
 }
}

/**
 * Copies the content of the small file to mV6 file system
 *
 */
void copySmallFile(void){
	iNode newInodeToAllocate;
	int sInode;
	unsigned short lsb;
	char msb,dummy;

	char *fileContents = new char[fileSize];
	eStream.read(fileContents,fileSize);
	newInodeToAllocate.flags = (newInodeToAllocate.flags | 0x8000);
	//Storing size
	lsb = (fileSize & 0xffff);
	dummy= (int)(fileSize >> 16);
	msb = static_cast<unsigned int>(dummy);
	newInodeToAllocate.size1 = lsb;
	newInodeToAllocate.size0 = msb;

	for(int i=0;i<numberOfBlocksNeeded;i++) newInodeToAllocate.addr[i] = getNextFreeBlock();
	//Filling 0's for remaining addr array
	for(int i=numberOfBlocksNeeded;i<8;i++) newInodeToAllocate.addr[i] = 0;
	//Writing iNodes
	sInode = MetaDirectory::Instance()->getNextFreeInode();
	fileSystem.seekg(0,ios::beg);
	fileSystem.seekg(BLOCK_SIZE + (sInode * (int)sizeof(iNode)));
	fileSystem.write((char *)&newInodeToAllocate,sizeof(iNode));


	//Writing data
	fileSystem.seekg(0,ios::beg);
	fileSystem.seekg(BLOCK_SIZE * newInodeToAllocate.addr[0]);
	fileSystem.write(fileContents,fileSize);

	MetaDirectory::Instance()->file_entry(getInternalFile(),sInode);
}


bool copyLargeFile(void){
	bool large = false;
	iNode newInodeToAllocate;
	int thisInode;
	unsigned short lsb;
	char msb,dummy;
	int addrRequired = numberOfBlocksNeeded/131072;

	if(numberOfBlocksNeeded % 131072!=0) addrRequired++;

	if(addrRequired > 8){	//If file size exceeds maximum
		cout<<"File exceeds limit" <<endl;
		large = false;
	}else{
		newInodeToAllocate.flags = (newInodeToAllocate.flags | 0x9000);
		for(int i=0;i<addrRequired;i++) newInodeToAllocate.addr[i] = getNextFreeBlock();
		//Filling 0's for remaining addr array
		for(int i=addrRequired;i<8;i++) newInodeToAllocate.addr[i] = 0;

		lsb = (fileSize & 0xffff);
		dummy= (int)(fileSize >> 16);
		if(dummy < 256)	msb = static_cast<unsigned int>(dummy);
		else{
			dummy= (int)(fileSize >> 15);
			msb = static_cast<unsigned int>(dummy);
			//Setting the 25th bit in flags field
			newInodeToAllocate.flags |= 0x9200;
		}
		newInodeToAllocate.size1 = lsb;
		newInodeToAllocate.size0 = msb;

		//Writing iNodes
		thisInode = MetaDirectory::Instance()->getNextFreeInode();
		fileSystem.seekg(0,ios::beg);
		fileSystem.seekg(BLOCK_SIZE + (thisInode * (int)sizeof(iNode)));
		fileSystem.write((char *)&newInodeToAllocate,sizeof(iNode));

		//Writing single indirect blocks
		int blocksAllocated = 0; 				// Keep track of num of addr array allocated
		unsigned short *assignBlocks;
		assignBlocks= new unsigned short[256];

		//write 256->512 addresses into the first indirect block
		for(int i=0;i<addrRequired;i++){
			int j=0;
			for(;(j<256) && (blocksAllocated<=numberOfBlocksNeeded);j++){
				assignBlocks[j] =getNextFreeBlock();
				blocksAllocated+=512;
			}

			//add zeros if less than 256
			for(;j<256;j++)	assignBlocks[j] = 0;

			fileSystem.seekg(newInodeToAllocate.addr[i] * BLOCK_SIZE,ios::beg);
			fileSystem.write((char *)&assignBlocks,256 * sizeof(unsigned short));
		}

		char *fileContents = new char[fileSize]; //reading input data


		//Writing Double indirect blocks
		int start = ((newInodeToAllocate.addr[addrRequired-1])-1);
		int stop = getLastBlockUsed();
		assignBlocks= new unsigned short[256];
		blocksAllocated = 0;
		int j=0;
		for(int i=start; i>=stop; i--){
			for(;(j<256) && (blocksAllocated<=numberOfBlocksNeeded);j++){
				assignBlocks[j]=getNextFreeBlock();

				//Writing data
				fileSystem.seekg(BLOCK_SIZE * assignBlocks[j],ios::beg);
				eStream.seekg(BLOCK_SIZE * j);
				eStream.read(fileContents,BLOCK_SIZE);
				fileSystem.write(fileContents,BLOCK_SIZE);
				blocksAllocated++;
			}
			for(;j<256;j++) assignBlocks[j] = 0;
			fileSystem.seekg(i * BLOCK_SIZE,ios::beg);
			fileSystem.write((char *)&assignBlocks,256 * sizeof(unsigned short));
		}

	large = true;
	}

	MetaDirectory::Instance()->file_entry(getInternalFile(),thisInode);
	return large;
}


void setExternalFile(string name){
	this->external = name;
}


string getExternalFile(void){
	return this->external;
}


void setInternalFile(string name){
	this->internal = name;
}


string getInternalFile(void){
	return this->internal;
}


bool isSmallFile(void){
	bool res = false;
	fileSize = eStream.tellg();
	eStream.seekg(0,ios::beg);
	if(fileSize == 0) cout <<"!!Empty File!!" <<endl;
	else{
		numberOfBlocksNeeded = fileSize / BLOCK_SIZE;			 //calculate the the data blocks required to store the file
		if((fileSize % BLOCK_SIZE) !=0) numberOfBlocksNeeded++ ; //extra data lesser than a block size
		if(fileSize <= 4096) res = true;
		else res = false;
	}
	return res;
}


int getNextFreeBlock(void){
	int freeBlk,freeChainBlock;
	unsigned short freeHeadChain;
	superBlock freeBlockSB;
	try{
		//Moving the cursor to the start of the file
		fileSystem.seekg(0);

		//Reading the contents of super block
		fileSystem.read((char *)&freeBlockSB,sizeof(superBlock));

		if(freeBlockSB.nfree == 1){
			freeChainBlock = freeBlockSB.free[--freeBlockSB.nfree];

			//Head of the chain points to next head of chain free list
			fileSystem.seekg(freeChainBlock);
			fileSystem.read((char *)&freeHeadChain,2);

			//reset nfree to 1
			freeBlockSB.nfree = 100;

			//reset the free array to new free list
			for(int k=0;k<100;k++){
				freeBlockSB.free[k] = freeHeadChain + k;
			}

			freeBlk = (int)freeBlockSB.free[--freeBlockSB.nfree];

			//Writing after the free head chain is changed
			fileSystem.write((char *)&freeBlockSB,sizeof(superBlock));
		}else{
			freeBlk = (int)freeBlockSB.free[--freeBlockSB.nfree];
			//Writing after the free block has been allocated
			fileSystem.seekg(0,ios::beg);
			fileSystem.write((char *)&freeBlockSB,sizeof(superBlock));
		}
	}catch(exception& e){
		cout<<"Exception at getNextFreeBlock " <<endl;
	}
	return freeBlk;
}


int getLastBlockUsed(void){
	int lastBlk;
	superBlock freeBlockSB;
	try{
		//Moving the cursor to the start of the file
		fileSystem.seekg(0);

		//Reading the contents of super block
		fileSystem.read((char *)&freeBlockSB,sizeof(superBlock));

		lastBlk = (int)freeBlockSB.free[freeBlockSB.nfree];
	}catch(exception& e){
		cout<<"Exception at getNextFreeBlock " <<endl;
	}
	return lastBlk;
}
};



class MakeDir {
	string dirname;
	fstream fileSystem;
public:

void createDirectory(string inp,string path){
	if(checkParameters(inp)){
		fileSystem.open(path.c_str(),ios::binary | ios::in | ios::out);
		if(fileSystem.is_open()){
			if(searchDirectoryEntries()){ //Searched Directory has been found
				cout <<"directory already exist" <<endl;
				return;
			}else{
				allocateFreeDirectoryEntry();
				cout <<"new directory created" <<endl;
			}
		fileSystem.clear();
		fileSystem.close();
		} else cout<<"File system does not exist" <<endl;
	}
}

//to validate the file name
bool checkParameters(string inp){
	bool validCP = false;
	string name;
	int space[1];
	space[0] = inp.find(" ");
	space[1] = inp.find(" ",space[0]+1);
	name = inp.substr(space[0]+1,space[1]-space[0]-1);
	if((name.length()<=0) | (space[0]<=0)){
		cout <<"invalid argument" <<endl;
		validCP = false;
	}else if(name.size()>13){
		cout <<"directory name must be less than 14 in length" <<endl;
		setDirectoryName(name);
		validCP = true;
	}else{
		setDirectoryName(name);
		validCP = true;
	}
	return validCP;
}


bool searchDirectoryEntries(){
bool found = false;
int i=0;

	for(;i<=MetaDirectory::Instance()->inodeCounter;i++){
			if(MetaDirectory::Instance()->entryName[i].compare(getDirectoryName())==0) {
				found = true;
				break;
			}
	}

	if(i>=MetaDirectory::Instance()->inodeCounter){
		found = false;
		return found;
	}

return found;
}


void allocateFreeDirectoryEntry(){
iNode newInodeToAllocate;
Directory rootDirectory,myDirectory,filler;
int dnode;
	newInodeToAllocate.flags = (newInodeToAllocate.flags | 0x9000);
	dnode =  MetaDirectory::Instance()->getNextFreeInode();
	MetaDirectory::Instance()->file_entry(getDirectoryName(),dnode);
	//Writing iNodes
	fileSystem.seekg(0,ios::beg);
	fileSystem.seekg(BLOCK_SIZE + (dnode * (int)sizeof(iNode)));
	fileSystem.write((char *)&newInodeToAllocate,sizeof(iNode));

	//Writing directory
	fileSystem.seekg(0,ios::beg);
	fileSystem.seekg(BLOCK_SIZE * newInodeToAllocate.addr[0]);

	//Setting '.' character
	rootDirectory.inodeNumber=1;
	strcpy(rootDirectory.fileName,"..");
	fileSystem.write((char *)&rootDirectory,sizeof(rootDirectory));

	//Setting directory name
	string name = getDirectoryName();
	myDirectory.inodeNumber=dnode;
	strcpy(myDirectory.fileName,name.c_str());
	fileSystem.write((char *)&rootDirectory,sizeof(rootDirectory));

	//Setting the remaining directory entries
	for(int j=3; j<=numDirectoryEntry; j++){
		fileSystem.write((char *)&filler,sizeof(filler));
	}
}

void setDirectoryName(string name){
	this->dirname = name;
}


string getDirectoryName(void){
	return this->dirname;
}
};





class Cpout {
	fstream fileSystem,eStream;
	string external, internal;
	int blocks;
	long fileSize;
public:
//check the parameter, inp is the input string
//return true if valid
//return false if invalid
bool checkParameters(string inp){
	bool validCP = false;
	string eFile,iFile;
	int space[2];
	space[0] = inp.find(" ");
	space[1] = inp.find(" ",space[0]+1);
	space[2] = inp.find(" ",space[1]+1);
	iFile = inp.substr(space[0]+1,space[1]-space[0]-1);
	eFile = inp.substr(space[1]+1,space[2]-space[1]-1);

	if ((eFile.length() <=0) | (iFile.length()<=0) | (space[0]<=0) | (space[1]<=0) ){
		cout <<"Arguments are not correct" <<endl;
		validCP = false;
	}else{
		setInternalFile(iFile);
		setExternalFile(eFile);
		validCP = true;
	}
	return validCP;
}


 //
void copyOutFile(string inp,string path){
	string src,des;
	try{
		if(checkParameters(inp)){
			src = getInternalFile();
			des = getExternalFile();
			fileSystem.open(path.c_str(),ios::binary | ios::in );
			if(fileSystem.is_open()){
				if(searchFile()){
					cout <<"file has been copied" <<endl;
				}else
					cout <<"Could not find the file"  <<endl;
			} else cout<<"Could not find the file" <<endl;
			fileSystem.clear();
			fileSystem.close();
		}
	 }catch(exception& e){
		 cout <<"Exception occured at Cpin :: Copy File" <<endl;
	 }

}

//search for the file and output it
bool searchFile(){
	bool found = false;
	string ifile = getInternalFile();
	string efile = getExternalFile();

	int i=0,startAddr;

		for(;i<=MetaDirectory::Instance()->inodeCounter;i++){
			if(MetaDirectory::Instance()->entryName[i].compare(ifile)==0) break;
		}

		if(i>=MetaDirectory::Instance()->inodeCounter){
			found = false;
			return found;
		}

		//get inode for the v6 file
		int v6node = MetaDirectory::Instance()->inodeList[i];

		iNode vInode;
		fileSystem.seekg(BLOCK_SIZE + (v6node*(int)sizeof(iNode)),ios::beg);
		fileSystem.read((char *)&vInode,sizeof(iNode));

		//Get file size and starting address
		if((vInode.flags & 0x9200) == 0x9200){
		 fileSize =((vInode.flags & 0x0040)| vInode.size0 <<15 | vInode.size1);
		}else if((vInode.flags & 0x8000) == 0x8000){
		 fileSize =(vInode.size0 <<16 | vInode.size1);
		}else{
			found = false;
			return found;
		}
		calBlocks(fileSize);
		if(fileSize <= 4096) startAddr = vInode.addr[0];
		else{
			unsigned short *one,*two;
			one = new unsigned short[256];
			two = new unsigned short[256];
			fileSystem.seekg(vInode.addr[0]*BLOCK_SIZE,ios::beg);
			fileSystem.read((char *)&one,256  * sizeof(unsigned short));
			fileSystem.seekg(one[0]*BLOCK_SIZE,ios::beg);
			fileSystem.read((char *)&two,256  * sizeof(unsigned short));
			startAddr = two[0];
			found = true;
		}

		eStream.open(efile.c_str(),ios::out | ios::app);
		int blk = getBlocksToRead();
		if(eStream.is_open()){
			for(int m=0;m<blk;m++){
				fileSystem.seekg(BLOCK_SIZE*startAddr,ios::beg);
				char *buffer = new char[BLOCK_SIZE];
				fileSystem.read(buffer,BLOCK_SIZE);
				eStream.seekg(m * BLOCK_SIZE,ios::beg);
				cout <<eStream.tellg() <<endl;
				eStream.write(buffer,BLOCK_SIZE);
				startAddr--;
			}
			found = true;
			eStream.close();
		}
return found;
}

//set blocks to read
void setBlocksToRead(long n){
	this->blocks = n;
}

//calculate clocks needed for the file
void calBlocks(long fsize){
	long size;
	size = fsize/BLOCK_SIZE;
	if(fsize % BLOCK_SIZE !=0) size++;
	this->setBlocksToRead(size);
}


int getBlocksToRead(){
	return this->blocks;
}

void setExternalFile(string name){
	this->external = name;
}

string getExternalFile(void){
	return this->external;
}

void setInternalFile(string name){
	this->internal = name;
}

string getInternalFile(void){
	return this->internal;
}
};


class Rm
{
    fstream fileSystem;
	string v6File;
	int blocks,numBlocksToRemove,fileSize;
	public:

	void removeFile(string inp, string path){
			v6File = (string)inp.substr(inp.find(" ")+1, inp.length());
        fileSystem.open(path.c_str(),ios::binary | ios::in | ios::out);
        if(fileSystem.is_open()){
        int i=0;

        //search for the inode list, to check it's name
		for(;i<=MetaDirectory::Instance()->inodeCounter;i++){
			if(MetaDirectory::Instance()->entryName[i].compare(v6File)==0) break;
		}
        //file name does not exist
		if(i>=MetaDirectory::Instance()->inodeCounter){
			cout <<"There is no such file!" <<endl;
		}

		//get inode flag, write the first bit to 0, so it is removed
		int v6node = MetaDirectory::Instance()->inodeList[i];

		iNode vInode;
		iNode vInode2;
		fileSystem.seekg(BLOCK_SIZE + (v6node*(int)sizeof(iNode)),ios::beg);
		fileSystem.read((char *)&vInode,sizeof(iNode));
		vInode.flags = vInode.flags&0x7FFF;
		fileSystem.seekg(0,ios::beg);
		fileSystem.seekg(BLOCK_SIZE + (v6node*(int)sizeof(iNode)));
		fileSystem.write((char *)&vInode,sizeof(iNode));
        }
        fileSystem.clear();
        fileSystem.close();
        cout <<"File has been successfully removed!" <<endl;
	}

};



class InitializeFS {
	fstream file;
	string fsPath;
	int numOfBlocks,numOfinodes;
public:
	bool checkParameters(string inp){
	bool valid=false;
	int inodes,blocks;
	string path;
	int space[2];
	space[0] = inp.find(" ");
	space[1] = inp.find(" ",space[0]+1);
	space[2] = inp.find(" ",space[1]+1);
	path = inp.substr(space[0]+1,space[1]-space[0]-1);
	blocks = atoi(inp.substr(space[1]+1,space[2]-space[1]-1).c_str());
	inodes = atoi(inp.substr(space[2]+1,inp.length()-1).c_str());

	if ((blocks <= 0) | (inodes <=0) | (path.length()<=0) | (space[0]<=0) | (space[1]<=0) | (space[2]<=0) ){
		cout <<"!!Invalid arguments!!" <<endl;
		valid = false;
	}else {
		this->setFileSystemPath(path);
		this->setNumOfBlocks(blocks);
		this->setNumOfInodes(inodes);
		valid = true;
	}
	return valid;
	}
	void setFileSystemPath(string path){
        this->fsPath = path;
	}
	string getFileSystemPath(void){
	return this->fsPath;
	}
	void setNumOfBlocks(int n1){
	this->numOfBlocks = n1;
	}
	int getNumOfBlocks(void){
	return this->numOfBlocks;
	}
	void setNumOfInodes(int n2){
	this->numOfinodes = n2;
	}
	int getNumOfInodes(void){
	return this->numOfinodes;
	}
	int getSizeOfInode(void){
	return sizeof(iNode);
	}
	void createFileSystem(string inp){
	superBlock sb;
iNode node,rootNode;
Directory rootDirectory,filler;
unsigned short freeHeadChain;
try{
	if(checkParameters(inp)){
	 file.open(getFileSystemPath().append("fsaccess").c_str(),ios::binary | ios::app);
		if (file.is_open()){
			//Initializing the file system
			sb.isize = getInodesBlock();

			//1 Block is the directory for root node and another block is the head of the free chain list
			sb.fsize = getFreeBlocks();
			sb.ninode = getNumOfInodes();
			sb.nfree = 100;
			for(int i=0;i<100;i++){
				sb.free[i] = (getFreeBlocksIndex() + i);
			}
			file.write((char *)&sb,BLOCK_SIZE);

			//Setting up root node
			rootNode.flags = 0x1800;
			rootNode.addr[0] = (1+ getInodesBlock())*BLOCK_SIZE;
			file.write((char *)&rootNode,getSizeOfInode());

			//Writing inodes block to the file system
			for(int i=2;i<=getNumOfInodes();i++){
				file.write((char *)&node,getSizeOfInode());
			}

			//Padding empty characters to complete block
			if(calculateInodePadding() !=0 ){
				char *iNodeBuffer = new char[calculateInodePadding()];
				file.write((char *)&iNodeBuffer,calculateInodePadding());
			}

			//Writing Root Directory
			MetaDirectory::Instance()->setNumInodes(getNumOfInodes());
			MetaDirectory::Instance()->setPath(getFileSystemPath().append("fsaccess"));
			MetaDirectory::Instance()->dirState();

			//Setting './' string
			rootDirectory.inodeNumber=1;
			strcpy(rootDirectory.fileName,".");
			file.write((char *)&rootDirectory,sizeof(rootDirectory));

			//Setting '..' string
			rootDirectory.inodeNumber=1;
			strcpy(rootDirectory.fileName,"..");
			file.write((char *)&rootDirectory,sizeof(rootDirectory));

			//filler out the rest
			for(int j=3; j<=numDirectoryEntry; j++){
				file.write((char *)&filler,sizeof(filler));
			}

			//create Empty Char buffer
			char *buffer = new char[BLOCK_SIZE];
			char *headChainBuffer;

			//Writing free data blocks
			//If the number of free blocks are <100 no need to set free head chain values
			if((getFreeBlocks() - getFreeBlocksIndex()) < 100){
				for(int i=getFreeBlocksIndex();i<=getFreeBlocks();i++){
					file.write((char *)&buffer,BLOCK_SIZE);
				}
			}else {

			    //Checks for every 100th block and assigns the head chain values
				for(int i=getFreeBlocksIndex();i<=getFreeBlocks();i++){
                    //construct free[100] the headchaine connect to the next free100
					if((i % 100 == getFreeBlocksIndex()) && ((getFreeBlocks() - i) >= 100) ){
							freeHeadChain = i+100;
							file.write((char *)&freeHeadChain,2);
							headChainBuffer = new char[BLOCK_SIZE - 2];
							file.write((char *)&headChainBuffer,BLOCK_SIZE - 2);
					}else{
						file.write((char *)&buffer,BLOCK_SIZE);
					}
				}
			}
			cout <<"File system has been created" <<endl;
		}else{
			cout << "Invalid path" <<endl;
		}
	 file.close();
	 }else{
		return;
	 }
	}catch(exception& e){
		cout <<"Exception at createFileSystem method" <<endl;
	}
	}
	int getInodesBlock(void){
	float sizeOfInode, numInodesBlock, numOfInodes,blockSize;
	int inPB;
	sizeOfInode = getSizeOfInode();
	numOfInodes = getNumOfInodes();
	blockSize = BLOCK_SIZE;
	numInodesBlock = (sizeOfInode * numOfInodes)/blockSize;
	inPB = ceil(numInodesBlock);
	return inPB;
	}
	int getFreeBlocks(void){
	int freeBlockCount;
	freeBlockCount = (getNumOfBlocks() - (getInodesBlock() + 2));
	return freeBlockCount;
	}
	int getFreeBlocksIndex(void){
	int freeBlockIndex;
	freeBlockIndex = getNumOfBlocks() - getFreeBlocks() + 1;
	return freeBlockIndex;
	}
	int calculateInodePadding(void){
	int paddingSize;
	paddingSize =  (getInodesBlock() * BLOCK_SIZE) - (getNumOfInodes() * getSizeOfInode());
	return paddingSize;
	}
	void readBlocks(void)	//Used to test the written data
	{
	    iNode node;
	ifstream infile;
	infile.open("fsaccess",ios::binary);
	if(infile.is_open()){
		infile.seekg(BLOCK_SIZE);
		infile.read((char *)&node,getSizeOfInode());
		for(int k=0;k<8;k++) cout <<"addr = " << node.addr[k] <<endl;
		}
	infile.close();
	}
};





//check to see if the command is valid
bool check_command(string str);




/**
 * Main program, promote user to input command and arguments,
 * tell if there is error
 *
 */
int main() {
	string input,cmd,path;
	bool quit = false;

	InitializeFS fs;
	Cpin in;
	MakeDir mkd;
	Cpout out;
	Rm rm;

	cout <<"This is a modified Unix V6 file system" <<endl;
	cout <<"initfs:initfs path inodes blocks" <<endl;
	cout <<"cpin:cpin external internal" <<endl;
	cout <<"cpout:cpout internal external" <<endl;
	cout <<"mkdir: mkdir path" <<endl;
	cout <<"rm: rm filename" <<endl;
	cout <<"Press 'q' to quit" <<endl;

	while(!quit){
		getline(cin,input);

		if(check_command(input)) {
                string cmd = input.substr(0,input.find(" "));

            if(cmd == "initfs"){
                fs.createFileSystem(input);
                path = (fs.getFileSystemPath()).append("fsaccess");
                quit=false;
            }
            else if(cmd == "cpin"){
                in.copyFile(input,path);
                quit = false;
            }
            else if(cmd == "cpout"){
                out.copyOutFile(input,path);
                quit = false;
            }
            else if(cmd == "mkdir"){
                mkd.createDirectory(input,path);
                quit = false;
            }
            else if(cmd == "q"){
                cout <<"Goodbye!" <<endl;
                quit = true;
            }
            else if(cmd =="rm"){
                rm.removeFile(input,path);
                quit = false;
            }
		}
		else cout<<"This command is not supported!!" <<endl;
	}
	return 0;
}

//check to see if inputs are supported
bool check_command(string str)
{
    string cmd;
	cmd = str.substr(0,str.find(" "));
	if(cmd=="initfs" || cmd=="cpin" || cmd=="cpout" || cmd=="mkdir" || cmd=="q" || cmd=="rm")
		return true;
	else
		return false;

	return 1;
}

