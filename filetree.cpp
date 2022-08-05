#include "filetree.h"
#ifndef __linux__
#include <io.h>
#endif

using namespace std;

#ifndef __linux__
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

typedef struct strArgToTread
{
	std::string sBeginPath;
	std::list<std::string>* plsFilesPath;
	std::mutex *mLockList;
} *pstrArgToTread;

void FolderProc (string sPath, string sMask, list<string> *lsDirs, list<string> *lsFilesPath, mutex *mLockList)
{
    struct _finddata_t data;
    intptr_t handle;

	handle = _findfirst((sPath + sMask).data(), &data);
	if(handle  != -1L) {
		do {
			if (data.attrib & _A_SUBDIR) {
				if (((string)data.name != "..") && ((string)data.name != ".")) 
					lsDirs->push_back(sPath + (string)data.name + PATH_SEP);
			}
			else {
				mLockList->lock();
				lsFilesPath->push_back(sPath + (string)data.name);
				mLockList->unlock();
			}

		} while (_findnext(handle, &data) == 0);
		_findclose(handle);
	}
}

void FileTree (void * arg)
{
	pstrArgToTread ArgToTread;
	ArgToTread = (pstrArgToTread)arg;
	list<string> *lsFilesPath = ArgToTread->plsFilesPath;
    string sBeginPath = ArgToTread->sBeginPath;
    mutex *mLockList = ArgToTread->mLockList;
    
    static list<string> lsDirs;
    lsDirs.clear();
    
    string sPath;
    sPath.clear();
    string sMask;
    sMask.clear();

    size_t PositionPathSep = sBeginPath.find_last_of(PATH_SEP);
    if (PositionPathSep != string::npos){
		sPath = sBeginPath.substr(0,PositionPathSep + 1);
		sMask = sBeginPath.substr(PositionPathSep + 1);
	}
	else
		sMask = sBeginPath;
		
    struct _finddata_t data;
    intptr_t handle;

	handle = _findfirst((sPath + sMask).data(), &data);
	if(handle  != -1L) {
		if ((data.attrib & _A_SUBDIR) && ((string)data.name == sMask)) { 
			lsDirs.push_back(sPath + sMask + PATH_SEP);
			sPath = sPath + sMask + PATH_SEP;
			sMask = "*.*";
		}
		else
			lsDirs.push_back(sPath);
		_findclose(handle);

	    do{
			sPath = lsDirs.front();
			lsDirs.pop_front();
			FolderProc(sPath, sMask, &lsDirs, lsFilesPath, mLockList);
		} while (!lsDirs.empty());
	}
}

int GetFileTree (string sBeginPath, list<string> *plsFilesPath, mutex *mLockList)
{
	static struct strArgToTread ArgToTread;
	ArgToTread.sBeginPath = sBeginPath;
	ArgToTread.plsFilesPath = plsFilesPath;
	ArgToTread.mLockList = mLockList;
	
	thread thread(FileTree, &ArgToTread);
	thread.join();

	return 0;

}
