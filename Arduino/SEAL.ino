//Bibliotecas
#include <Wire.h>              //LCD
#include <LiquidCrystal_I2C.h> //LCD
#include <OneWire.h>           //Sensor de temperatura
#include <DallasTemperature.h> //Sensor de temperatura
#include <SoftwareSerial.h>    //SoftwareSerial para Bluetooth

#define MSGSTR #
#define MSGEND ~
#define SPLIT  | 

//Declaro los pines utilizados para los sensores
int pinTemp   = 2;       //SENSOR DE TEMPERATURA
int pinRelay1 = 3;       //RELAY 1
int pinRelay2 = 4;       //RELAY 2
int echoPin   = 5;       //SENSOR DE ULTRASONIDO - ECHO
int trigPin   = 6;       //SENSOR DE ULTRASONIDO - TRIGGER
SoftwareSerial BT(10,11);// Pines RX y TX del Arduino conectados al Bluetooth

//Declaro los parametros de escritura del LCD
LiquidCrystal_I2C  lcd(0x27,20,4); // 0x27 is the I2C bus address for an unmodified backpack

//Temperatura
OneWire ourWire(pinTemp);                     //Bus para comunicación OneWire (Temperatura)
DallasTemperature sensorTempSumerg(&ourWire); //Se instancia la biblioteca DallasTemperature

//Variables para controlar las bombas de agua
bool encendido1;
bool encendido2;

//Varible de modo de funcionamiento
int ArdCtl=1;

//Variables para lectura de sensores
float capacidad;
int ntu;
int temperatura;

/*---------------------*\
| SETUP                 |
|      Inicializaciones |
\*---------------------*/
void setup() {
  //Inicializamos el Monitor Serie
  Serial.begin (9600);

  //Inicializo variables para controlar las bombas de agua
  encendido1 = false;
  encendido2 = false;

  //Declaramos los pines para el sensor de ultrasonido
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);

  //Se inicializa el sensor de Temperatura Sumergible
  sensorTempSumerg.begin(); 

  //Inicializamos el LCD y encendemos el Backlight
  lcd.init();
  lcd.backlight();
  
  //Declaramos los pines para los relay 1 y 2
  pinMode(pinRelay1,OUTPUT);
  pinMode(pinRelay2,OUTPUT);
  //Al comienzo apagamos ambos relays
  digitalWrite(pinRelay1,HIGH);
  digitalWrite(pinRelay2,HIGH);

  //Inicializamos el puerto serie BT que hemos creado
  BT.begin(9600);

}


/*----------------------------*\
| LOOP:                        |
|      Comportamiento principal|
\*----------------------------*/
void loop() {
  
  //Declaracion de variables locales
  char buffer[64];
  String msjin;
  int i;
  //Leo capacidad
  capacidad = CalcularCapacidad();

  //Leo Nivel de Turbiedad
  ntu = CalcularTurbiedad();

  //Leo Temperatura del Agua
  temperatura = CalcularTemperaturaAgua();
  //Eliminamos todos los caracteres del LCD
  lcd.clear();

//Por proteccion se apagan las bombas al llegar al minimo nivel habilitado
  if (capacidad <= 1.2)
      { if (encendido1) {digitalWrite(pinRelay1,HIGH);
                         encendido1=false;}
        if (encendido2) {digitalWrite(pinRelay2,HIGH);
                         encendido2=false;}
        lcd.setCursor(0,3);       
        lcd.print("Nivel limite");
       }

  //Leemos que nos envia el bluetooth
  if(BT.available())    // Si llega un dato por el puerto BT se envía al monitor serial
  {
    msjin = LeerBT();
    Serial.print(msjin);
    if (msjin.equals("#FA~")){ArdCtl=1;}
    else if(msjin.equals("#FM~")){ArdCtl=0;}
  }

  //Informo el estado por Bluetooth
  EnviarBT();


  //Seteamos el cursor en la 1º linea
  lcd.setCursor(0,0);       
  lcd.print("Temp: ");
  lcd.print(temperatura);
  lcd.print(char(223));
  lcd.print("C");

  //Verifica el estado de funcionamiento
  if (ArdCtl == 0){lcd.setCursor(14,0);
                   lcd.print("MANUAL");}
  else {lcd.setCursor(14,0);
        lcd.print("AUTO");}

  //Seteamos el cursor en la 2º linea
  lcd.setCursor(0,1);
  lcd.print("Nivel: "  );
  lcd.print(capacidad);
  lcd.print(" lts");
  
  //Seteamos el cursor en la 3º linea
  lcd.setCursor(0,2);
  lcd.print("Turbiedad: ");
  lcd.print(ntu);
  lcd.print(" NTU");

if (ArdCtl == 1){
  //Modo autonomo

        //Si llega al punto máximo de agua
          if (capacidad > 3.8)
          {
            if (ntu <= 2)
             {//Agua para consumo, encendemos Relay de Bomba 1
              digitalWrite(pinRelay1,LOW);
              encendido1 = true;
              lcd.setCursor(0,3);
              lcd.print("Agua a consumo");
             }
            else
             {
              digitalWrite(pinRelay2,LOW);
              encendido2 = true;
              lcd.setCursor(0,3);
              lcd.print("Agua a riego");
             }

           //Informo el estado por Bluetooth
           EnviarBT();

            //Mantenemos la bomba encendida hasta que el agua baje
            while (capacidad > 1.2)
                    {capacidad = CalcularCapacidad();
                     lcd.setCursor(0,1);       
                     lcd.print("Nivel: "  );
                     lcd.print(capacidad);
                     lcd.print(" lts");
                     delay(400);
                     }

          }
    }//ArdCtl = 0
     //Modo manual
    else
      {if (msjin.equals("#B11~")&&capacidad > 1.2) {//Abrir bomba 1
                                                    digitalWrite(pinRelay1,LOW);
                                                    encendido1 = true;
                                                    lcd.setCursor(0,3);
                                                    lcd.print("Agua a consumo");
                                                    }
       else if(msjin.equals("#B10~")){//Cerrar bomba 1
                              digitalWrite(pinRelay1,HIGH);
                              encendido1 = false;
                              lcd.setCursor(0,3);
                              lcd.print("Consumo cerrada");
                              }
       else if(msjin.equals("#B21~")&&capacidad > 1.2){//Abrir bomba 2
                                                       digitalWrite(pinRelay2,LOW);
                                                       encendido2 = true;
                                                       lcd.setCursor(0,3);
                                                       lcd.print("Agua a riego");}
       else if(msjin.equals("#B20~")){//Cerrar bomba 2
                                digitalWrite(pinRelay2,HIGH);
                                encendido2 = false;
                                lcd.setCursor(0,3);
                                lcd.print("Riego cerrada");}
       else if(msjin.equals("#BLB~")){//Parpadeo de pantalla
                                      lcd.noBacklight();
                                      delay(100);
                                      lcd.backlight();
                                      delay(100);
                                      lcd.noBacklight();
                                      delay(100);
                                      lcd.backlight();}

       //Informo el estado por Bluetooth
       EnviarBT();
       }
  
  delay(500); 
}


/*------------------------------------*\
| CalcularCapacidad:                   |
|      Retorna nivel del agua en litros|
\*------------------------------------*/
float CalcularCapacidad(){
  float duracion;
  float distancia;
  float capacidad;
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duracion = pulseIn(echoPin,HIGH);
  distancia = (duracion/(float)2) / 29.1;

  capacidad = -0.2656 * distancia + 4.7687;
  
  return capacidad;
  }


/*----------------------------*\
| CalcularTemperaturaAgua:     |
|      Retorna temp del sensor |
\*----------------------------*/
int CalcularTemperaturaAgua(){
  //Prepara el sensor para la lectura
  sensorTempSumerg.requestTemperatures(); 
  return int(sensorTempSumerg.getTempCByIndex(0));
}


/*---------------------------------*\
| CalcularTurbiedad:                |
|      Calcula la turbiedad en "NTU"|
\*---------------------------------*/
int CalcularTurbiedad(){
  //Leemos el pin 0 analógico y obtenemos un valor entre 0 y 1023 
  //Cuanto mas chico sea el valor (particulas suspendidas en el agua) mayor turbidez
  int NTU=0;
  int sensorValue = analogRead(A0);

  if (sensorValue >= 780) NTU = 1;
  else if (780 > sensorValue && sensorValue >= 760 )  NTU = 1.5;
  else if (760 > sensorValue && sensorValue >= 745 )  NTU = 2;
  else if (745 > sensorValue )  NTU =-(166/15)* sensorValue +(24740/3);

  return NTU;  
}

/*---------------------------------*\
| Leer Bluetooth:                   |
|      Lee lo recibido por Bluetooth|
\*---------------------------------*/
String LeerBT(){
  String msj;

  delay(10); //delay de contingencia, para cargar el buffer

//Cargo el buffer en una variable hasta el fin de linea
  while (BT.available()>0){
                    msj+= char(BT.read()); 
                    }

  return msj;
}


/*---------------------------------*\
| Enviar Bluetooth:                 |
|      Envia al Bluetooth lo        |
|      recibido por parametro.      |
\*---------------------------------*/
void EnviarBT(){
  String msjout;
  String ModoFun;
  String EstadoB1;
  String EstadoB2;

  // Armo la cadena de envío de valores de sensores al Bluetooth
  if (ArdCtl==1) ModoFun = "FA";
     else ModoFun = "FM";
  if (encendido1) EstadoB1 = "B11";
     else EstadoB1 = "B10";
  if (encendido2) EstadoB2 = "B21";
     else EstadoB2 = "B20";
  
  msjout = "#SE" + String(capacidad) + "|" + String(temperatura) + "|" + String(ntu) + "|" + ModoFun + "|" + EstadoB1 + "|" + EstadoB2 + "~";
  char msjBT[msjout.length()+1];
  msjout.toCharArray(msjBT,msjout.length()+1);
  
  //Envio el mensaje al BT
  BT.write(msjBT);

}