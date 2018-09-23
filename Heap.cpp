time_t StartOfDay(time_t t) { return time_t(t / 86400) * 86400; }
// #################################################################
template <typename t>
class cmpPreValue {
	t PreVal;
public:
	cmpPreValue (t _PreVal): PreVal(_PreVal){}
	void operator = (const t& Val) { PreVal = Val; }
	bool operator!=(const t& Val) { 
		bool ret = PreVal != Val;
		PreVal = Val;
		return ret;
	}
};

cmpPreValue <int>preDay(0);					// Хранит число предидущего дня;

void PrintToLog(const char* _value, time_t _time) {
	if (CONNECTED == BL_CONNECTED) {
		char BufTimeData[18];				// Буффер для форматирования даты и времени
		if (!_time) _time = now();
		sprintf(BufTimeData, "%02d:%02d %02d.%02d \0", hour(_time), minute(_time), day(_time), month(_time));
		terminal.print(BufTimeData); terminal.println(_value);
		terminal.flush();
		DEBUGPRN(_value)
	}
}
#define deleaySaveParam _SEC_(30)
void saveParamTimer() {
	FParam.saveParam();
	PrintToLog("Параметры сохранены");
}
BLYNK_WRITE_DEFAULT() {
	TimeInputParam t(param);
	switch (request.pin) {
	case _DAuto:
		FParam.json["DAuto"][0] = String(param[0].asString());
		FParam.json["DAuto"][1] = String(param[1].asString());
		preDay = 0;
		PrintToLog("Установлено время автопроветривания");
		break;
	case _TOpenIn:
		FParam.json["TOpen"][0] = param.asInt();
		PrintToLog("Установлена Т внутри вкл.атопроветривания");
		break;
	case _TOpenOut:
		FParam.json["TOpen"][1] = param.asInt();
		PrintToLog("Установлена Т снаружи вкл.автопроветривания");
		break;
	case _WAuto:
		FParam.json["WAuto"] = param[0].asLong();
		PrintToLog("Установлено время автополива");
		break;
	case _WDur:
		FParam.json["WDur"] = param.asInt();
		Blynk.setProperty(_WDone, "max", param.asInt());
		PrintToLog("Установлена длительность полива");
		break;
	case _WPer:
		FParam.json["WPer"] = param.asInt();
		PrintToLog("Установлена переодичность полива");
		break;
	case _FAuto:
		FParam.json["FAuto"][0] = param[0].asLong();
		FParam.json["FAuto"][1] = param[1].asLong();
		PrintToLog("Установлено время долива емkости");
		break;
	case _SAuto:
		FParam.json["SAuto"] = param.asInt();
		preDay = 0;
		PrintToLog("Установлено смещение времени проветривания");
		break;
	}
	if (timer_saveParam) timer.deleteTimer(timer_saveParam);
	timer_saveParam = timer.setTimeout(deleaySaveParam, saveParamTimer);
}

BLYNK_WRITE(InternalPinOTA) {
	String overTheAirURL = param.asString();
	//  overTheAirURL += String("?auth=") + configStore.cloudToken;
	Blynk.disconnect();
	delay(500);
	DEBUGPRN(String("Firmware update URL: ") + overTheAirURL);
	switch (ESPhttpUpdate.update(overTheAirURL, BOARD_FIRMWARE_VERSION)) {
	case HTTP_UPDATE_FAILED:
		DEBUGPRN(String("Firmware update failed (error ") + ESPhttpUpdate.getLastError() + "): " + ESPhttpUpdate.getLastErrorString());
		break;
	case HTTP_UPDATE_NO_UPDATES:
		DEBUGPRN("No firmware updates available.");
		break;
	case HTTP_UPDATE_OK:
		DEBUGPRN("Firmware updated.");
		ESP.restart();
		break;
	}
}

// FILLING ###############################################
ValveControl Fill(pin_open_fill, pin_close_fill, _FButton, DelayFillingValve, "Насос", LOW);
FillSensorControl FillSensor(pin_up_fill, pin_down_fill);
BLYNK_WRITE(_FButton) { 
	if (FirstCONNECTEDFILL) {
		FirstCONNECTEDFILL = false;
		if (param.asInt()) {
			Fill.Valve = Fill.Botton = true;
			Fill.LastStart = Now;
			Fill.Switch(false);
		}
	} else {
		Fill.Error = false;
		if (param.asInt() && FillSensor.FULL()) {
			Blynk.virtualWrite(_FButton, false);
		} else {
			Fill.Switch(param.asInt(), true); 
		}
	}

}
void FillRestart() { 
	Blynk.virtualWrite(_FButton, Fill.Botton);
	Fill.sendColor();
}
void FillControl() {
	time_t Duration = FParam.json["FAuto"][1].as<long>();		// Максимальная длительность включения
	FillSensor.Read();								// Читаем состояние сенсоров
	if (Fill.Valve) {		
		Fill.Error = (Now >= Fill.LastStart + Duration);
		if (FillSensor.FULL() || Fill.Error) {
			Fill.Switch(false);		// Полна коробочка или Превышено максимальное время ожидания Выkлючаю 
		}
	} else {
		bool FillToday = (Duration>0) && ((Now % 86400) == FParam.json["FAuto"][0].as<long>()); // Устанавливаем, наполнялаcь ли сегодня емкость по времени
		static cmpPreValue <bool>preFillToday(FillToday);
		if (preFillToday != FillToday && FillToday && !FillSensor.FULL() && !Fill.Error) Fill.Switch(true);	// Вkлючение по времени
	}
	if (FillSensor.EventEMPTY()) {					// Пусто
		sprintf(Buf, "%s\0", "Емкость пуста"); 
		PrintToLog(Buf);
		Blynk.notify(Buf);
	}
	if (Fill.EndOfSwitch()) {						// Переключили
		Fill.StopSwitch();
		if (Fill.Valve) {							// Включился?
			Fill.LastStart = Now;					// Запомнил время старта
			Fill.Error = false;
		} else {									// Выключился?
			if (Fill.Error) {
				sprintf(Buf, "%s\0", "Сбой датчика уровня");
				PrintToLog(Buf);
				Blynk.notify(Buf);
			}
		}
	}
}

// WATERING ###############################################
ValveControl Water(pin_watering, pin_watering, _WButton, DelayWaterValve, "Полив", HIGH);
bool __WaterRestart = false;
BLYNK_WRITE(_WButton) { Water.Switch(param.asInt(), true); }
void WaterRestart() { 
	Blynk.virtualWrite(_WButton, Water.Botton);
	Water.sendColor();
	Blynk.virtualWrite(_WDone, 0);
	__WaterRestart =true;
}
void WaterControl() {
	int Duration = FParam.json["WDur"].as<int>();				// Длительность полива
	int Done = Duration; 
	static cmpPreValue <int>preDone(Done);
	time_t Next = bool(FParam.json["WPer"].as<long>()) * (StartOfDay(FValues.json["WPrev"].as<long>() + FParam.json["WPer"].as<long>() * 86400) + FParam.json["WAuto"].as<long>());
	static cmpPreValue <time_t>preNext(Next);
	char bufLabel[] =  "Автополив отключен" ;
	if (__WaterRestart || preNext != Next) {
		if (Next) {							// Вычислен следующий полив
			tmElements_t rm; breakTime(Next, rm);
			sprintf(bufLabel, "%2d:%02d %2d.%02d.%04d\n", rm.Hour, rm.Minute, rm.Day, rm.Month, rm.Year + 1970);
		}
		Blynk.virtualWrite(_WNext, bufLabel);
	}
	if (Water.Valve) {		// Полив включен ***********************************************
		Done = Duration - ((Now - Water.LastStart) / 60);		// Осталось полить минут
		if (Done <= 0) {										// Готово
			Done = 0;
			Water.Switch(false);
		}
		if (__WaterRestart || preDone != Done) Blynk.virtualWrite(_WDone, Done);
	} else {				// Полив выключен ***********************************************
		if (Next && Now >= Next) Water.Switch(true);			// Вkлючение по времени
	}
	__WaterRestart = false;
	if (Water.EndOfSwitch()) {
		preDone=-1;
		Water.StopSwitch();
		if (Water.Valve) {							// Включился полив?
			Water.LastStart = Now;					// Запомнил время старта
		} else {									// Выключился?
			Blynk.virtualWrite(_WDone, 0);
//			При выключении подошло время автополива или поливалось больше половины необходимого времени?
			if (Now >= Next || Done < Duration/2) {
				FValues.json["WPrev"] = Water.LastStart;	// Записваем в файл очередное время полива
				FValues.saveParam();
			}
		}
	}
}

// DOOR ###############################################
ValveControl Door(pin_open, pin_close, _DButton, DelayDoor, "Вентиляция", LOW);
time_t TimeStampPress = 0;

BLYNK_WRITE(_DButton) {
	Door.Switch(param.asInt(), true);
	TimeStampPress = millis();
}
void DoorRestart() { 
	Blynk.virtualWrite(_DButton, Door.Botton);
	Door.sendColor();
	DSSensor.SendTemp(0);
	DSSensor.SendTemp(1);
	preDay = day(Now);
}
void DoorControl() {
	if (millis() > TimeStampPress + _MIN_(5)) TimeStampPress = 0;	// Ждем время после нажатия ручного изменения проветривания, и переводим снова в авторежим
	if (DSSensor.Respond()) DSSensor.SendTemp(DSSensor.currSensor);	// Считываю состояние датчиков и отсылаю на сервер
	enum TStat {unknown=-1, close, open } TempStatus = unknown;
	static time_t TimeStart, TimeStop;
	if (preDay != day(Now)) {
		tmElements_t t; breakTime(Now, t);
		byte sunrise[6] = { 0, 0, 0,  t.Day, t.Month, t.Year };
		byte sunset[6] = { 0, 0, 0,  t.Day, t.Month, t.Year };
		TimeLord myLord;		// переменные TimeLord
		myLord.TimeZone(FConfig.json["TZ_Offset"].as<int>() * 60);
		myLord.Position(FConfig.json["LATITUDE"].as<float>(), FConfig.json["LONGITUDE"].as<float>());
		myLord.SunRise(sunrise);
		myLord.SunSet(sunset);
		if (!strlen(FParam.json["DAuto"][0].asString())) TimeStart = -1;
		else if (!strcmp(FParam.json["DAuto"][0].asString(), "sr")) TimeStart = (int)sunrise[tl_hour] * 3600 + (int)sunrise[tl_minute] * 60 + FParam.json["SAuto"].as<int>();
		else if (!strcmp(FParam.json["DAuto"][0].asString(), "ss")) TimeStart = (int)sunset[tl_hour] * 3600 + (int)sunset[tl_minute] * 60 + FParam.json["SAuto"].as<int>();
		else TimeStart = FParam.json["DAuto"][0].as<long>();
		if (!strlen(FParam.json["DAuto"][1].asString())) TimeStop = 0;
		else if (!strcmp(FParam.json["DAuto"][1].asString(), "sr")) TimeStop = (int)sunrise[tl_hour] * 3600 + (int)sunrise[tl_minute] * 60 - FParam.json["SAuto"].as<int>();
		else if (!strcmp(FParam.json["DAuto"][1].asString(), "ss")) TimeStop = (int)sunset[tl_hour] * 3600 + (int)sunset[tl_minute] * 60 - FParam.json["SAuto"].as<int>();
		else TimeStop = FParam.json["DAuto"][1].as<long>();
		sprintf(Buf, "%s %2d:%02d\0", "Старт авто", hour(TimeStart), minute(TimeStart)); PrintToLog(Buf);
		sprintf(Buf, "%s %2d:%02d\0", "Стоп авто", hour(TimeStop), minute(TimeStop)); PrintToLog(Buf);
	}
	time_t tmNow = Now % 86400;										// Проверю время когда автопроветривание работает
	if (tmNow >= TimeStart && tmNow < TimeStop) {
		if (!DSSensor.Error(0) && !DSSensor.Error(1)) {				// Проверю температурные границы
			if (DSSensor.asFloat(0) < (FParam.json["TOpen"][0].as<float>() - _TDelta) || DSSensor.asFloat(1) < (FParam.json["TOpen"][1].as<float>() - _TDelta)) TempStatus = close;
			if (DSSensor.asFloat(0) >= FParam.json["TOpen"][0].as<float>() && DSSensor.asFloat(1) >= FParam.json["TOpen"][1].as<float>()) TempStatus = open;
		}
	} else {
		TempStatus = close;
	}
	static cmpPreValue <TStat>preTempStatus(unknown);
	if (!TimeStampPress && TempStatus != unknown && preTempStatus != TempStatus) Door.Switch(TempStatus);
	if (DoorStatus.ChangeStatus()) {
		Door.Error = false;
		Door.StopSwitch(DoorStatus.Status);
	}
	Door.Error = Door.EndOfSwitch();
	if (Door.Error) {
		Door.StopSwitch(false);
		sprintf(Buf, "%s\0", "Ошибка датчика вентиляции");
		PrintToLog(Buf);
		Blynk.notify(Buf);
	}
}

// Termometr ###############################################
void TermometrControl() {
	DSSensor.Request();
}

