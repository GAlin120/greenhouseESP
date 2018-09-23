#include <OneWire.h>
#define MaxSensorCount 2
#define MaxCountError 3
#define ErrorValue -2731
class c_TempDS18B20 {
public:
	c_TempDS18B20(const uint8 _pinTherm) {
		ds = new OneWire(_pinTherm);					// Инициализация датчика температуры (пин датчика) 
		ds->reset_search();
		while (Count < MaxSensorCount && ds->search(SensAdr[Count])) Count++;
		for (uint8 i = 0; i < Count; i++) {
			Request();
			for (unsigned long _i = millis(); millis() - _i < 800; );
			CountError[Count] = MaxCountError;
			Respond();
		}
	};
	bool Respond() {
		if (Busy && Busy <= millis()) {
			Busy = 0;
			ds->reset();
			ds->select(SensAdr[currSensor]);					// Тут выполнение останавливается на 6 мс. Прерывания отключаются.
			ds->write(0xBE);
			byte data[9];
			for (byte i = 0; i < 9; i++) data[i] = ds->read();	// Читаем из датчика 9 байт
			byte crc = ds->crc8(data, 8);						// Высчитываем CRC
			int _Temp = ErrorValue;
			if (!(crc == 0) || (crc != data[8])) {
				_Temp = ((data[1] << 8) | data[0]) * 5 / 8;
				if (_Temp > 800 || _Temp < -600) _Temp = ErrorValue;	// Ошибка если температура выше 80 и ниже 60 градусов
			}
			if (_Temp != Temp[currSensor]) {
				if (_Temp == ErrorValue) {
					if (CountError[currSensor] < MaxCountError) {
						CountError[currSensor]++;
						return false;
					} else {
//						sprintf(Buf, "%s%1d\0", "Ошибка датчика температуры # ", currSensor);
//						PrintToLog(Buf);
					}
				}
				Temp[currSensor] = _Temp;
				return true;
			}
			CountError[currSensor] = 0;
		}
		return false;
	}
	void Request() {
		if (!Busy) {
			if (++currSensor >= Count) currSensor = 0;
			ds->reset();
			ds->select(SensAdr[currSensor]);
			ds->write(0x44, 1); // команда на измерение температуры
			Busy = millis() + 800;
		}
	}
	bool Error(uint8 _Sensor) { return Temp[_Sensor] == ErrorValue; }
	float asFloat(uint8 _Sensor) { return float(Temp[_Sensor])/10.0; }
	uint8 Count = 0;
	uint8 currSensor = MaxSensorCount;
	void SendTemp(uint8 _Sensor) {
		if (Error(_Sensor))
			Blynk.virtualWrite(_Temp[_Sensor], "Ошибка");
		else
			Blynk.virtualWrite(_Temp[_Sensor], asFloat(_Sensor));
	}
private:
	int Temp[MaxSensorCount] = { ErrorValue, ErrorValue };							// Температура
	OneWire	*ds;
	byte SensAdr[MaxSensorCount][8];
	ulong Busy=0;
	byte CountError[MaxSensorCount];
};

