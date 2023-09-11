#define voutPin A3
#define readPin 8
#define xckPin 9
#define resetPin 10
#define loadPin 11
#define sinPin 12
#define startPin 13

#define reg0 0b10011111
#define reg1 0b11101000 //ゲイン
#define reg2 0b00000011 //C1 露光時間
#define reg3 0b00000000 //C0 露光時間
#define reg4 0b00000001
#define reg5 0b00000000
#define reg6 0b00000001
#define reg7 0b00000011

#define Xck_H digitalWrite(xckPin,HIGH)
#define Xck_L digitalWrite(xckPin,LOW)

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(2000000);

  pinMode(readPin, INPUT);
  pinMode(xckPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(loadPin, OUTPUT);
  pinMode(sinPin, OUTPUT);
  pinMode(startPin, OUTPUT);

}

// the loop routine runs over and over again forever:
void loop() {
  char buf;
  int pixel;

  digitalWrite(resetPin, LOW); //RESET -> L
  Xck_H;
  Xck_L;
  Xck_H;
  Xck_L;
  digitalWrite(resetPin, HIGH); //RESET -> H リセット解除
  Xck_H;
  Xck_L;

  //レジスタ設定
  setReg(2, reg2);
  setReg(3, reg3);
  setReg(1, reg1);
  setReg(0, reg0);
  setReg(4, reg4);
  setReg(5, reg5);
  setReg(6, reg6);
  setReg(7, reg7);

  Xck_H;
  Xck_L;
  digitalWrite(startPin, HIGH); //スタートH　カメラスタート
  Xck_H;
  digitalWrite(startPin, LOW); //スタートL
  Xck_L;
  Xck_H;

  while (digitalRead(readPin) == LOW) { //READシグナル待
    Xck_L;
    Xck_H;
  }

  //the author made an error about sensor resolution in the initial code so I had to rewrite this part (I left most of the original comments besides that)
  //今回は4列4行毎に32×32画素を取得する //display in 32*32 is quite fun to see, this is the native resolution of a mouse optical sensor
  for (int y = 1; y <= 128; y++) {//rewritten in 2023
    for (int x = 1; x <= 128; x++) {//rewritten in 2023
      Xck_L;
      //シリアルモニタに画像データを表示する 
      if ((((x%4)==0) & ((y%4))==0)) { //4列中１列データ取得
        pixel = (analogRead(voutPin) >> 2); //ADの最大値1024を8bitに圧縮
        if (pixel < 0xF) {//rewritten in 2023
          Serial.print("0");//rewritten in 2023
        }
        Serial.print(pixel, HEX);//rewritten in 2023
        Serial.print(" ");//rewritten in 2023
      }
      Xck_H;
    } // end for x
  } // for y

  Serial.println(" ");
  delay(500);
}

void setReg( unsigned char adr, unsigned char data )
{
  //アドレス転送(3bit)
  if ((adr & 0x04) == 0x04) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((adr & 0x02) == 0x02) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((adr & 0x01) == 0x01) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;

  //データ転送(8bit)
  if ((data & 0x80) == 0x80) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((data & 0x40) == 0x40) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((data & 0x20) == 0x20) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((data & 0x10) == 0x10) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((data & 0x08) == 0x08) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((data & 0x04) == 0x04) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((data & 0x02) == 0x02) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H; Xck_L;
  if ((data & 0x01) == 0x01) {
    digitalWrite(sinPin, HIGH);
  } else {
    digitalWrite(sinPin, LOW);
  }
  Xck_H;
  digitalWrite(loadPin, HIGH); //LOADピン→H
  digitalWrite(sinPin, LOW);
  Xck_L;
  digitalWrite(loadPin, LOW); //LOADピン→L
}
