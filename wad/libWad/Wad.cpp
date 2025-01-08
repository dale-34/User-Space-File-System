#include "Wad.h"

Wad::Wad(const string &path) {
    file.open(path, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        cout << "Can't Open File" << endl;
    }

    // Read Header
    char readMagic[5];
    readMagic[4] = '\0';

    file.read(readMagic, 4);
    file.read((char *) &numDescriptor, 4);
    file.read((char *) &offsetDescriptor, 4);
    magic = readMagic;

    // Initialize Root
    root = new Node();
    root->elementOffset = 0;
    root->elementLength = 0;
    root->name = "/";


    paths.insert(make_pair(root->name, root));
    stack<Node *> s;
    s.push(root);

    // Read Descriptors
    file.seekg(offsetDescriptor, ios::beg);

    Node *prev = root;
    string pathName = root->name;
    regex mapPattern("^E[0-9]M[0-9]/$");
    regex startPattern("_START");
    regex endPattern("_END");

    int count = 0;
    int mapCount = 0;

    while (count < numDescriptor) {
        Node *temp = new Node();
        char readName[9];
        readName[8] = '\0';
        streampos currentPosition = file.tellg();
        file.read((char *) &temp->elementOffset, 4);
        file.read((char *) &temp->elementLength, 4);
        file.read(readName, 8);
        temp->name = readName;
        temp->position = currentPosition;
        temp->name += "/";

        if (mapCount == 10) {
            mapCount = 0;
            s.pop();
            pathName = pathName.erase(pathName.size() - 5);
            prev = s.top();
        }

        if (regex_match(s.top()->name, mapPattern) && mapCount < 10) {
            // E#M# Content Tracking
            string newPath = pathName + temp->name;
            paths.insert(make_pair(newPath, temp));
            prev->children.push_back(temp);
            mapCount++;
        } else if (regex_search(temp->name, startPattern) || regex_match(temp->name, mapPattern)) {
            // Starting or E#M# Markers Tracking
            s.push(temp);
            pathName += temp->name;
            if(checkNameMarker(pathName, "start")) {
                pathName = pathName.erase(pathName.size() - 7) + "/";
            }
            paths.insert(make_pair(pathName, temp));
            prev->children.push_back(temp);
            prev = temp;
        } else if (regex_search(temp->name, endPattern)) {
            // End Markers Tracking
            string find;
            if (regex_search(s.top()->name, startPattern)) {
                find = s.top()->name.substr(0, s.top()->name.size() - 7);
                s.pop();
            } else {
                while (!regex_search(s.top()->name, startPattern)) {
                    s.pop();
                    find = s.top()->name.substr(0, s.top()->name.size() - 7);
                }
                s.pop();
            }
            for (auto & i : paths) {
                if (i.second->name == prev->name) {
                    endPaths.insert(make_pair(i.first, currentPosition));
                }
            }
            prev = s.top();
            size_t pos = pathName.find(find);
            if (pos != string::npos) {
                pathName.erase(pos);
            }
        } else {
            // File Content Tracking
            if (prev->name == "/") {
                paths.insert(make_pair(pathName+temp->name, temp));
                prev->children.push_back(temp);
            } else {
                s.push(temp);
                prev->children.push_back(temp);
                paths.insert(pair<string, Node *>(pathName+temp->name, temp));
            }
        }
        count++;
    }
    root->position = file.tellg();
}

Wad *Wad::loadWad(const string &path) {
    Wad* wadObject = new Wad(path);
    return wadObject;
}

string Wad::getMagic() {
    return magic;
}

bool Wad::isContent(const string &path) {
    string paramPath = path;
    if (path == "/" || path.empty()) {
        return false;
    }
    if (paramPath.back() != '/') {
        paramPath += '/';
    }
    auto it = paths.find(paramPath);
    if (it != paths.end()) {
        if(checkMapMarker(it->second->name) || checkNameMarker(it->second->name, "both")) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

bool Wad::isDirectory(const string &path) {
    string paramPath = path;
    if (path == "/") {
        return true;
    }
    if (paramPath.back() != '/') {
        paramPath += '/';
    }
    auto it = paths.find(paramPath);
    if (it != paths.end()) {
        if(checkMapMarker(it->second->name) || checkNameMarker(it->second->name, "both")) {
            return true;
        }
    }
    return false;
}

int Wad::getSize(const string &path) {
    string pathName = path;
    if (pathName.back() != '/') {
        pathName += "/";
    }
    if (isDirectory(pathName)) {
        return -1;
    }
    if (isContent(pathName)) {
        string find = path + "/";
        auto it = paths.find(find);
        if (it != paths.end()) {
            return it->second->elementLength;
        }
    }
    return -1;
}

int Wad::getContents(const string &path, char *buffer, int length, int offset) {
    string pathName = path;
    int bytesRead = 0;
    if (!isContent(path)) {
        return -1;
    }
    if (pathName.back() != '/') {
        pathName += "/";
    }
    auto it = paths.find(pathName);
    if (it != paths.end()) {
        unsigned int start = it->second->elementOffset + offset;
        file.seekg(start, ios::beg);
        if (offset < it->second->elementLength) {
            if (length > it->second->elementLength) {
                bytesRead = it->second->elementLength - offset;
            } else {
                bytesRead = length;
            }
            file.read(buffer, bytesRead);
        }
    }
    return bytesRead;
}

int Wad::getDirectory(const string &path, vector<string> *directory) {
    string suffix = "_START";
    string pathName = path;
    int numChildren;

    if (!isDirectory(path)) {
        return -1;
    }
    if (pathName.back() != '/') {
        pathName += "/";
    }

    auto it = paths.find(pathName);
    if (it != paths.end()) {
        numChildren = it->second->children.size();
        for (int i = 0; i < numChildren; i++) {
            string name = it->second->children[i]->name;
            if(checkNameMarker(name, "start")) {
                name.erase(name.size() - suffix.size());
            }
            name.pop_back();
            directory->push_back(name);
        }
        return numChildren;
    }
    return -1;
}

void Wad::createDirectory(const string &path) {
    string wholePath;
    string newDirectory = path;
    string prevDirectory;

    if (newDirectory.back() == '/') {
        wholePath = path;
        newDirectory.pop_back();
    } else {
        wholePath = path + "/";
    }

    size_t pathPos = newDirectory.find_last_of('/');
    if (pathPos != string::npos) {
        prevDirectory = newDirectory.substr(0, pathPos + 1);
        newDirectory = newDirectory.substr(pathPos + 1);
    }

    if (newDirectory.size() > 2) {
        return;
    }

    string start = newDirectory + "_START";
    string end = newDirectory + "_END";

    start += (start.length() <= 8) ? string(8 - start.length(), '\0') : "\n";
    end += (end.length() < 8) ? string(8 - end.length(), '\0') : "\n";

    auto it = paths.find(prevDirectory);
    if (it != paths.end()) {
        vector<char> contents;
        Node *newDir = new Node();
        uint32_t dirLength = 0, dirOffset = 0;
        char ch;

        int pos = it->second->position + 16;
        if (it->second->children.size() != 0) {
            auto itTwo = endPaths.find(prevDirectory);
            if (itTwo != endPaths.end()) {
                pos = itTwo->second;
            }
        }
        file.seekg(pos, ios::beg);
        if (prevDirectory != "/") {
            while (file.get(ch)) {
                contents.push_back(ch);
            }
            if (file.eof()) {
                file.clear();
                file.seekg(pos, ios::beg);
            }
        } else {
            file.seekg(it->second->position, ios::beg);
        }

        newDir->position = file.tellg();
        file.write((char *) &dirOffset, 4);
        file.write((char *) &dirLength, 4);
        file.write(start.c_str(), start.size());
        endPaths.insert(make_pair(wholePath, file.tellg()));

        file.write((char *) &dirOffset, 4);
        file.write((char *) &dirLength, 4);
        file.write(end.c_str(), end.size());
        file.write(contents.data(), contents.size());


        newDir->name = start + "/";
        newDir->elementLength = dirLength;
        newDir->elementOffset = dirOffset;

        paths.insert(make_pair(wholePath, newDir));
        it->second->children.push_back(newDir);

        numDescriptor += 2;
        file.seekg(4, ios::beg);
        file.write((char *) &numDescriptor, sizeof(numDescriptor));

        auto itTwo = endPaths.find(prevDirectory);
        if (itTwo != endPaths.end()) {
            itTwo->second += 32;
        }
    }
}

void Wad::createFile(const string &path) {
    string wholePath;
    string newFile = path;
    string prevDirectory;

    if (newFile.back() == '/') {
        wholePath = path;
        newFile.pop_back();
    } else {
        wholePath = path + "/";
    }

    size_t pathPos = newFile.find_last_of('/');
    if (pathPos != string::npos) {
        prevDirectory = newFile.substr(0, pathPos + 1);
        newFile = newFile.substr(pathPos + 1);
    }

    // Validate file
    if (newFile.size() > 8 || checkNameMarker(path, "both") || checkMapMarker(path)) {
        return;
    }

    string insert = newFile;
    insert += (newFile.length() < 8) ? string(8 - newFile.length(), '\0') : "";

    auto it = paths.find(prevDirectory);
    if (it != paths.end()) {
        vector<char> contents;
        char ch;
        int pos = it->second->position+16;

        // Define position after children content of directory
        if (it->second->children.size() != 0) {
            auto itTwo = endPaths.find(prevDirectory);
            if (itTwo != endPaths.end()) {
                pos = itTwo->second;
            }
        }

        file.seekg(pos, ios::beg);
        if (prevDirectory != "/") {
            while (file.get(ch)) {
                contents.push_back(ch);
            }
            if (file.eof()) {
                file.clear();
                file.seekg(pos, ios::beg);
            }
        } else {
            file.seekg(it->second->position, ios::beg);
        }

        uint32_t fileLength = 0, fileOffset = 0;

        // Create new node for file
        Node* addFile = new Node();
        addFile->name = newFile + "/";
        paths.insert(make_pair(wholePath, addFile));
        it->second->children.push_back(addFile);
        addFile->elementOffset = fileOffset;
        addFile->elementLength = fileLength;
        addFile->position = file.tellg();

        // Write file descriptor
        file.write((char*)&fileOffset, 4);
        file.write((char*)&fileLength, 4);
        file.write(insert.c_str(), insert.size());
        file.write(contents.data(), contents.size());
        numDescriptor++;

        // Rewrite descriptor header
        file.seekg(4, ios::beg);
        file.write((char*)&numDescriptor, sizeof(numDescriptor));

        // Update positions in map
        auto itTwo = endPaths.find(prevDirectory);
        if (itTwo != endPaths.end()) {
            string name = itTwo->first;
            while (name != "/") {
                auto itThree = endPaths.find(name);
                if (itThree != endPaths.end()) {
                    itThree->second += 16;
                }
                name = name.substr(0, name.size() - 3);
            }
        }
    }
}

int Wad::writeToFile(const string &path, const char *buffer, int length, int offset) {
    string pathName = path;
    if (!isContent(path)) {
        return -1;
    }
    if (pathName.back() != '/') {
        pathName += "/";
    }

    char toWrite[length+1];
    toWrite[length] = '\0';
    memcpy(toWrite, buffer+offset, length);

    auto it = paths.find(pathName);
    if (it != paths.end()) {
        if (it->second->elementLength != 0) {
            return 0;
        }
        unsigned int start = offsetDescriptor;
        vector<char> contents;
        char ch;

        file.seekg(start, ios::beg);
        while (file.get(ch)) {
            contents.push_back(ch);
        }
        if (file.eof()) {
            file.clear();
            file.seekg(start, ios::beg);
        }

        // Redefine file's offset and length
        it->second->elementOffset = file.tellg();
        it->second->elementLength = length;

        // Write lump data
        file.write(toWrite, sizeof(toWrite));
        offsetDescriptor = file.tellg();

        // Update path positions
        for (auto &i: paths) {
            i.second->position += length + 1;
        }
        for (auto &i: endPaths) {
            i.second += length + 1;
        }

        // Rewrite descriptors
        file.write(contents.data(), contents.size());
        file.seekg(8, ios::beg);
        file.write((char*)&offsetDescriptor, sizeof(offsetDescriptor));

        // Rewrite File Offset and Length
        file.seekg(it->second->position, ios::beg);
        file.write((char*)&it->second->elementOffset, 4);
        file.write((char*)&it->second->elementLength, 4);
    }
    return length;
}

bool Wad::checkNameMarker(const string &marker, string type) {
    string start = "_START/";
    string end = "_END/";

    size_t foundStart = marker.find(start);
    size_t foundEnd = marker.find(end);
    if (type == "start") {
        if (foundStart != string::npos) {
            return true;
        } else {
            return false;
        }
    } else {
        if (foundStart != string::npos || foundEnd != string::npos) {
            return true;
        }
    }
    return false;
}

bool Wad::checkMapMarker(const string &marker) {
    regex pattern("E[0-9]M[0-9]");
    if (regex_search(marker, pattern)) {
        return true;
    }
    return false;
}

Wad::~Wad() {
    for (auto it = paths.begin(); it != paths.end(); ++it) {
        delete it->second;
    }
    paths.clear();
    file.close();
}
