#include <Arduino.h>
#include "configuration.h"

#if defined(WITH_WIFI)



#include "esp_log.h"


#include "html_content.h"

#include <WiFi.h>

#include <ESPmDNS.h>

#include <list>
#include <FS.h>
#include <LittleFS.h>
#include "Languages.h"
#include <nvs_flash.h>


#include "handleWebserver.h"
#include "fileList.h"
#include "playSound.h"


extern void blinkLED(int count, int dly);

WebServer server(80);

static FileList *fl = FileList::getInstance();
static PlaySound *ps = PlaySound::getInstance();

bool HandleWebserver::isConnected = false;
HandleWebserver* HandleWebserver::instance = 0;

HandleWebserver *HandleWebserver::getInstance() {
  if (!instance)
  {
      instance = new HandleWebserver();
  }
  return instance;
}

HandleWebserver::HandleWebserver()
{
  
}

// Returns list of files in LittleFS of ESP32
String getFilelist(void) {
  ESP_LOGI(HWTAG,"getFileList 1 %ld\n",millis());
  ESP_LOGI(HWTAG,"got file list\n");
  String result="<br>Verwendet:" + formatBytes(fl->usedBytes) + "<br>";
  result += "<br>Gesamt: " + formatBytes(fl->totalBytes) + "<br>";
  result += "<br>Frei: " + formatBytes(fl->freeBytes) + "<br>";
  ESP_LOGI(HWTAG,"add files to website\n");
  for (auto& t : fl->dirList) {
    result += "<br>Ordner: " +  get<0>(t) + " Name: " + get<1>(t) + " Größe: " + formatBytes(get<2>(t)) + "<br>";
  }
   ESP_LOGI(HWTAG,"getFileList 2 %ld\n",millis());
  return result;
}


// generates index.html file
String generateHTML(const char *index_html) {
  String html(index_html);
  html.replace("%VERSION%", "2.0");
  html.replace("%BATTERIE%", String(ps->getBatterie()));
  html.replace("%VOLUME%", String(ps->getVolume()));
  html.replace("%FILES%", getFilelist());
  return html;
}


// Setting Webhandlers
void HandleWebserver::setupActions() {
  server.on("/fs", handleFSExplorer);
  server.on("/fsstyle.css", handleFSExplorerCSS);

  server.on("/sanduhr", []() {handleContent(p_bluespinner,sizeof(p_bluespinner),IMAGE_GIF);});

  server.on("/format", formatFS);
  server.on("/upload", HTTP_POST, sendResponse, handleUpload);

  server.on("/", []() {
       server.send(200, TEXT_HTML, generateHTML(index_html));
  });

  server.on("/start", HTTP_GET, []() {
      isConnected=false;
      //currentMillis=millis()+LOOPCNTMAX;
      server.send(200, "text/ascii", "Klangbox gestartet");
  });


  // Not found handler checks weather a file is requested
  server.onNotFound([]() {
    if (!handleFile(server.urlDecode(server.uri())))
      server.send(404, TEXT_PLAIN, "FileNotFound");
    });
}


// Creates a list of files for the Webpage
bool handleList() {   
  ESP_LOGI(HWTAG,"handleList 1 %ld\n",millis());

  if(!fl->buildList())
    return false;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  uint8_t jsonoutteil = 0;
  bool jsonoutfirst = true;
  String temp = "[";
  for (auto& t : fl->dirList) {
    if (temp != "[") temp += ',';
    temp += "{\"folder\":\"";
    temp += get<0>(t);
    temp += "\",\"name\":\"";
    temp += get<1>(t);
    temp += "\",\"size\":\"";
    temp += formatBytes(get<2>(t));
    temp += "\"}";
    jsonoutteil++;
    if ( jsonoutteil > 2 ) 
    {
      jsonoutteil = 0;
      if ( jsonoutfirst )
      {
        jsonoutfirst = false;
        server.send(200, "application/json", temp);
      }
      else
      {
        server.sendContent(temp);
      }
      ESP_LOGI(HWTAG,"%s", temp.c_str());
      temp = "";
      delay(0);
    } 
  }
  if(fl->dirList.size())
    temp += ",";
  temp += "{\"fl->usedBytes\":\"";
  temp += formatBytes(fl->usedBytes);   // used bytes in filesystem
  temp += "\",\"fl->totalBytes\":\"";
  temp += formatBytes(fl->totalBytes);  // total bytes in filesystem
  temp += "\",\"fl->freeBytes\":\""; 
  temp += formatBytes(fl->freeBytes);   // free bytes in filesystem
  temp += "\"}]";   

  server.sendContent(temp);
  server.sendContent("");

  ESP_LOGI(HWTAG,"%s", temp.c_str());

  temp = "";
  ESP_LOGI(HWTAG,"handleList 2 %ld\n",millis());
  return true;
}


// Delete direktory in filesystem
void deleteDirectory(const char* path) {
  ESP_LOGI(HWTAG,"deleteDirectory <%s>\n",path);
  if (LittleFS.exists(path)) {
    File dir = LittleFS.open(path);
    
    if (dir.isDirectory()) {
      File file = dir.openNextFile();
      
      // Solange es Dateien gibt, iteriere durch sie
      while (file) {
        String filePath = String(path) + "/" + file.name();
        
        if (file.isDirectory()) {
          // Wenn es ein Unterverzeichnis ist, rekursiv löschen
          deleteDirectory(filePath.c_str());
        } else {
          // Wenn es eine Datei ist, löschen
          file.close();
          LittleFS.remove(filePath.c_str());
          ESP_LOGI(HWTAG,"Datei gelöscht: ");
          ESP_LOGI(HWTAG,"%s",filePath.c_str());
        }
        
        file = dir.openNextFile();  // Nächste Datei
      }
      
      // Verzeichnis löschen
      LittleFS.rmdir(path);
      ESP_LOGI(HWTAG,"Verzeichnis gelöscht: ");
      ESP_LOGI(HWTAG,"%s", path);
    }
  } else {
    ESP_LOGI(HWTAG,"Verzeichnis existiert nicht: ");
    ESP_LOGI(HWTAG,"%s", path);
  }
}


// Delete file or direktory in filesystem
void deleteFileOrDir(const String &path) {
  ESP_LOGI(HWTAG,"deleteFileOrDir <%s>\n",path.c_str());
  if (!LittleFS.remove(path))

    deleteDirectory(path.c_str());
  fl->dirList.clear();
}


// Reads in filesystem and streams it to the client
bool handleFile(String &&path) {
  ESP_LOGI(HWTAG,"LittleFS handleFile: ");
  ESP_LOGI(HWTAG,"%s\n", path.c_str());
  if (path.endsWith("/")) path += "index.html";
  if (path.endsWith("favicon.ico")) path="/_favicon.ico";
  if ( LittleFS.exists(path) )
  {
    File f = LittleFS.open(path, "r");
    if(!f)
    {
      ESP_LOGI(HWTAG,"Fehler beim lesen des Files: ");
      ESP_LOGI(HWTAG,"%s",path.c_str());
      return false;
    }
    else
    {
      String mime_type;
      if(path.endsWith(".htm") || path.endsWith(".html"))
          mime_type = "text/html";
      else if(path.endsWith(".jpg") || path.endsWith(".jpeg"))
          mime_type = "image/jpeg";
      else if(path.endsWith(".png"))
        mime_type = "image/png";
      else if(path.endsWith(".ico"))
        mime_type = "image/x-icon";
      else if(path.endsWith(".bmp"))
        mime_type = "image/bmp";
      else if(path.endsWith(".gif"))
        mime_type = "image/gif";
      else if(path.endsWith(".css"))
        mime_type = "text/css";
      else if(path.endsWith(".pdf"))
        mime_type = "application/pdf";
      else if(path.endsWith(".txt"))
        mime_type = "text/plain";
      else if(path.endsWith(".json"))
        mime_type = "application/json";
      else if(path.endsWith(".mp3"))
        mime_type = "audio/mpeg";
      else if(path.endsWith(".js"))
        mime_type = "text/javascript";
      else
        mime_type = "application/octet-stream";

      //ESP_LOGI(HWTAG,mime_type.c_str());

      server.streamFile(f, mime_type);
      f.close(); 
      return true;
    }
  }
  else
  {
    return false;
  }
}


// Write file intp filesystem
void handleUpload() { 
  static File fsUploadFile;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (upload.filename.length() > 31) {  // Dateinamen kürzen
      upload.filename = upload.filename.substring(upload.filename.length() - 31, upload.filename.length());
    }
    fsUploadFile = LittleFS.open(server.arg(0) + "/" + server.urlDecode(upload.filename), "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
     fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    fsUploadFile.close();
    fl->dirList.clear();
  }

}


// Format filesystem
void formatFS() {
  LittleFS.format();
  ps->initFS();
  fl->dirList.clear();
  server.sendHeader("Location", "/");
  server.send(303, "message/http");
}


// Send response
void sendResponse() {
  server.sendHeader("Location", "fs");
  server.send(303, "message/http");
}


// Format bytes
const String formatBytes(size_t const& bytes) {                                        // lesbare Anzeige der Speichergrößen
  return bytes < 1024 ? static_cast<String>(bytes) + " Byte" : bytes < 1048576 ? static_cast<String>(bytes / 1024.0) + "KB" : static_cast<String>(bytes / 1048576.0) + "MB";
}


// Filesystem explorer
void handleFSExplorer() {

  String message;
  if (server.hasArg("new")) 
  {
    String folderName {server.arg("new")};
    for (auto& c : {34, 37, 38, 47, 58, 59, 92}) for (auto& e : folderName) if (e == c) e = 95;    // Ersetzen der nicht erlaubten Zeichen
    folderName = "/" + folderName;
    ESP_LOGI(HWTAG,"mkdir: %s\n",folderName);
    if(!LittleFS.mkdir(folderName)) {
      ESP_LOGI(HWTAG,"mkdir error");
    } 
    fl->dirList.clear();
    sendResponse();
  }
  else if (server.hasArg("sort")) 
  {
    handleList();
  }
  else if (server.hasArg("delete")) 
  {
    deleteFileOrDir(server.arg("delete"));
    sendResponse();
  }
  else
  {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    message =  "<!DOCTYPE HTML>";
    message += "<html lang=\"";
    message += LANG_HTMLLANG;
    message += "\">\n";
    message += "<head>";
    message += "<meta charset=\"UTF-8\">";
    message += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    message += "<link rel=\"stylesheet\" href=\"fsstyle.css\">";
    message += "<title>";
    message += LANG_EXPLORER;
    message += "</title>\n";
    message += "<script>";
    message += "const LANG_FORMATCONF = \"" LANG_FORMATCONF "\";\n";
    message += "const LANG_NOSPACE = \"" LANG_NOSPACE "\";\n";
    message += "const LANG_FILESIZE= \"" LANG_FILESIZE "\";\n";
    message += "const LANG_SURE= \"" LANG_SURE "\";\n";
    message += "const LANG_FSUSED= \"" LANG_FSUSED "\";\n";
    message += "const LANG_FSTOTAL= \"" LANG_FSTOTAL "\";\n";
    message += "const LANG_FSFREE= \"" LANG_FSFREE "\";\n";
    
    server.send(200, TEXT_HTML, message);
    delay(0);
    server.sendContent_P(fshtml1);
    delay(0);
    message = ("<h2>" LANG_EXPLORER "</h2>");
    server.sendContent(message);
    server.sendContent_P(fshtml2);
    delay(0);
    message = "<input name=\"new\" placeholder=\"" LANG_FOLDER "\"";    
    //message += " pattern=\"[^\x22/%&\\:;]{1,31}\" title=\"";
    message += " title=\"";
    message += LANG_CHAR_NOT_ALLOW;
    message += "\" required=\"\">";
    message += "<button>Create</button>\n";
    message += "</form>";
    message += "<main></main>\n";
    message += "<form action=\"/format\" method=\"POST\">\n";            
    message += "<button class=\"buttons\" title=\"" LANG_BACK "\" id=\"back\" type=\"button\" onclick=\"window.location.href=\'\\/\'\">&#128281; " LANG_BACK "</button>\n";
    message += "<button class=\"buttons\" title=\"Format LittleFS\" id=\"btn\" type=\"submit\" value=\"Format LittleFS\">&#10060;Format LittleFS</button>\n";
    message += "</form>\n";
    message += "</body>\n";
    message += "</html>\n";
    server.sendContent(message);
    message = "";
    delay(0);
    server.sendContent("");
    delay(0);
  }

}


// Sends stylesheet to client
void handleFSExplorerCSS() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  delay(0);
  server.send_P(200, TEXT_CSS, fsstyle1);
  delay(0);
  server.sendContent_P(fsstyle2);
  delay(0);
  server.sendContent("");  
  delay(0);
}


// Upload file to filesystem
void handleContent(const uint8_t * image, size_t size, const char * mime_type) {
  uint8_t buffer[512];
  size_t buffer_size = sizeof(buffer);
  size_t sent_size = 0;

  server.setContentLength(size);
  server.send(200, mime_type, "");

  while (sent_size < size) {
    size_t chunk_size = min(buffer_size, size - sent_size);
    memcpy_P(buffer, image + sent_size, chunk_size);
    server.client().write(buffer, chunk_size);
    sent_size += chunk_size;
    delay(0);

    //ESP_LOGI(HWTAG,"sendContent: %i byte : %i byte of %i byte\n", chunk_size, sent_size,size );

  }
  server.sendContent("");
  delay(0);
}


// Callback-Funktion for WiFi-Events
void WiFiEvent(WiFiEvent_t event) {
   ESP_LOGI(HWTAG,"[WiFiEvent] Event: %d\n", event);
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      ESP_LOGI(HWTAG,"Ein Client hat sich verbunden!\n");
      ESP_LOGI(HWTAG,"Aktuelle Anzahl der Clients: %d\n", WiFi.softAPgetStationNum());
      HandleWebserver::isConnected=true;
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      ESP_LOGI(HWTAG,"Ein Client hat sich identifiziert.\n");
      ESP_LOGI(HWTAG,"Aktuelle Anzahl der Clients: %d\n", WiFi.softAPgetStationNum());
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      ESP_LOGI(HWTAG,"Ein Client hat sich getrennt!\n");
      ESP_LOGI(HWTAG,"Aktuelle Anzahl der Clients: %d\n", WiFi.softAPgetStationNum());
      HandleWebserver::isConnected=false;
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      ESP_LOGI(HWTAG,"Accesspoint beendet!\n");
      break;
    default:
      ESP_LOGI(HWTAG,"[WiFiEvent] Event: %d\n", event);
      break;
  }
}


// Displays Webpage if client connects withing 45 seconds to server
void HandleWebserver::handleWeb() {
    unsigned long currentMillis;

    ESP_LOGI(HWTAG,"initWebServer\n");
    isConnected=false;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS-Partition muss gelöscht und neu initialisiert werden
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    WiFi.mode(WIFI_AP);
    WiFi.softAP(APNAME);
    WiFi.onEvent(WiFiEvent);
    //dnsServer.start(53, "*", WiFi.softAPIP());

    delay(500);

    ESP_LOGI(HWTAG,"status=%d\n",WiFi.status());

    setupActions(); // 

    ESP_LOGI(HWTAG,"setupActions finished\n");

    server.begin();
    if (!MDNS.begin(APNAME)) { // APNAME ist der Hostname
        ESP_LOGI(HWTAG,"mDNS konnte nicht gestartet werden\n");
        return;
    }
    ESP_LOGI(HWTAG,"mDNS gestartet. Zugang über: http://klangbox.local\n");

    // Optionale mDNS-Dienste registrieren (z. B. HTTP)
    MDNS.addService("http", "tcp", 80);

    ESP_LOGI(HWTAG,"HTTP server gestarted\n");
    currentMillis=millis();

    while(true) {
        vTaskDelay(pdMS_TO_TICKS(100));
        server.handleClient();
        unsigned long ms = millis() - currentMillis;
        if(!isConnected && ms >= LOOPCNTMAX) {
            server.stop();
            WiFi.disconnect();    // Disconnect WiFi
            WiFi.mode(WIFI_OFF);  // turnoff WiFi
            ESP_LOGI(HWTAG,"HTTP server stopped, wifi stopped, deep sleep starting\n");
            break;
        }
    }
}

#else
void HandleWebserver::handleWeb() {

}
#endif