#ifndef FILETREE_H_
#define FILETREE_H_

#ifdef __linux__
#include "findfirst.h"
#include "spec.h"
#endif
#include <string>
#include <list>
#include <thread>
#include <fstream>
#include <iostream>
#include <cstddef> 
#include <mutex>

int GetFileTree (std::string sBeginPath, std::list<std::string>* plsFilesPath, std::mutex *mLockList);


#endif /* FILETREE_H_ */
