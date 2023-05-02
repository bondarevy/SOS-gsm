/*
   Кнопка SOS на дорожном столбе
   v0.01: Управление по смс и звонки .
   - исправление 0.01
*/


////////////////////////// начало настроек которые можно менять //////////////////////
char SketchVersion[16] = "Bondarev v0.01";
char SMSDefaultContact[18] = "+79187958840"; //кому отправлять смс по умолчанию
char SMSContact[18]; //кому отправляются уведомления о критических ошибках

#include <avr/wdt.h>
#include <Bounce2.h>
#include <EEPROM.h>
#include <NeoSWSerial.h>



//цифровые входы/выходы (прерывания только на 2 и 3 пине)
const int GSMTXPin = 2;        // пин на который мы получаем входящие сообщения от GSM асинхронно по прерываниям (!!!прерывание - ОБЯЗАТЕЛЬНО!!!).
const int SOSButtonPin = 3;    // кнопка SOS
const int GSMRXPin = 4;        //  пин c которого мы отправляем исходящие сообщения, с ардуины на GSM (без прерывания)
const int GSMPowerPin = 5;     // пин включения/выключения GSM, подать HIGH на 1,1 сек.
const int GSMStatusPin = 6;    // пин состояния, когда GSM запустился появляется напряжение
const int GSMRSTPin = 7;       // пин перезагрузки GSM модуля, подтянуть к земле для сброса
const int GSMStatusCallPin = 13   // пин индикации вызова

String inputString = "";   // буквенная переменная в которую будут собиратся сообщения по одному байту от GSM
boolean stringComplete = false;  // устанавливаем переменную "stringComplete" в ложь

NeoSWSerial serialSIM800(GSMTXPin, GSMRXPin); // мы инициализировали библиотеку NeoSWSerial, исоздали объект серейного порта платы SIM800

//инициируем объект дебаунсера
Bounce DebouncedSOSButton = Bounce();

static void HandleGSMRXData( char c ) // собираем сообщения от GSM в ячейку "c" по одному символу
{
  // add it to the inputString:       // проверяем сообщение, \n или \r означает конец сообщения
  if (c == '\n' || c == '\r') {       // если "c" равен "\n" или "\r"
    stringComplete = true;            // "stringComplete" назначается истинным
  } else {                            // ещё
    inputString += c;                 // записываем в inputString информацию из ячейки "с" путём сложения
  }
}
void HandleIncomingSerialFromGSM() {  // посылаем законченое сообщение в компорт 
  // Вывод                              
  if (stringComplete) {               //      
    if (inputString.length() != 0) {  // если количество символов не равно нулю
      Serial.println(inputString);    // послать в компорт сообщение находящиеся в "inputString"
      inputString = "";               // стёрли сообщение в "inputString"
    }
    stringComplete = false;           // назначили "stringComlete" ложью
  }
}


void power()                          // команда "power" подаёт на "GSMPowerPin" логическую единицу на 1.1 секунду
{
  digitalWrite(GSMPowerPin, HIGH);
  delay(1100);

  digitalWrite(GSMPowerPin, LOW);
}


void AtRequest(String AtCommand = "", String AtResponse = "", int Retries = 5) {
  if (AtCommand.length() > 0 and AtResponse.length() == 0) {             // если длина строки запроса больше ноля и длина строки ответов ровна нулю, это означает что программа зависла
    wdt_reset();                                                         // и по этому делаем сброс
    serialSIM800.println(AtCommand);                                     // посылаем в GSM модуль АТ команду
   Serial.println(AtComand);    //для тестирования
     AtCommand = "";                                                      // 
  } else if (AtCommand.length() > 0 and AtResponse.length() > 0) {
    bool Finished = false;
    while (Retries > 0 and Finished != true) {
      wdt_reset();
      Retries--;
      serialSIM800.println(AtCommand);
       Serial.println(AtComand);      // для тестирования
      unsigned long CurrentTime = millis();
      while ( millis() < CurrentTime + 5000 and Finished != true) {
        wdt_reset();
        if (stringComplete) {
          if (inputString.length() != 0) {
            Serial.println(inputString);
            if (inputString.endsWith(AtResponse)) {
              //OK: AT command succeded.
              Finished = true;
            }
            inputString = "";
          }
          stringComplete = false;
        }
      }
    }
  } else {
    if (stringComplete) {
      if (inputString.length() != 0) {
        Serial.println(inputString);
        inputString = "";
      }
      stringComplete = false;
    }
  }
}

void GSMInit() {
  Serial.println("GSM init...");
  AtRequest("AT", "OK"); //включен ли модем?
  AtRequest("ATE0", "OK"); //отключаем эхо
  AtRequest("AT+CLIP=1", "OK"); //включаем нотификацию на экран
  AtRequest("AT+CMGF=1", "OK"); //переходим в текстовый режим из бинарного
  AtRequest("AT+GSMBUSY=0", "OK"); //разрешить входящие звонки
  AtRequest("AT+CNMI=2,2,0,0,0", "OK"); //отключаем хранение смс в памяти - показывать на экране и все
}

//void SendSMS(char Phone[16], char Message[16]) {              //отключил отправку смс !!!!!!!!!!!!!!!!!!!!!!!!

  //GSMInit();
  //Serial.println("Sending sms..");
  //AtRequest("AT+CMGS=\"" + String(Phone) + "\"\n" + String(Message) + "\x1A" );

//}

void CallSOS() {                                              // вызов!!!!!!!
  GSMInit();
  Serial.println("Call SOS...");
  AtRequest("ATD89187958840", "OK");
} 
   
   
void setup() {
  pinMode(GSMTXPin, INPUT); // инициализируем цифровой вывод в качестве входного.
  pinMode(SOSButtonPin, INPUT_PULLUP); // инициализируем цифровой вывод в качестве входного.
  pinMode(GSMRXPin, OUTPUT); // инициализируем цифровой вывод в качестве выходного.
  pinMode(GSMPowerPin, OUTPUT); // инициализируем цифровой вывод в качестве выходного.
  digitalWrite(GSMPowerPin, LOW); // установили дефолтное значение 0 на пин питания
  pinMode(GSMStatusPin, INPUT); // инициализируем цифровой вывод в качестве входного.
  pinMode(GSMRSTPin, INPUT); // инициализируем цифровой вывод в качестве входного.
  digitalWrite(GSMRSTPin, HIGH); // установили дефолтное значение 1 на пин Ресет.
  pinMode(GSMStatusCallPin, OUTPUT); //инициализируем цифровой вывод в качестве входного.
  digitalWrite(GSMStatusCallPin, LOW);  // установили дефолтное значение 0 на пине статуса звонка
   
  Serial.begin(9600);


  serialSIM800.attachInterrupt(HandleGSMRXData); // Задали обработчик прерывания от SIM800
  serialSIM800.begin(9600);


  //цепляемся раздребезгом к кнопке
  DebouncedSOSButton.attach(SOSButtonPin);
  DebouncedSOSButton.interval(50);


  // Проверка статуса GSM модуля,  читаем статус GSM модуля если ( != HIGH ) - не высокий, отправляем сообшение сериал порт и включаем GSM модуль
  if (digitalRead(GSMStatusPin) != HIGH) {
    Serial.println("GSM modul vikluchen ");
    power();                                        //включение sim808 или выключение sim808

    while (digitalRead(GSMStatusPin) != HIGH) {
      Serial.println("Vkluchaem gsm... ");
      delay(500);
    }

    if (digitalRead(GSMStatusPin) == HIGH) {
      Serial.println("GSM modul vkluchen ");
    }
  }

  GSMInit();

  bool SOSButtonPressed = false;

  // Вечный цикл (начало)
  while (true) {

    //проверяем нажата ли кнопка SOS
    DebouncedSOSButton.update();

    if ( DebouncedSOSButton.read() == LOW ) {
      if (SOSButtonPressed == false) {
        Serial.println("SOSButton = LOW");
        SOSButtonPressed = true;
        //SendSMS("+79187958840", "SOS: Yura ti molodets! Ogurets!");
        //AtRequest("ATD89187958840", "OK");
         CallSOS();
      }
    } else {
      SOSButtonPressed = false;
    }
    //Вывести на экран все сообщения от GSM модуля
    HandleIncomingSerialFromGSM();
    
  } // Вечный цикл (конец)


}



void loop()
{

}




