#ifndef BlynkManager_h
#define BlynkManager_h 
#ifdef  DEBUGBLYNKMANAGER
#define DEBUGBLYNKPRN(...) { Serial.print("*BM: "); Serial.println(__VA_ARGS__); }
#else
#define DEBUGBLYNKPRN(...)
#endif

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "HTMLData.h"
extern "C" {
#include "user_interface.h"
}

#define DNS_PORT 53

class BlynkManager {
public:
	BlynkManager(FileManager *_FM) : FM(_FM) {};
	void startConfigPortal(String _apName, String _apPass);
private:
	void handleRoot();
	void handleWifiSave();
	void handleNotFound();
	void handleReset();
	boolean captivePortal();
	std::unique_ptr<DNSServer> dnsServer;
	std::unique_ptr<ESP8266WebServer> apServer;
	FileManager *FM;
	bool APSTA_ON = false;
};

void BlynkManager::startConfigPortal(String _apName, String _apPass) {
	DEBUGBLYNKPRN("Start config portal");
	WiFi.mode(WIFI_AP_STA);
	dnsServer.reset(new DNSServer());
	apServer.reset(new ESP8266WebServer(80));
	if (_apName == "") _apName = "ESP" + String(ESP.getChipId());
	DEBUGBLYNKPRN("Configuring access point " + _apName);
	if (_apPass != "" && (_apPass.length() < 8 || _apPass.length() > 63)) {
		DEBUGBLYNKPRN("Invalid AccessPoint password. Ignoring");
		_apPass = "";
	} else DEBUGBLYNKPRN("Password " + _apPass);
	IPAddress ip(192, 168, 4, 1);
	IPAddress gateway(0, 0, 0, 0);
	IPAddress subnet(255, 255, 255, 0);
	WiFi.softAPConfig(ip, gateway, subnet);
	WiFi.softAP(_apName.c_str(), _apPass.c_str());
	delay(500);
	DEBUGBLYNKPRN("AP IP address: " + WiFi.softAPIP().toString());
	dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
	apServer->on("/", std::bind(&BlynkManager::handleRoot, this));
	apServer->on("/wifisave", std::bind(&BlynkManager::handleWifiSave, this));
	apServer->on("/r", std::bind(&BlynkManager::handleReset, this));
	apServer->on("/fwlink", std::bind(&BlynkManager::handleRoot, this));
	apServer->onNotFound(std::bind(&BlynkManager::handleNotFound, this));
	apServer->begin(); 
	DEBUGBLYNKPRN("HTTP server started");
	APSTA_ON = true;
	while (APSTA_ON) {
		dnsServer->processNextRequest();
		apServer->handleClient();
		yield();
	}
	WiFi.mode(WIFI_STA);
}

void BlynkManager::handleRoot() {
	if (captivePortal()) return;
	DEBUGBLYNKPRN("Handle root");
	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Конфигурация");
	DEBUGBLYNKPRN("Scan networks...");
	int n = WiFi.scanNetworks();
	DEBUGBLYNKPRN("Scan done");
	if (n == 0) {
		DEBUGBLYNKPRN("No networks found");
		page += "WiFi сетей не найдено.";
	} else {
		page += FPSTR(HTTP_SCRIPT_SSID);
		for (int i = 0; i < n; i++) {
			DEBUGBLYNKPRN(WiFi.SSID(i) + " " + WiFi.RSSI(i));
			String item = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
			item.replace("{v}", WiFi.SSID(i));
			item.replace("{r}", String(WiFi.RSSI(i)));
			if (WiFi.encryptionType(i) != ENC_TYPE_NONE) item.replace("{i}", "l");
			else item.replace("{i}", "");
			page += item;
		}
		page += "<br/>";
	}
	page += "<form method='get' action='wifisave'>";
	page += "<input id='s' name='s' length=32 placeholder='SSID'><br/>";
	page += "<input id='p' name='p' length=64 type='password' placeholder='password'><br/>";
	page += "<input id='server' name='server' length=40 placeholder='blynk server' value='" + String(BLYNK_DEFAULT_DOMAIN) + "'><br/>";
	page += "<input id='port' name='port' length=6 placeholder='blynk port' value='" + String(BLYNK_DEFAULT_PORT) + "'><br/>";
	page += "<input id='token' name='token' length=32 placeholder='blynk token' value=''><br/>";
	page += "<input id='TZ_Offset' name='TZ_Offset' length=3 placeholder='Смещение от Гринвича (час)' value=''><br/>";
	page += "<input id='LATITUDE' name='LATITUDE' length=5 placeholder='Широта' value=''><br/>";
	page += "<input id='LONGITUDE' name='LONGITUDE' length=5 placeholder='Долгота' value=''><br/>";
	page += "<br/><button type='submit'>Сохранить</button>";
	page += "<br/><button type='button' onclick=\"location.href='/r'\">Рестарт</button>";
	page +=	"</form>";
	apServer->send(200, "text/html", page);
	DEBUGBLYNKPRN("Sent config page");
}

void BlynkManager::handleWifiSave() {
	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Сохранение параметров");
	page += "Параметры сохранены.";
	page += FPSTR(HTTP_END);
	apServer->send(200, "text/html", page);
	FM->json["wifi_ssid"] = apServer->arg("s");
	FM->json["wifi_pass"] = apServer->arg("p");
	FM->json["blynk_server"] = apServer->arg("server");
	FM->json["blynk_port"] = apServer->arg("port").toInt();
	FM->json["blynk_token"] = apServer->arg("token");
	FM->json["TZ_Offset"] = apServer->arg("TZ_Offset").toInt();
	FM->json["LATITUDE"] = apServer->arg("LATITUDE").toFloat();
	FM->json["LONGITUDE"] = apServer->arg("LONGITUDE").toFloat();
	FM->saveParam();
	APSTA_ON = false;
}
void BlynkManager::handleReset() {
	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Reset");
	page += "Модуль перезагрузится через<br/>несколько секунд.";
	page += FPSTR(HTTP_END);
	apServer->send(200, "text/html", page);
	delay(1000);
	ESP.restart();
}

void BlynkManager::handleNotFound() {
	if (captivePortal()) return;
	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "404. Не найдена");
	page += "<h3>Страница не найдена<br></h3>";
	page += "URI: " + apServer->uri() + "<br>";
	page += "Аргумент: " + String(apServer->args()) + "<br>";
	for (uint8_t i = 0; i < apServer->args(); i++) page += " " + apServer->argName(i) + ": " + apServer->arg(i) + "<br>";
	page += FPSTR(HTTP_END);
	apServer->send(404, "text/html", page);
}

boolean BlynkManager::captivePortal() {
	IPAddress ip;
	if (!ip.fromString(apServer->hostHeader())) {
		DEBUGBLYNKPRN("Request redirected to captive portal");
		apServer->sendHeader("Location", String("http://") + apServer->client().localIP().toString(), true);
		apServer->send(302, "text/plain", ""); 
		apServer->client().stop();
		return true;
	}
	return false;
}

#endif