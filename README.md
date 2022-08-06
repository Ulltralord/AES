# AES

This is a software implementation of the algorithm AES with a key length of 128 bits and CBC mode. This program allows you to encrypt/decrypt files and directories. The program can be compiled to run on Linux and Windows systems. In the first case, you need to add 3 files to the project that emulate the work of Windows functions in Linux: _findfirst(), _findnext() and _findclose().

Starting the program:
Aes.exe [-m <mode>] [-i <path to file/s>] [-o <path to file/dir>] [-p <password>]
  
Parameters:
 • -h – displays a hint
  
 • -m – operation(crypt/decrypt)
  
 • -i – defines the input options:
  
    • - for "crypt" mode: file/files/directories. Name can be given by mask: ?, *. To encrypt all contents of the current directory use "*" or "*.*";
  
    • - for "decrypt" mode: path and filename with previously encrypted data.
  
 • -o – output options:
  
    • - for "crypt" mode: name and path to the file with encrypted data;
  
    • - for "decrypt" mode: the path to the file/files/directories being restored.
  
 • -p – password (the maximum size is 16 bytes)
