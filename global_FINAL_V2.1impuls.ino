
//------------------------------------------------------------------------------------------------------------------------
//--------------------------------------Подключаемые библиотеки-----------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------                            
#include <SoftwareSerial.h>      //Библиотека порта
#include <TinyGPS++.h>          // Библиотека GPS
#include <DHT.h>              // Библиотека гигрометра
#include <EEPROM.h>


//------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------
//--------------------------------------Определение пинов-----------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------
#define pump 2                 // пин помпы
#define mode_button 7         // пин кнопки режимов
#define DHTPIN 6             // Пин гигрометра
#define DHTTYPE DHT11       // Определение модели гигрометра
#define LED 13             // Пин светодиода
//--------------------------GPS-------------------------------------------------------------------------------------------
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
//------------------------------------------------------------------------------------------------------------------------
DHT dht(DHTPIN, DHTTYPE);      // Определение объекта гигрометра
//------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------
//---------------------------------------Шрифты OLED----------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------
extern uint8_t MediumFont[];                            
extern uint8_t SmallFont[];
//------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------
//--------------------------------------Глобальные переменные-------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------
     //------------------------Переменные BLUETOOTH--------------------------
            unsigned long BluetoothAndroid; // Переменная для данных из серийного порта bluetooth 
            int city_trip;                 // Переменная из bluetooth "Городской пробег" получаемая от Android содержащая предельное значение пробега по городу
            int hw_trip;                  // Переменная из bluetooth "Пробег по Трассе" получаемая от Android содержащая предельное значение пробега по трассе
            int w_time;                  // Переменная из bluetooth "Время работы" получаемая от Android содержащее среднее время включения насоса
     //-------------------------------------------------------------------------------------------------------------------


     //-----------------------Переменные режимов--------------------------------------------------------------------------
             int mode;     // Переменная режима работы (работает в функции режимов и передается на OLED
             
     //----------------------Переменные спидометра и одометра-------------------------------------------------------------
             float real_trip;             // Переменная текущего пробега (одометр)
             float speedvar;             //Переменная спидометра
             float speedms;             // Переменная спидометра (метры в секунду)
             unsigned long timing;     //Переменная счетчика времени для одометра       
             int sattel;             //Колличество спутников
             int previos_trip;
    //---------------------Остальное----------------------------------------------------------------------------------------
             int w;            //Переменная маркер для гигрометра
             int r;           
//--------------------------------------------------------------------------------------------------------------------------
//         Первичные настройки при включении устройства
//--------------------------------------------------------------------------------------------------------------------------
void setup(){
  //------------------Определение пинов и запуск портов---------------------------------------------------------------------
    digitalWrite(pump,HIGH);                       //Сразу выключаем пин помпы для деактивации насоса
    delay(5000);                                 //Пауза перед запуском
    Serial.begin(9600);                         //Запуск серийного порта для bt
    ss.begin(9600);                            //Запуск GPS
    pinMode(pump,OUTPUT);                     // Пин управления помпой
    pinMode(mode_button,INPUT);              //Пин кнопки,(INPUT_PULLUP)
    dht.begin();                            //Запускаем работу гигрометра
    GET_EEPROM_DATA();                     //Считываем данные из EEPROM
  //-------------------------------------------------------------------------------------------------------------------------
    //*******************Отладочные принты***************************************
    

    
}                                                          
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------



//**************************************************************************************************************************
//          Основной цикл
//**************************************************************************************************************************
void loop(){
//-----Читаем данные из Bluetooth----------------------
btvalue();      
//-------------------------------------------------------


//-----Спидометр-----------------------------------------
wspeed();
//-------------------------------------------------------

//-----Одометр-------------------------------------------
trip();
//-------------------------------------------------------
//-----Упаравляем насосом--------------------------------
MODES();
//-------------------------------------------------------

//-----Проверяем влажность ------------------------------
hum();
//-------------------------------------------------------
//------------------Отладочные принты--------------------    
Serial.print("TRP-");  
Serial.print(real_trip);
Serial.print("   M:");
Serial.print(mode);
Serial.print("   G:");
Serial.print(sattel);
Serial.println("         ");
}//***********Закрытие основного цикла*******************

//------------------------------------------------------------------------------------------------------------------------
//            Функция задержки для GPS
//------------------------------------------------------------------------------------------------------------------------
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------------------
//          Функция смазывания цепи (основная функция)
//--------------------------------------------------------------------------------------------------------------------------

void MODES(){
if (mode==1){                           //Условие срабатывания при включении режима 1 "город"
  digitalWrite(pump,HIGH);             // Отключаем помпу для избежания ложных срабатываний
  if(real_trip>city_trip){            //Сравниваем реальный пробег с маркером пробега трасса
     oiler();                        // Включаем алгоритм насоса
}
}
if (mode==2){                       //Условие срабатывания при включении режима 2 "трасса"
  digitalWrite(pump,HIGH);         // Отключаем помпу для избежания ложных срабатываний
  if(real_trip>hw_trip){          //Сравниваем реальный пробег с маркером пробега трасса
   oiler();                      // Включаем алгоритм насоса
}
}
if (mode==3){                   //Условие срабатывания при включении режима 3 "прокачка"
  digitalWrite(pump,LOW);      // Включаем насос на постоянную работу
}
if (mode==4){                   //Условие срабатывания при включении режима 4 "отключение"
  digitalWrite(LED,HIGH);      // Выключаем насос 
}

}
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------------------------------------
//          Функция получения данных от bluetooth (подключен к серийному порту USB)+запись на SD
//--------------------------------------------------------------------------------------------------------------------------
void btvalue(){
if (Serial.available()) {                                     //Смотрим поток данных из bluetooth
BluetoothAndroid = Serial.parseInt();                        //парсим полученные данные
}
if (BluetoothAndroid==1){                                   //Включение режима "Город"
  mode = 1;
  EEPROM.put(12, mode);
}
if (BluetoothAndroid==2){                                  //Включение режима "Трасса"
  mode = 2;
  EEPROM.put(12, mode);
} 
if (BluetoothAndroid==3){                                     //Включение режима "Прокачка"
  mode = 3;
}
if (BluetoothAndroid==4){                                     //Отключение (сбрасывается при перезапуске)
  mode = 4;
} 
if (BluetoothAndroid==5){                                     //Обнуление пробега
  real_trip=0;
} 
if(BluetoothAndroid>10000 && BluetoothAndroid<19000){        //Установка порогового значения пробега в городе
  city_trip = BluetoothAndroid-10000;
  EEPROM.put(0, city_trip);
  Serial.println("St-");
  Serial.print(city_trip);
}
if(BluetoothAndroid>20000&& BluetoothAndroid<25004){      //Установка порогового значения пробега на трассе
  hw_trip = BluetoothAndroid-20000; 
  EEPROM.put(5, hw_trip); 
  Serial.println("HWt-");
  Serial.print(hw_trip);                    
}
if(BluetoothAndroid>100 && BluetoothAndroid<200){       //Установка времени срабатывания насоса
  w_time = BluetoothAndroid-100;
  EEPROM.put(7, w_time);
   Serial.println("WT-");
  Serial.print(w_time);
}
BluetoothAndroid = 0;                                 //обнуляем парс данные для избежания ошибок
}
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
//          Спидометр
//--------------------------------------------------------------------------------------------------------------------------
void wspeed(){
  sattel=gps.satellites.value();          //Получение колличества пойманных спутников от GPS
  if (sattel==0){
    speedvar=0;}
  else{
  speedvar=gps.speed.kmph(); }            //Получения значения скорости от GPS 
  int nsattel=-1;                       // Переменная для вывода гпс
  smartDelay(0);
speedms=(speedvar*5)/18;               //Переводим км/ч в м/с
}
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
//          Функция одометра
//--------------------------------------------------------------------------------------------------------------------------
void trip(){
if (millis() - timing > 1000){                                      //замеряем разницу времени для пополнения одометра
  timing = millis();
  if(speedvar>4.00){
    real_trip+=speedms;                                            //пополняем одометр
  }
}
if (real_trip-previos_trip>200 || real_trip==0){                              
  EEPROM.put(17, real_trip);
  previos_trip=real_trip;
}
}
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------------------
//          Функция работы насоса 
//--------------------------------------------------------------------------------------------------------------------------
void oiler(){
for(int i=0;i<w_time;i++){                                                     //Основной цикл смазки цепи
  wspeed();                                                                   //Вызываем спидометр для отслеживания скорости в реальном времени
  Serial.println(real_trip);
  btvalue();
  if(real_trip==0){break;}
  if(speedvar<10){                                                           //Усливие, при котором насос не работает на низкой скорости
       digitalWrite(pump,HIGH);                                             //Выключаем помпу
       i--;                                                                // Останавливаем счетчик основного цикла
       continue;                                                          //Возвращаем цикл в начало
    }
    if(speedvar>10 && speedvar<170){                                    //Условие включения насоса от 10 до 60 кмч на 70% времени
    digitalWrite(pump,LOW);                                           //Включаем помпу
    Serial.println("PUMPING!");
    delay(10);                                                       //импульс включения
    digitalWrite(pump,HIGH);
    delay(1000);                                                     //делаем заержку на 1 секунду для синхронизации цикла с реальным временем
    if (i>w_time){                                            //условие выхода из цикла
    break;                                                         // выход из цикла
    }
    }
    if (speedvar>171){                                //Усливие, при котором насос не работает на высокой скорости
      digitalWrite(pump,HIGH);                       //Выключаем помпу
      i--;                                          //Останавливаем счетчик основного цикла
      continue;                                    //Возвращаем цикл в начало
    }
}
digitalWrite(pump,HIGH);                         //Выключаем помпу при выходе из функции
real_trip=0;                                    //Скидываем переменную текущего пробега для следующего цикла работы программы
previos_trip=0;                                //Скидываем маркер одометра для перезаписи на sd
}
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------------------
//          Функция гигрометра и термометра
//--------------------------------------------------------------------------------------------------------------------------

void hum(){
 float h = dht.readHumidity();                    //Получаем данные от датчика влажности
  if (h>80.00){                                  //Условие срабатывания режима "дождь"
    if (w == 0){                                //Условие маркера во избежание дублирования процесса
      city_trip=city_trip/2;                   //Перевод режима город в режим дождь
      hw_trip=hw_trip/2;                      //Перевод режима трасса в режим дождь
      w++;                                   //Проставляем маркер
      Serial.println("Very wet!");
    }
  }
  if(h<80.00){                             //Условие выключения режима дождь
    if(w==1){                             //Условие маркера во избежание дублирования
      city_trip=city_trip*2;             //Возвращение режима город        
      hw_trip=hw_trip*2;                //Возвращение режима трасса
      w--;                              //Проставляем маркер
      Serial.println("Now dry!");
    }
  }
}

//-------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------
//            Получение данных EEPROM
//-------------------------------------------------------------------------------------------------------------------------
void GET_EEPROM_DATA(){
EEPROM.get(0, city_trip);
EEPROM.get(5, hw_trip);
EEPROM.get(7, w_time);
EEPROM.get(12, mode);
EEPROM.get(17, real_trip); 
}
