//Tis is project of Yili Zou(yxz163931) and Jing Zhou(jxz160330)
//To design a UNIX V6 file system to remove the 16 MB limitation on file size.
//This file system  will allow user  access to the file system of a foreign operating system.
// to compile this program, use 'g++ main.cpp MetaDirectory.cpp -o fsaccessFS'
//to run the program, use './fsaccessFS'



#include <string>
#include <cstring>

using namespace std;


class MetaDirectory {
private:
	//Kept private so only this class can have access to its instance
	static MetaDirectory* m_Instance;

public:
	string *entryName;
	string path;
	int *inodeList;
	int inodeCounter,numInodes;
	fstream fileSystem;

	MetaDirectory()	{
		entryName = NULL;
		path = "";
		inodeCounter = numInodes =0;
		inodeList = new int[0];
	}




	static MetaDirectory* Instance(); //Public static access function


	void setNumInodes(int num);
	string getPath(void);
	int getInodeNum(void);
	int getNextFreeInode(void);
	void quit(void);
	void smallRoot(void);
	void dirState(void);
	void file_entry(string,int);
	void setPath(string path);
	void bigRoot(void);
	int getLastBlockUsed(void);
	int getNextFreeBlock(void);
};

