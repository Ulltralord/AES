#ifndef CHIPFILE_H_
#define CHIPFILE_H_

#include <string>
#include <list>
#include <thread>
#include <fstream>
#include <iostream>
#include <cstddef> 
#include <mutex>
#include <cstring>
#include "aes.h"

int ChipFilesFromListStart (std::list<std::string> *plsFilesPath, std::mutex *mLockList, std::string *psArgPassword, std::string *psPath);
void ChipFilesFromListStop ();
int DeChipFileStart (std::string *psArgPassword, std::string *psPathFile, std::string *psPathToDecrypt);
void DeChipFileStop ();
uint8_t* CryptedIV(Aes* mAesCipp, char* buf, int size, char* pWorkPassword, int sizePass);

#endif /* CHIPFILE_H_ */

