#pragma once

#define FLTAG "FL"

#include <Arduino.h>
#include <list>
#include <tuple>


class FileList
{
public:
    static FileList* getInstance();
    void sortList(void);
    bool buildList(void);

    // File List
    //using namespace std;
    using records = std::tuple<String, String, int>;
    std::list<records> dirList;

    size_t usedBytes;
    size_t totalBytes;
    size_t freeBytes;

private:
    FileList();
    static FileList *instance;
};