Unix V6 File System


PROBLEM And OBJECTIVE 
To design a UNIX V6 file system to remove the 16 MB limitation on file size.
This file system  will allow user  access to the file system of a foreign operating system.
to compile this program, use 'g++ main.cpp MetaDirectory.cpp -o fsaccessFS'
to run the program, use './fsaccessFS'

This V6 file system will support the following commands 


1. initfs path inodes blocks
	example: initfs project 200 300

2. cpin external_internal
	example: cpin test v6

3. cpout internal external
	example: cpout v6 copy_of _test

4. mkdir path
	example: mkdir  newdir

5. rm filename
        example: rm v6

6. q