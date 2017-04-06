/*
ePalnik ver. 3.0
Autor: Rafał Fijałkowski, 01.2016

Aplikacja do sterowania palnikiem biokominka. Dostępne funkcje:
- załączanie/wyłączanie palnika (żarnik);
- opóźnione wyłączanie palnika (timer w smartfonie);
- regulacja otwarcia przepustnicy (otwarcie 40%, 70% i 100%);
- kontrola temperatury żarnika;
- sterowanie ze smartfona oraz przycisków na płytce;
- wyświetlanie parametrów na ekranie LCD i ekranie smartfona.
*/

//---------------- zmienne globalne
int flaga_przepustnica = 0; //znacznik informujący o otwarciu przepustnicy (w %)
char znak; //zmienna przechowująca odebrany przez BT znak ASCII
int odbior; //konwersja znaku ASCII na wartość liczbową
//---------------- obsługa przycisków
#define przyciskStart_pin 14 //wyjście przycisku Start
int przyciskStart = 0; //zmienna przechowująca stan przycisku Start
#define przyciskStop_pin 15 //wyjście przycisku Stop
int przyciskStop = 0; //zmienna przechowująca stan przycisku Stop
#define przyciskTankuj_pin 16 //wyjście przycisku Tankuj
int przyciskTankuj = 0; //zmienna przechowująca stan przycisku Start
#define przyciskPrzepustnica40_pin 17 //wyjście przycisku Przepustnica40 (otwarcie przepustnicy w 40%)
int przyciskPrzepustnica40 = 0; //zmienna przechowująca stan przycisku Przepustnica40
#define przyciskPrzepustnica70_pin 18 //wyjście przycisku Przepustnica70 (otwarcie przepustnicy w 70%)
int przyciskPrzepustnica70 = 0; //zmienna przechowująca stan przycisku Przepustnica70
//---------------- obsługa żarnika i przepustnicy
#define zarnik_pin 19 //wyjście załączające palnik za pośrednictwem przekaźnika
#define zarnik_czas 2000 //czas (w ms) włączenia żarnika
#define przepustnica_czas 5000 //czas (w ms) przerwy między otwarciem przepustnicy, a rozpoczęciem żarzenia
//---------------- obsługa termometru zewnętrznego
#include <Timer.h>
Timer timer_; //timer do odczytu temperatury
#define temp_pin A6 //wyjście analogowe do podłączenia termometru zewnętrznego
int temp_zew = 0; //temperatura palnika
#define temp_palnika 35 //temperatura palnika, po której przekroczeniu zostanie aktywowana przerwa między otwarciem przepustnicy, a rozpoczęciem żarzenia
//---------------- obsługa ekranu LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 2, 3, 4, 5); // przypisanie pinów do ekranu LCD (RS, EN, D4, D5, D6, D7)
//---------------- obsługa silników krokowych przepustnicy
#define ENABLE 6 //włączanie/wyłączanie obu silników
#define DIR_L 7 //kierunek obrotów lewego silnika
#define STEP_L 8 //ruch lewego silnika
#define DIR_P 9 //kierunek obrotów prawego silnika
#define STEP_P 10 //ruch prawego silnika

void setup(void) 
{
  Serial.begin(19200); //inicjalizacja łącza szeregowego (Bluetooth)
  lcd.begin(16, 2); // ustawienia ekranu LCD
  digitalWrite(ENABLE, HIGH); //silniki wyłączone
  //---------------- komunikat na powitanie
  lcd.clear();
  lcd.setCursor(0, 0); //kolumna, wiersz
  lcd.print("ePalnik ver. 3.0"); 
  lcd.setCursor(0, 1); //kolumna, wiersz
  lcd.print("Glob Metal Trade"); 
  delay(5000);
  lcd.clear();
  //---------------- konfiguracja pinów
  pinMode(przyciskStart_pin, INPUT);
  pinMode(przyciskStop_pin, INPUT);
  pinMode(przyciskTankuj_pin, INPUT);
  pinMode(zarnik_pin, OUTPUT);
  pinMode(ENABLE, OUTPUT);
  pinMode(DIR_L, OUTPUT);
  pinMode(STEP_L, OUTPUT);
  pinMode(DIR_P, OUTPUT);
  pinMode(STEP_P, OUTPUT);
  timer_.every(5000, temperatura); //ustawia częstotliwość wywołania funkcji pomiaru temperatury 
}

void loop(void) 
{ 
  //sprawdzenie czy naciśnięto przycisk Start
  przyciskStart = digitalRead(przyciskStart_pin);
  if (przyciskStart == HIGH) {zapal();}
    
  //sprawdzenie czy naciśnięto przycisk Stop
  przyciskStop = digitalRead(przyciskStop_pin);
  if (przyciskStop == HIGH) {zgas();}
    
  //sprawdzenie czy naciśnięto przycisk Tankuj
  przyciskTankuj = digitalRead(przyciskTankuj_pin);
  if (przyciskTankuj == HIGH) {tankuj();}

  //sprawdzenie czy naciśnięto przycisk Przepustnica40
  przyciskPrzepustnica40 = digitalRead(przyciskPrzepustnica40_pin);
  if (przyciskPrzepustnica40 == HIGH) {przepustnica40();}

  //sprawdzenie czy naciśnięto przycisk Przepustnica70
  przyciskPrzepustnica70 = digitalRead(przyciskPrzepustnica70_pin);
  if (przyciskPrzepustnica70 == HIGH) {przepustnica70();}
  
  timer_.update(); //wywołuje funkcję pomiaru temperatury   
  odbierz_dane(); //komunikacja ze smartfonem   
}

void zapal()
{
  if (flaga_przepustnica==0) //otwórz przepustnicę (100%)
  { 
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik ZAPALONY");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, HIGH); //otwarcie przepustnicy
    digitalWrite(DIR_P, LOW); //otwarcie przepustnicy
    for (int i=0; i<200; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=100; 
    
    //jeżeli temperatura palnika przekroczy ustaloną to rozpoczęcie żarzenia rozpocznie się z opóźnieniem
    if (temp_zew >= temp_palnika) delay(przepustnica_czas); 
    
    digitalWrite(zarnik_pin, HIGH); //włączenie żarnika
    digitalWrite(13, HIGH); //włączenie didody LED
    delay(zarnik_czas);
    digitalWrite(zarnik_pin, LOW); //wyłączenie żarnika  
    digitalWrite(13, LOW); //wyłączenie diody LED
  }
}

void zgas()
{  
  if (flaga_przepustnica==100) //zamknij przepustnicę (0%)
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik ZGASZONY");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, LOW); //zamknięcie przepustnicy
    digitalWrite(DIR_P, HIGH); //zamknięcie przepustnicy
    for (int i=0; i<200; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=0; 
  }
  if (flaga_przepustnica==999) //zakończ proces tankowania paliwa, zamknij przepustnicę (0%)
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik ZGASZONY");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, LOW); //zamknięcie przepustnicy
    digitalWrite(DIR_P, HIGH); //zamknięcie przepustnicy
    for (int i=0; i<200; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników   
    flaga_przepustnica=0; 
  }
  if (flaga_przepustnica==40) //zamknij przepustnicę (0%)
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik ZGASZONY");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, LOW); //zamknięcie przepustnicy
    digitalWrite(DIR_P, HIGH); //zamknięcie przepustnicy
    for (int i=0; i<60; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=0; 
  }
  if (flaga_przepustnica==70) //zamknij przepustnicę (0%)
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik ZGASZONY");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, LOW); //zamknięcie przepustnicy
    digitalWrite(DIR_P, HIGH); //zamknięcie przepustnicy
    for (int i=0; i<120; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=0; 
  }
}

void tankuj()
{
  if (flaga_przepustnica==0) //otwórz przepustnicę (100%)
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik TANKUJ  ");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, HIGH); //otwarcie przepustnicy
    digitalWrite(DIR_P, LOW); //otwarcie przepustnicy
    for (int i=0; i<200; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=999;
  }
}

void przepustnica40()
{
  if (flaga_przepustnica==100) //przymknij przepustnicę (40%)
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik OTW. 40%");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, LOW); //zmniejszenie otwarcia przepustnicy
    digitalWrite(DIR_P, HIGH); //zmniejszenie otwarcia przepustnicy
    for (int i=0; i<140; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=40;
  }
  if (flaga_przepustnica==70) //przymknij przepustnicę (40%)
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik OTW. 40%");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, LOW); //zwiększenie otwarcia przepustnicy
    digitalWrite(DIR_P, HIGH); //zwiększenie otwarcia przepustnicy
    for (int i=0; i<60; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=40;
  }
}

void przepustnica70()
{
  if (flaga_przepustnica==100) //przymknij przepustnicę do 70%
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik OTW. 70%");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, LOW); //zmniejszenie otwarcia przepustnicy
    digitalWrite(DIR_P, HIGH); //zmniejszenie otwarcia przepustnicy
    for (int i=0; i<80; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=70;
  }
  if (flaga_przepustnica==40) //przymknij przepustnicę do 70%
  {
    lcd.setCursor(0, 0); //kolumna, wiersz
    lcd.print("ePalnik OTW. 70%");
    digitalWrite(ENABLE, LOW); //włączenie silników
    digitalWrite(DIR_L, HIGH); //zwiększenie otwarcia przepustnicy
    digitalWrite(DIR_P, LOW); //zwiększenie otwarcia przepustnicy
    for (int i=0; i<60; i++) //obrót silników
    {
      digitalWrite(STEP_L, HIGH);
      digitalWrite(STEP_P, HIGH);
      delayMicroseconds(1000);          
      digitalWrite(STEP_L, LOW); 
      digitalWrite(STEP_P, LOW);
      delayMicroseconds(1000); 
    }  
    digitalWrite(ENABLE, HIGH); //wyłączenie silników
    flaga_przepustnica=70;
  }
}

void temperatura()
{
  //pomiar temperatury paleniska
  int wart_czuj_temp=0;
  for (int i=0; i<20; i++)
  {
    wart_czuj_temp = wart_czuj_temp + analogRead(temp_pin);
  }
  wart_czuj_temp = wart_czuj_temp / 20; //obliczenie średniej arytmetycznej z 20 pomiarów
  temp_zew = (5.0*wart_czuj_temp*100)/1024.0; //przeliczenie wartości na stopnie Celsjusza
  lcd.setCursor(0, 1); //kolumna, wiersz
  lcd.print("Temp. " + String(temp_zew)); lcd.write(byte(223)); lcd.print("C."); //wyświetla temperaturę na LCD
  //Serial.print("Temperatura paleniska = "); Serial.println(temp_zew); //podgląd danych (debugowanie)
  Serial.print("%"); Serial.println((int)temp_zew); //wysyła odczytaną temperaturę przez BT
}

void odbierz_dane()
{
  if (Serial.available()) 
  {
    znak = Serial.read(); //odczyt danych z BT
    odbior = char(znak); //konwersja kodu ASCII na znak
    //Serial.print(znak); //podgląd danych (debugowanie)
    
    if (odbior==42) //znak * (przycisk Włącz)
    {
      zapal();     
    }   
    else if (odbior==35) //znak # (przycisk Wyłącz)
    {
      zgas();
    }
    else if (String(odbior).startsWith("36")) //znak $ (suwak Przepustnica)
    {
      int kat;
      kat = Serial.parseInt();  
      if (kat>10 and kat<45) przepustnica40();
      if (kat>46 and kat<90) przepustnica70(); 
    }
  }  
}

