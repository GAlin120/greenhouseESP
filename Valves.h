#define BLYNK_GREEN     "#23C48E"
#define BLYNK_BLUE      "#04C0F8"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED       "#D3435C"
#define BLYNK_DARK_BLUE "#5F7CD8"
#define BLYNK_GREY		"#ED9D00"
class PinControl {
	uint8 PinOpen, PinClose;
	bool ONLavel;
public:
	PinControl(const uint8 _PinOpen, const uint8 _PinClose, const bool _ONLavel);
	void SwitchPin(bool _St);
	void ResetPin();
};
PinControl::PinControl(const uint8 _PinOpen, const uint8 _PinClose, const bool _ONLavel) : PinOpen(_PinOpen), PinClose(_PinClose), ONLavel(_ONLavel) {
	pinMode(PinOpen, OUTPUT); 	digitalWrite(PinOpen, !ONLavel);
	pinMode(PinClose, OUTPUT); 	digitalWrite(PinClose, !ONLavel);
}
void PinControl::SwitchPin(bool _St) {
	if (_St) { Serial.print("Pin "); Serial.print(PinOpen); Serial.print(" "); Serial.println(ONLavel); }
	else { Serial.print("Pin "); Serial.print(PinClose); Serial.print(" "); Serial.println(PinOpen != PinClose ? ONLavel: !ONLavel); }
	if (_St) digitalWrite(PinOpen, ONLavel);
	else digitalWrite(PinClose, (PinOpen != PinClose ? ONLavel : !ONLavel));
}
void PinControl::ResetPin() {
	if (PinOpen != PinClose) {
		Serial.print("Pin "); Serial.print(PinOpen); Serial.print(" "); Serial.println(!ONLavel);
		Serial.print("Pin "); Serial.print(PinClose); Serial.print(" "); Serial.println(!ONLavel);
		digitalWrite(PinOpen, !ONLavel);
		digitalWrite(PinClose, !ONLavel);
	}
}
// #################################################################
class ValveControl {
public:
	ValveControl(const uint8 _PinOpen, const uint8 _PinClose, const uint8 _VPin, const uint16 _TimeBusy, char *_Discr, const bool _ONLavel) :
	TimeBusy(_TimeBusy), Discr(_Discr), VPin(_VPin), Pin(_PinOpen, _PinClose, _ONLavel) {};
	void Switch(bool _Botton, bool _Manual = false);
	bool EndOfSwitch() { return (busy && millis() >= busy); }
	void StopSwitch(int8 _Status = -1) {
		Pin.ResetPin();
		busy = 0;
		if (_Status >= 0) {
			Valve = Botton = _Status;
			Blynk.virtualWrite(VPin, _Status);
		} else {
			Valve = Botton = Botton;
		}
		sendColor();
	}
	void sendColor() {
		const char *_Color = BLYNK_GREEN;
		if (Botton != Valve) _Color = BLYNK_GREY;
		else if (Error) _Color = BLYNK_RED;
		Blynk.setProperty(VPin, "color", _Color);
	}
	bool Valve = false, Botton = false, Error = false;
	time_t LastStart=0;
private:
	PinControl Pin;
	ulong busy=0;
	uint8 VPin;
	uint16 TimeBusy;
	char *Discr;
};
void ValveControl::Switch(bool _Botton, bool _Manual) {
	if (busy) {
		if (_Manual) Blynk.virtualWrite(VPin, Botton);
	} else if (_Botton != Valve) {
		if (!_Manual) Blynk.virtualWrite(VPin, _Botton);
		Botton = _Botton;
		sendColor();
		busy = millis() + TimeBusy;
		Pin.SwitchPin(Botton);
		sprintf(Buf, "%s %s%s\0", Discr, _Botton ? "вкл." : "откл.", _Manual ? "вручную" : "авто");
		PrintToLog(Buf);
	}
}

// #################################################################
class FillSensorControl {
	uint8 PinUp, PinDown;
	bool Full, Empty, preEMPTY=false;
public:
	FillSensorControl(const uint8 _PinUp, const uint8 _PinDown) :
		PinUp(_PinUp), PinDown(_PinDown) {
		pinMode(PinUp, INPUT); digitalWrite(PinUp, HIGH);
		pinMode(PinDown, INPUT); digitalWrite(PinDown, HIGH);
	}
	void Read() {
		Full  = digitalRead(PinUp);
		Empty = !digitalRead(PinDown);
	}
	bool FULL() { return Full;  }
	bool EMPTY() { return Empty; }
	bool EventEMPTY() { 
		bool ret = Empty && (Empty != preEMPTY);
		preEMPTY = Empty;
		return ret;
	}
};

class c_DoorStatus {
public:
	c_DoorStatus(const uint8 _pin) : pin(_pin) { ChangeStatus(); }
	bool ChangeStatus() {
		uint16 vol_sensor = analogRead(pin);
		if (vol_sensor <= vol_Close) Status = 0;			// Закрыто
		else if (vol_sensor >= vol_Open) Status = 1;		// Открыто
		else Status = -1;
		if (Status >-1 && Status != preStatus) {
			preStatus = Status;
			return true;
		} else {
			return false;
		}
	}
	int8 Status = -1;								 		// Открывается-Закрывается
private:
	int8 pin;
	int8 preStatus = -2;
};