#ifndef EXTERNALMEDIA_H
#define EXTERNALMEDIA_H
#include <string>
#include <vector>
/*
void ClearAllVar();
int ChangedAfterLastRead();
int ReadMountsFile();
void PrintFile();*/
int ExtMediaCount(bool refresh = true);
const std::vector<std::string> ExtMediaMountPointList();

int FileSystemSizeMB(const std::string&);
#endif

