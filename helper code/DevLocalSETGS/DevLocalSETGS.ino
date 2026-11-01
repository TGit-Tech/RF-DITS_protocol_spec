#include "SETGS.h";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  InitRAMMem();
}

uint32_t ms=0;
int Step = 0;
void prtMode(byte _pinMode) {
  if (_pinMode==INPUT) {Serial.print("pinMode: ");Serial.println("INPUT");} //0
  if (_pinMode==OUTPUT) {Serial.print("pinMode: ");Serial.println("OUTPUT");} //1
  if (_pinMode==INPUT_PULLUP) {Serial.print("pinMode: ");Serial.println("INPUT_PULLUP");} //2
  if (_pinMode==3) {Serial.print("pinMode: ");Serial.println("DISBABLED");}
}
void prt(const char* _id, int _val) {Serial.print(_id);Serial.print(": ");Serial.println(_val);}
void loop() {
  
  // put your main code here, to run repeatedly:
  if (!(millis() - ms > 5000)) return;
  ms=millis();Step++;
  if (Step > 10) Step = 0;

  if (Step==1) {
    SETGSDigital sgsDig(0);
    
    sgsDig.Pin1Mode(OUTPUT);
    sgsDig.Pin1(3);
    sgsDig.Pin2Mode(INPUT_PULLUP);
    sgsDig.Pin2(11);
    Serial.println("------ DIG ------");
    prt("Pin1",sgsDig.Pin1());
    prtMode(sgsDig.Pin1Mode());
    prt("Pin2",sgsDig.Pin2());
    prtMode(sgsDig.Pin2Mode());
  }
  if (Step==2) {
    SETGSMath sgsMath(1);
    sgsMath.in1DevUID(60);
    sgsMath.in2DevUID(20);
    sgsMath.Operator(3);
    Serial.println("------ MATH ------");
    prt("inDev1",sgsMath.in1DevUID());
    prt("inDev2",sgsMath.in2DevUID());
    prt("oper",sgsMath.Operator());
  }
  if (Step==3) {
    SETGSAnalog sgsAna(2);
    sgsAna.MultNumerator(-5000);
    sgsAna.Pin(12);
    sgsAna.RawOffset(1001);
    sgsAna.PinMode(OUTPUT);
    sgsAna.Samples(3);
    Serial.println("------ ANALOG ------");
    prt("Pin",sgsAna.Pin());
    prtMode(sgsAna.PinMode());
    prt("RawOffset",sgsAna.RawOffset());
    prt("MultNum",sgsAna.MultNumerator());
    Serial.print("MultNum: "); Serial.println(sgsAna.MultNumerator(),HEX);
    prt("Samples",sgsAna.Samples());
  }
  if (Step==4) {
    SETGSSetpoint sgsSetpoint(3);
    Serial.println("------ SETPOINT ------");
    prt("setpoint",sgsSetpoint.Setpoint());
    prt("inDev",sgsSetpoint.inDevUID());
    prt("inTol",sgsSetpoint.inTolerance());
    prt("timef",sgsSetpoint.TimeFrameTol());
    prt("timefText",sgsSetpoint.TimeFrameTolStr());
    prt("timefms",sgsSetpoint.TimeFrameTolMS());
    prt("outNode",sgsSetpoint.outNodeIdx());
    prt("outDev",sgsSetpoint.outDevUID());
    prt("AboveDo",sgsSetpoint.AboveSetDo());
    prt("BelowDo",sgsSetpoint.BelowSetDo());
    prt("IsAuto",sgsSetpoint.IsAuto());
  }
}
