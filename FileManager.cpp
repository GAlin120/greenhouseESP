#ifndef FileManager_h
#define FileManager_h 
#ifdef  DEBUGFILEMANAGER
#define DEBUGFM(...) { Serial.print("*FM: "); Serial.println(__VA_ARGS__); }
#else
#define DEBUGFM(...)
#endif
#include <FS.h>
#include <ArduinoJson.h>
class FileManager {
public:
	FileManager(const char *FileName) : file(FileName) { json = jsonBuffer.createObject(); };
	bool readParam();
	bool saveParam();
	bool reset();
	JsonVariant json;
	bool createJson(const char * buf) { 
		json = jsonBuffer.parseObject(buf); 
		return json.success();
	};
private:
	const char	*file;
	DynamicJsonBuffer jsonBuffer;
};

bool FileManager::readParam() {
	bool ret = false;
	char* buf;
	if (SPIFFS.begin()) {
		DEBUGFM("Mounted file system");
		if (SPIFFS.exists(file)) {
			if (File configFile = SPIFFS.open(file, "r")) {
				size_t size = configFile.size();
				buf = (char*)malloc(size);
				configFile.readBytes(buf, size);
				configFile.close();
				json = jsonBuffer.parseObject(buf);
				ret = json.success();
			} else DEBUGFM("Failed to open config file for reading");
		} else DEBUGFM("File not found");
		SPIFFS.end();
	} else DEBUGFM("Failed to mount FS");
	return ret;
}

bool FileManager::saveParam() {
	if (SPIFFS.begin()) {
		DEBUGFM("Mounted file system");
		if (File configFile = SPIFFS.open(file, "w")) {
			json.printTo(configFile);
			configFile.close();
			DEBUGFM("Saving config");
			return true;
		} else DEBUGFM("Failed to open config file for writing");
		SPIFFS.end();
	} else DEBUGFM("Failed to mount FS");
	return false;
}
bool FileManager::reset() {
	if (SPIFFS.begin()) {
		SPIFFS.remove(file);
		SPIFFS.end();
		json = NULL;
		return true;
	} else
		return false;
}
#endif