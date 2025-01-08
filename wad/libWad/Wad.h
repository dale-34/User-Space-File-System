#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <stack>
#include <regex>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#pragma once
using namespace std;


struct Node {
    vector<Node*> children;
    uint32_t elementOffset;
    uint32_t elementLength;
    string name;
    int position;
};

class Wad {
private:
    Node* root = nullptr;
    map<string, Node*> paths;
    map<string, streampos> endPaths;
    fstream file;
    string magic;
    uint32_t numDescriptor;
    uint32_t offsetDescriptor;
    Wad(const string &path);
public:
    static Wad* loadWad(const string &path);
    string getMagic();
    bool isContent(const string &path);
    bool isDirectory(const string &path);
    int getSize(const string &path);
    int getContents(const string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const string &path, vector<string> *directory);
    void createDirectory(const string &path);
    void createFile(const string &path);
    int writeToFile(const string &path, const char *buffer, int length, int offset = 0);
    bool checkMapMarker(const string &marker);
    bool checkNameMarker(const string &marker, string type);
    ~Wad();
};