    #pragma once

    #define HWTAG "MYFS"


    #include <Arduino.h>
    #include "WebServer.h"

    void handleFSExplorer(void);
    void handleFSExplorerCSS(void);
    void handleContent(const uint8_t * image, size_t size, const char * mime_type);
    void deleteFileOrDir(const String &path);
    void deleteDirectory(const char* path);
    void formatFS(void);
    void handleUpload(void);
    String generateHTML(void);
    bool handleFile(String &&path);
    void sendResponse(void);
    const String formatBytes(size_t const& bytes);
    String getFilelist(void);
    bool handleList(void);

    class HandleWebserver
    {
    public:
        static HandleWebserver* getInstance();

    
        void setupActions(void);
    
        void handleWeb(void);
        static bool isConnected;
        
    private:
        HandleWebserver();
        static HandleWebserver *instance;
        

    };