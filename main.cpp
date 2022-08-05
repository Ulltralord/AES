#include <iostream>
#include <string>
#include <thread>
#include <list>
#include <mutex>
#include "filetree.h"
#include "chipfile.h"
#include <cstring>
#include "aes.h"
//#include "chipfile.cpp"

using namespace std;

std::string FindParam(int argc, char* argv[], const char* param)
{
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], param) == 0)
		{
			if (i + 1 <= argc - 1)
			{
				return std::string(argv[i + 1]);
			} else {
				std::cout << "Error while parsing command." << std::endl;
				exit(1);
			}
		}
	}
	return std::string("");
}


int main(int argc, char **argv)
{
	//----------------------------------------------------------------------
	//Print help
	//----------------------------------------------------------------------
	if (argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0)) {
		std::cout << "This program aes crypt file/s or folder to one crypted file " << std::endl;
		std::cout << "or aes decrypt file/s or folders from one crypted file. " << std::endl << std::endl;
		std::cout << "Usage: aes.exe [-m <mode>] [-i <path to file/s>] [-o <path to file/dir>] [-p <password>] " << std::endl << std::endl;
		std::cout << "Parameters description:" << std::endl;
		std::cout << "  -h\t Prints this guide." << std::endl;
		std::cout << "  -t\t Prints test results." << std::endl;
		std::cout << "  -m\t Specifies the operation mode. Valid values: crypt, decrypt." << std::endl;
		std::cout << "  -i\t Specifies the input: " << std::endl;
		std::cout << "       for crypt mode: file/s or folders. May define name mask: ?, *. Crypt all files in folder: \"*\" or \"*.*\"" << std::endl;
		std::cout << "       for decrypt mode: crypted file." << std::endl;
		std::cout << "  -o\t Specifies the output: " << std::endl;
		std::cout << "       for crypt mode: output crypted file." << std::endl;
		std::cout << "       for decrypt mode: output folder. If not set - all files will decrypt in destination folder." << std::endl;
		std::cout << "  -p\t Specifies the password to use. Max 16 bytes." << std::endl;
		std::cout << std::endl;
		exit(0);
	}

	//----------------------------------------------------------------------
	//Print test
	//----------------------------------------------------------------------

	if (argc == 1 || (argc == 2 && strcmp(argv[1], "-t") == 0)) {
		size_t outputSize;
		std::cout << "Testing AES" << std::endl;
		const unsigned char data[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
		std::cout << "PLAINTEXT:" << std::endl;
		for (int i = 0; i < sizeof data; i++) printf(" %x", data[i]);
		std::cout << std::endl;
		char key[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
		std::cout << "KEY:" << std::endl;
		for (int i = 0; i < sizeof key; i++) printf(" %x", key[i]);
		char buf[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		Aes* mAestest = new Aes;
		uint8_t* result = mAestest->CryptData((const char*)data, sizeof data, key, 0, outputSize);
		std::cout << std::endl;
		std::cout << "ENCRYPT:" << std::endl;
		for (int i = 0; i < 16; i++) printf(" %x", result[i]);		
		std::cout << std::endl;
		std::cout << "DECRYPT:" << std::endl;
		char* result2 = mAestest->DecryptData(result, 16, key, 0, outputSize);
		for (int i = 0; i < 16; i++) printf(" %x", (unsigned char)result2[i]);
		std::cout << std::endl;

		delete[] result2;
		delete[] result;
		return 0;
	}

	//----------------------------------------------------------------------
	//Check params
	//----------------------------------------------------------------------
	std::string passwdStr = FindParam(argc, argv, "-p");
	bool ifPasswd = !passwdStr.empty();

	std::string inputStr = FindParam(argc, argv, "-i");
	bool inputCheck = !inputStr.empty();

	std::string outputStr = FindParam(argc, argv, "-o");
	bool outputCheck = !outputStr.empty();

	std::string modeStr = FindParam(argc, argv, "-m");
	bool mode = !modeStr.empty();

	//----------------------------------------------------------------------
	//Handle password & init vector & paths
	//----------------------------------------------------------------------

	if (mode == false)
	{
		std::cout << "Please specify the operation mode." << std::endl;
		exit(0);
	} else {
		if (modeStr != "crypt" && modeStr != "decrypt"){
			std::cout << "Valid values for -m parameter are: crypt, decrypt." << std::endl;
			exit(0);
		}
	}

	if (inputCheck == false)
	{
		std::cout << "Please specify the input" << std::endl;
		exit(0);
	}

	if (outputCheck == false && modeStr == "crypt")
	{
		std::cout << "Please specify the input and output." << std::endl;
		exit(0);
	}

	if (ifPasswd)
	{
		if (passwdStr.size() > 16) {
			std::cout << "Password is too long." << std::endl;
			exit(0);
		}
	} else {
			std::cout << "You have to enter the password for decryption." << std::endl;
			exit(0);
	}


    static string sBeginPath = inputStr;
    static string sFileToSavePath = outputStr;
    static string sPathToDecrypt = outputStr;
    static string sPassword = passwdStr;
    if (modeStr == "crypt"){
		static list<string> lsFilesToChip;
		lsFilesToChip.clear();
		static list<string> *plsFilesToChip = &lsFilesToChip;
		plsFilesToChip->clear();
		static mutex mLockList;

		ChipFilesFromListStart (&lsFilesToChip, &mLockList, &sPassword, &sFileToSavePath);
		GetFileTree(sBeginPath, &lsFilesToChip, &mLockList);
		while (!lsFilesToChip.empty());
		ChipFilesFromListStop();
		std::cout << "Done." << std::endl;
	}
    
    if (modeStr == "decrypt"){
		DeChipFileStart (&sPassword, &sBeginPath, &sPathToDecrypt);
		std::cout << "Done." << std::endl;
	}
	
	return 0;
}

