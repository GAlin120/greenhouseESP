#define BOARD_FIRMWARE_VERSION        "2.0.4" 
#define BL_FALSE		0B0000000000000000 //Не горит
#define BL_CONNECTED	0B0000000000000001 //Короткая вспышка раз в 2 сек Соединено
#define BL_DISCONNECTED	0B0101010101010101 //Мигание по 0.125 сек Разъединено
#define BL_ERROR		0B1111111111111111 //Горит
#define BL_SETUP		0B0000000000010101 //Три короткие вспышки раз в секунду Режим настройки

// "c2798e08433b418f8bf2dfda3d183aaa";
// "ed3ebf5528784ec0a2a605ffc2fc4f72";  Test
// "TPLINKD07BB2";
// "Tplink730re";
// 62.148.143.64
// 38442

//#define BLYNK_DEBUG
#define	BLYNK_PRINT Serial
#define DEBUGBLYNKMANAGER
#define DEBUGFILEMANAGER
#define DEBUG
#ifdef  DEBUG
#define DEBUGPRN(...) { Serial.print("*GH: "); Serial.println(__VA_ARGS__); }
#else
#define DEBUGPRN(...)
#endif
#define _SEC_(sec) 1000UL*sec
#define _MIN_(mn)  _SEC_(60*mn)
#define _HR_(h)  _MIN_(60*h)
#define _DAY_(d) _HR(24*d)

// Реальные kонтаkты
#define pin_status		A0			// АЦП
#define pin_key			D3			// kнопkа сброса
#define pin_led			D4			// Индиkация
#define pin_down_fill	10			// Нижний датчик емкости
#define pin_up_fill		D1			// SD3 Верхний датчик емкости
#define pin_open_fill	D2			// Отkрытие kлапана заполнения
#define pin_close_fill	D0			// Заkрытие kлапана заполнения
#define pin_open		D6			// Отkрытие двери
#define pin_close		D7			// Заkрытие двери
#define pin_TempIn		D5			// Датчиk температуры
#define pin_watering	D8			// Полив

#define DelayFillingValve 3200		// Время задержkи kлапана насоса
#define DelayWaterValve _SEC_(1)	// Время задержkи kлапана полива
#define DelayDoor _SEC_(6)			// Время задержkи отkрывания двери

// Виртуальные kонтаkты
#define _TOpenIn	0				// Температура отkрывания дверей внутри
#define _TOpenOut	1				// Температура отkрывания дверей вне
#define _DAuto		2				// Время работы автопроветривания
#define _WAuto		3				// Время автополива
#define _FAuto		4				// Время автозаполнения
#define _WDur		5				// Длительность полива мин.
#define _WPer		6				// Период автополива в днях
#define _WDone		7				// Выполнен полив в %
#define _WNext		8				// Следующий полив
#define _SAuto		9				// Количество часов после восхода и до заката между которыми работает автопроветривание
#define _DButton	20				// kнопkа вентиляции
#define _WButton	21				// kнопkа полива
#define _FButton	22				// kнопkа емkости
#define _TIn		23				// Температура внутри
#define _TOut		24				// Температура вне
#define _HIn		25				// Влажность внутри
#define _HOut		26				// Влажность вне
#define _Terminal	30				// Терминал
uint8	_Temp[2] = { _TIn,  _TOut };
#define tz "Europe/Moscow"
#define _TDelta		1.0				// Температурная дельта открытием и закрытием проветривания
#define vol_Close 275		// Значение на входе положения Закрыто
#define vol_Open 710		// Значение на входе положения Открыто

char Buf[80];											// Буффер для строkи журнала
void PrintToLog(const char* _value, time_t _time = 0);	// Вывод строkи журнала

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
WidgetRTC rtc;
time_t Now;
#include <TimeLord.h>
#define	TZ_Offset	2
#define LATITUDE	53.51	// установка координат: широта
#define LONGITUDE	34.26 // установка координат: долгота
TimeLord myLord;		// переменные TimeLord

WidgetTerminal terminal(_Terminal);

BlynkTimer timer;
uint16_t timer_saveParam;

#include "FileManager.cpp"
FileManager FConfig("/config.json");
FileManager FParam("/param.json");
FileManager FValues("/val.json");

#include "BlynkManager.cpp"
uint16_t CONNECTED = BL_DISCONNECTED;
bool FirstCONNECTED = true;
bool FirstCONNECTEDFILL = true;
#define AP_PASS "espadmin"
#define AP_SSID ""
uint Connect;
#define TimeReconnect _MIN_(3)

#include <ReadDigKey.h>
ReadDigKey key;

#include <Ticker.h>
Ticker   blinker;
uint8_t  blink_loop = 0;
void timerIsr() {
	digitalWrite(pin_led, !(CONNECTED & 1 << (blink_loop++ & 0x0F)));
	key.readkey();
	if (key.shot_press()) CONNECTED = BL_SETUP;              // kоротkое нажатие 
	if (key.long_press()) CONNECTED = BL_ERROR;				// длинное нажатие 
}

#ifdef DEBUG
#include "TMP.cpp"
//cInputDEBUG input;
#else
#endif // DEBUG
#include "Temperature.h"
c_TempDS18B20 DSSensor(pin_TempIn);
#include "Valves.h"
c_DoorStatus DoorStatus(pin_status);
#include "Heap.cpp"

void BlynkConnect() {
	if (WiFi.status() != WL_CONNECTED) {
		WiFi.persistent(false);
		WiFi.mode(WIFI_STA);
		WiFi.begin(FConfig.json["wifi_ssid"].asString(), FConfig.json["wifi_pass"].asString());
		WiFi.waitForConnectResult();
	}
	if (!Blynk.connected()) {
		Blynk.config(FConfig.json["blynk_token"].asString(), FConfig.json["blynk_server"].asString(), FConfig.json["blynk_port"]);
		FConfig.json.prettyPrintTo(Serial);
		Blynk.connect();
	}
}

BLYNK_CONNECTED() {
	DEBUGPRN("Connect from ESP");
	rtc.begin();
	Blynk.virtualWrite(_TOpenIn, FParam.json["TOpen"][0].as<int>());
	Blynk.virtualWrite(_TOpenOut, FParam.json["TOpen"][1].as<int>());
	Blynk.virtualWrite(_DAuto, FParam.json["DAuto"][0].asString(), FParam.json["DAuto"][1].asString(), tz);
	Blynk.virtualWrite(_WAuto, FParam.json["WAuto"].as<long>(), tz);
	Blynk.virtualWrite(_WDur, FParam.json["WDur"].as<int>());
	Blynk.virtualWrite(_WPer, FParam.json["WPer"].as<int>());
	Blynk.virtualWrite(_FAuto, FParam.json["FAuto"][0].as<long>(), FParam.json["FAuto"][1].as<long>(), tz);
	Blynk.virtualWrite(_SAuto, FParam.json["SAuto"].as<int>());
	while (!timeStatus()) Blynk.run();
	CONNECTED = BL_CONNECTED;
	if (FirstCONNECTED) {
		Blynk.syncVirtual(_FButton);
		PrintToLog("== Старт системы ==");
		FirstCONNECTED = false;
		if (DSSensor.Count < MaxSensorCount) {
			sprintf(Buf, "%s\0", "Ошибка датчиков температуры");
			PrintToLog(Buf);
		}
	}
	PrintToLog("Связь установлена");
	timer.setTimeout(150, DoorRestart);
	timer.setTimeout(250, FillRestart);
	timer.setTimeout(350, WaterRestart);
}

void setup() {	//*************************************************************************************************************
	Serial.begin(115200);
	delay(10);
	Serial.println();
	pinMode(pin_led, OUTPUT);
	key.add_key(pin_key);
	key.LONG_DELAY = 4000;
	blinker.attach(0.125, timerIsr);
	if (!FParam.readParam()) FParam.createJson("{\"TOpen\":[0,0],\"DAuto\":[0,0],\"WAuto\":0,\"WDur\":1,\"WPer\":0,\"FAuto\":[0,0]},\"SAuto\":[0]");
	if (!FValues.readParam()) FValues.createJson("{\"WPrev\":0,\"FPrev\":0}");
	FConfig.json["wifi_ssid"] = "NONE";
	FConfig.json["wifi_pass"] = "";
	FConfig.json["blynk_server"] = BLYNK_DEFAULT_DOMAIN;
	FConfig.json["blynk_port"] = String(BLYNK_DEFAULT_PORT);
	FConfig.json["blynk_token"] = "";
	FConfig.json["TZ_Offset"] = TZ_Offset;
	FConfig.json["LATITUDE"] = LATITUDE;
	FConfig.json["LONGITUDE"] = LONGITUDE;
	FConfig.readParam();
	BlynkConnect();
	setSyncInterval(600); // Sync interval in seconds (10 minutes)
	timer.setInterval(50, DoorControl);
	timer.setInterval(100, WaterControl);
	timer.setInterval(100, FillControl);
	timer.setInterval(_SEC_(2), TermometrControl);
	Connect = timer.setInterval(_SEC_(30), BlynkConnect);
	Water.LastStart = FValues.json["WPrev"].as<long>();	// Время последнего включения
	Door.Botton = (DoorStatus.Status == -1) ? false : DoorStatus.Status;
	Door.Valve = Door.Botton;
}
//*************************************************************************************************************

void loop() {
	//	if (WiFi.status() == WL_CONNECTED) Blynk.run();
	Blynk.run();
	timer.run();
	if (CONNECTED == BL_SETUP) {				// kоротkое нажатие 
		BlynkManager BM(&FConfig);
		BM.startConfigPortal(AP_SSID, AP_PASS);
		timer.restartTimer(Connect);
		BlynkConnect();
		CONNECTED = BL_DISCONNECTED;
	}
	if (CONNECTED == BL_ERROR) {				// Длинное нажатие 
		Blynk.disconnect();
		WiFi.disconnect();
		FConfig.reset();
		FParam.reset();
		FValues.reset();
		ESP.restart();
	}
	if (!Blynk.connected()) CONNECTED = BL_DISCONNECTED;
	Now = timeStatus() ? now() : 0;
}
