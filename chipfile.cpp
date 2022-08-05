#include "chipfile.h"
#include "aes.h"
#include <sys/stat.h>
#ifndef __linux__
#include <direct.h>
#endif

#ifndef __linux__
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

using namespace std;

typedef struct strArgToTread
{
	std::list<std::string>* plsFilesPath;
	std::string *sPathFileToStore;
	std::string *sPassword;
	std::mutex *mLockList;
} *pstrArgToTread;


int iFlagStop = 0;
int iFlagRunning = 0;

int mkpath(std::string s)
{
    size_t pre=0,pos;
    std::string dir;
    int mdret;

    if(s[s.size()-1]!=PATH_SEP){
        // force trailing / so we can handle everything in loop
        s+=PATH_SEP;
    }

    while((pos=s.find_first_of(PATH_SEP,pre))!=std::string::npos){
        dir=s.substr(0,pos++);
        pre=pos;
        if(dir.size()==0) continue; // if leading / first time is 0 length
#ifndef __linux__
		if ((mdret = _mkdir(dir.c_str())) && errno != EEXIST) {
#else
		if ((mdret = mkdir(dir.c_str(), 0777)) && errno != EEXIST) {
#endif
            return mdret;
        }
    }
    return mdret;
}


char* LoadFile(std::string path, size_t & size)
{
	std::fstream file;
	file.open(path.c_str(), std::ios::in | std::ios::binary);
	if (file.good() == true)
	{

		file.seekg(0, std::ios_base::end);
		size = file.tellg();
		file.seekg(0, std::ios_base::beg);

		char* bufor = new char[size];
		file.read(bufor, size);

		file.close();
		return bufor;
	}
	std::cout << "Cannot find file: "<<path << std::endl;
	exit(2);
}

char* LoadFilePart(std::string path, size_t size)
{
	std::fstream file;
	file.open(path.c_str(), std::ios::in | std::ios::binary);
	if (file.good() == true)
	{
		char* bufor = new char[size];
		file.read(bufor, size);

		file.close();
		return bufor;
	}
	std::cout << "Cannot find file: "<<path << std::endl;
	exit(2);
}

void CreateFileFromBuffer(std::string path, char* data, size_t size)
{
	std::fstream file(path.c_str(), std::ios::out | std::ios::binary);
	file.write(data, size);
	file.close();
}

uint8_t* CryptedIV(Aes *mAesCipp, char *buf, int size, char* pWorkPassword, int sizePass)
{
	size_t outputSize;
	char keyIV[16];
	memset(keyIV, 0, 16);
	memcpy(keyIV, buf, size);
	char keyPass[16];
	memset(keyPass, 0, 16);
	memcpy(keyPass, pWorkPassword, sizePass);
	return (mAesCipp->CryptData(keyIV, 16, keyPass, 0, outputSize));
}

void ChipFiles (void * arg)
{
	iFlagRunning = 1;
	size_t SectionOfset = 0;
	size_t uiCounterIV = 0;
	pstrArgToTread ArgToTread;
	ArgToTread = (pstrArgToTread)arg;
	list<string> *lsFilesPath = ArgToTread->plsFilesPath;
    string *psPathFileToStore = ArgToTread->sPathFileToStore;
    string sPassword = ArgToTread->sPassword->data(); 
    mutex *mLockList = ArgToTread->mLockList;
    
    Aes *mAesCipp = new Aes;
    size_t outputSize = 0;
    
	uint8_t* pWorkPassword = CryptedIV(mAesCipp, (char*)sPassword.data(), sPassword.size(), (char*)sPassword.data(), sPassword.size());

	string sPathDirTmp;
	sPathDirTmp.clear();
	size_t PositionPathSep = psPathFileToStore->find_last_of(PATH_SEP);
	if (PositionPathSep != string::npos){
		sPathDirTmp = psPathFileToStore->substr(0,PositionPathSep + 1);
	}
	if (mkpath(sPathDirTmp.data()) == 0)
		cout  << "[Make New Folder]: " << sPathDirTmp.data() << endl;
	std::fstream file(psPathFileToStore->data(), std::ios::out | std::ios::binary);
    while(iFlagStop) {
		static string sPath;
		sPath.clear();
		mLockList->lock();
		if (!lsFilesPath->empty()) {
			sPath = lsFilesPath->front();
			lsFilesPath->pop_front();
		}
		mLockList->unlock();
		while(!sPath.empty())
		{
			if (sPath != *psPathFileToStore)
			{
				size_t size = 0;
				char* data = LoadFile(sPath, size);
			cout << "[Crypted File]: " << sPath.data() << " [File Size]: " << size; 
			
				uint8_t* pWorkIV = CryptedIV(mAesCipp, (char*)&uiCounterIV, sizeof(size_t), (char*)pWorkPassword, 16);

				uint8_t* ChippedData = mAesCipp->CryptData(data, size, (char*)pWorkPassword, pWorkIV, outputSize);

			cout << " [Crypted File Size]: " << outputSize << endl;

				size_t SectionSize = sizeof(size_t) + sizeof(size_t) + (sPath.size() + 1) + outputSize;
				SectionOfset += SectionSize;

				char* Buff = new char[SectionSize];
				memcpy(Buff, &SectionOfset, sizeof(size_t));	
				memcpy((Buff + sizeof(size_t)), &uiCounterIV, sizeof(size_t));	
				memcpy((Buff + sizeof(size_t) + sizeof(size_t)), sPath.data(), (sPath.size() + 1));	
				if (ChippedData != nullptr)
					memcpy((Buff + sizeof(size_t) + sizeof(size_t) + (sPath.size() + 1)), ChippedData, outputSize);	
				
				file.write(Buff, SectionSize);
				uiCounterIV++;
				delete[] Buff;
				delete[] ChippedData;
				delete[] pWorkIV;
				delete[] data;
			}
			sPath.clear();
		}
	}
	file.close();	

	
	
 	delete[] pWorkPassword;
	delete mAesCipp;
	iFlagRunning = 0;
}

int ChipFilesFromListStart (list<string> *plsFilesPath, mutex *mLockList, string *psArgPassword, string *psPathFlToStore)
{
	static struct strArgToTread ArgToTread;
	ArgToTread.plsFilesPath = plsFilesPath;
	ArgToTread.mLockList = mLockList;
	ArgToTread.sPassword = psArgPassword;
	ArgToTread.sPathFileToStore = psPathFlToStore;
	iFlagStop = 1;
	iFlagRunning = 0;
	
	thread thread(ChipFiles, &ArgToTread);
	thread.detach();

	return 0;
}

void ChipFilesFromListStop ()
{
	iFlagStop = 0;

	while(iFlagRunning);
}

void DeChipFile(string *psArgPassword, string *psArgPathFile, string *psPathToDecrypt)
{
	iFlagRunning = 1;
	size_t SectionOfset = 0;
	size_t uiCounterIV = 0;
    string sPassword = psArgPassword->data(); 
    string sPathFile = psArgPathFile->data();
	string sPathToDecrypt = psPathToDecrypt->data();
    if(sPathToDecrypt[sPathToDecrypt.size()-1]!=PATH_SEP){
        // force trailing / 
        sPathToDecrypt+=PATH_SEP;
    }
    
    Aes *mAesCipp = new Aes;
    size_t outputSize = 0;

	uint8_t* pWorkPassword = CryptedIV(mAesCipp, (char*)sPassword.data(), sPassword.size(), (char*)sPassword.data(), sPassword.size());

	std::fstream file;
	file.open(sPathFile.c_str(), std::ios::in | std::ios::binary);
	if (file.good() != true)
	{
		std::cout << "Cannot find Crypted file: "<<sPathFile << std::endl;
		exit(2);
	}
	file.seekg(0, std::ios_base::end);
	size_t CryptedFileSize = file.tellg();
	if (CryptedFileSize == 0)
	{
		file.close();
		std::cout << "Crypted file size = 0: "<<sPathFile << std::endl;
		exit(2);
	}
	file.seekg(0, std::ios_base::beg);
	size_t CryptedFileOffset = 0;


	do
	{
		size_t tmpSectionSize = 0;
		file.read((char*)&tmpSectionSize, sizeof(size_t));
		if (file.good() != true)
		{
			std::cout << "Crypted file Corrupted!: "<<sPathFile << std::endl;
			exit(2);
		}
		if (tmpSectionSize == 0) break; 
		tmpSectionSize -= CryptedFileOffset;
		char* tmpDatabuff = new char[tmpSectionSize - sizeof(size_t)];
		file.read(tmpDatabuff, (tmpSectionSize - sizeof(size_t)));
		if (file.good() != true)
		{
			std::cout << "Crypted file Corrupted!: "<<sPathFile << std::endl;
			exit(2);
		}
		CryptedFileOffset += tmpSectionSize;
		uint8_t* pWorkIVtmp = CryptedIV(mAesCipp, tmpDatabuff, sizeof(size_t), (char*)pWorkPassword, 16);
		int i = 0;
		while(*(tmpDatabuff + sizeof(size_t) + i) != 0) i++;
		char* decrypted = mAesCipp->DecryptData((uint8_t*)(&tmpDatabuff[sizeof(size_t) + i + 1]), (tmpSectionSize - 2*(sizeof(size_t)) - i - 1), (char*)pWorkPassword, pWorkIVtmp, outputSize);
		string sPathFileTmp = (string)(tmpDatabuff + sizeof(size_t));
		string sPathDirTmp;
		sPathDirTmp.clear();
#ifndef __linux__
		size_t PositionPathSep1 = sPathFileTmp.find_last_of(':');
		if (PositionPathSep1!= string::npos) {
			sPathFileTmp = sPathFileTmp.substr(PositionPathSep1 + 2);
		}
#endif
		size_t PositionPathSep = sPathFileTmp.find_last_of(PATH_SEP);
		if (PositionPathSep != string::npos){
			sPathDirTmp = sPathFileTmp.substr(0,PositionPathSep + 1);
		}
		if (mkpath((sPathToDecrypt + sPathDirTmp)) == 0)
			cout  << "[Make New Folder]: " << (sPathToDecrypt + sPathDirTmp).data() << endl;
		CreateFileFromBuffer((sPathToDecrypt + sPathFileTmp), (char*)decrypted, outputSize);
		cout << "[Decrypted File]: " << (sPathToDecrypt + sPathFileTmp).data() << " [File Size]: " << outputSize << endl;; 
		delete[] decrypted;
		delete[] pWorkIVtmp;
		delete[] tmpDatabuff;
	}while(CryptedFileOffset < CryptedFileSize);

	file.close();
 	delete[] pWorkPassword;
	delete mAesCipp;
	iFlagRunning = 0;
}

int DeChipFileStart (string *psArgPassword, string *psPathFile, string *psPathToDecrypt)
{
	iFlagStop = 1;
	iFlagRunning = 0;
	
	thread thread(DeChipFile, psArgPassword, psPathFile, psPathToDecrypt);
	thread.join();

	return 0;
}

void DeChipFileStop ()
{
	iFlagStop = 0;

	while(iFlagRunning);
}
