#include <M5Stack.h>
#include "SPIFFS.h"
#include "FS.h"

void bridge(void * pvParameters) {
  static constexpr uint32_t timeout_msec = 2000;
  static constexpr uint32_t bufsize = 16384;

  SPIFFS.begin(true);
  while (true) {
    while (Serial.available() == 0) delay(5);
    char f = Serial.read();
    if (f == 'D') { 
      //M5.Lcd.println("DOWNLOAD MODE");
  
      // ПОЛУЧЕНИЕ РАЗМЕРА ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА
      size_t sizeOfName = 0;
      for (int i = 3; i > -1; i--) {
        while (Serial.available() < 1) delay(5);
        sizeOfName |= Serial.read() << (i * 8);
      }
      //M5.Lcd.print("Size Of Name: ");
      //M5.Lcd.println(sizeOfName);
  
      // ПОЛУЧЕНИЕ ИМЕНИ СКАЧИВАЕМОГО ФАЙЛА
      String fileName = "";
      for (size_t i = 0; i < sizeOfName; i++) {
        while (Serial.available() < 1) delay(5);
        fileName += (char)Serial.read();
      }
      //M5.Lcd.print("Filename: ");
      //M5.Lcd.println(fileName);
  
      // ПОПЫТКА ОТКРЫТИЯ ФАЙЛА ДЛЯ ЧТЕНИЯ
      File file = SPIFFS.open(fileName, FILE_READ);
      if (!file) {
        //M5.Lcd.print("E: can't open the file");
        //M5.Lcd.println(fileName);
        Serial.write('d');
      } else {
        // ОТПРАВКА РАЗМЕРА ФАЙЛА
        size_t fileSize = file.size();
        for (int i = 3; i > -1; i--) {
          while (Serial.availableForWrite() < 1) delay(5);
          byte x = (fileSize >> (i * 8)) & 0xFF;
          Serial.write((byte)x);
        }
        //M5.Lcd.print("Filesize: ");
        //M5.Lcd.println(fileSize);
    
        // ОТПРАВКА СОДЕРЖИМОГО ФАЙЛА
        //M5.Lcd.print("Blocks: ");
        //M5.Lcd.println(int(fileSize / 1024));
        for (size_t i = 0; i < fileSize; i++) {
          while (Serial.availableForWrite() < 1) delay(5);
          Serial.write((char)file.read());
          //if (i % 1024 == 0) M5.Lcd.print("*");
        }
        file.close();
      }
    } 
  
    else if (f == 'M') {
      // ОТПРАВКА ОБЩЕГО ОБЪЁМА ПАМЯТИ
      size_t totalMemory = SPIFFS.totalBytes();
      for (int i = 3; i > -1; i--) {
        while (Serial.availableForWrite() < 1) delay(5);
        byte x = (totalMemory >> (i * 8)) & 0xFF;
        Serial.write((byte)x);
      }
      //M5.Lcd.print("Total memory: ");
      //M5.Lcd.println(totalMemory);
  
      // ОТПРАВКА ЗАНЯТОГО ОБЪЁМА ПАМЯТИ
      size_t usedMemory = SPIFFS.usedBytes();
      for (int i = 3; i > -1; i--) {
        while (Serial.availableForWrite() < 1) delay(5);
        Serial.write((byte)(usedMemory >> (i * 8)) & 0xFF);
      }
      //M5.Lcd.print("Used memory: ");
      //M5.Lcd.println(usedMemory);
    }
  
    else if (f == 'L') {
      File root = SPIFFS.open("/");
      File file = root.openNextFile();
      size_t sizeOfData = 0;
      while (file) {
        if (!file.isDirectory()) {
          String Name = file.name();
          size_t sizeOfName = Name.length();
          sizeOfData += (sizeOfName + 8);
        }
        file = root.openNextFile();
      }
      root.close();
  
      for (int i = 3; i > -1; i--) {
        while (Serial.availableForWrite() < 1) delay(5);
        Serial.write((sizeOfData >> (i * 8)) & 0xFF);
      }
      
      root = SPIFFS.open("/");
      file = root.openNextFile();
      while (file) {
        if (!file.isDirectory()) {
          String Name = file.name();
          uint8_t sizeOfName = Name.length();
          for (int i = 3; i > -1; i--) {
            while (Serial.availableForWrite() < 1) delay(5);
            Serial.write((sizeOfName >> (i * 8)) & 0xFF);
          }
          size_t fileSize = file.size();
          for (int i = 3; i > -1; i--) {
            while (Serial.availableForWrite() < 1) delay(5);
            Serial.write((fileSize >> (i * 8)) & 0xFF);
          }
          for (uint8_t i = 0; i < sizeOfName; i++) {
            while (Serial.availableForWrite() < 1) delay(5);
            Serial.write(Name[i]);
          }
        }
        file = root.openNextFile();
      }
      root.close();
    }
  
    else if (f == 'R') {
      //M5.Lcd.println("REMOVE MODE");
  
      // ПОЛУЧЕНИЕ РАЗМЕРА ИМЕНИ УДАЛЯЕМОГО ФАЙЛА
      size_t sizeOfName = 0;
      for (int i = 3; i > -1; i--) {
        while (Serial.available() < 1) delay(5);
        sizeOfName |= Serial.read() << (i * 8);
      }
  
      // ПОЛУЧЕНИЕ ИМЕНИ УДАЛЯЕМОГО ФАЙЛА
      String fileName = "";
      for (size_t i = 0; i < sizeOfName; i++) {
        while (Serial.available() < 1) delay(5);
        fileName += (char)Serial.read();
      }
      //M5.Lcd.print("Filename: ");
      //M5.Lcd.println(fileName);
  
      if (SPIFFS.remove(fileName)) {
        Serial.write('R');
      } else {
        Serial.write('r');
      }
    }
  
    else if (f == 'E') {
      //M5.Lcd.println("ERASE MODE");
  
      File root = SPIFFS.open("/");
      if (!root) {
        Serial.write('e');
      } else {
        File file = root.openNextFile();
        while (file) {
          if (!file.isDirectory()){
            SPIFFS.remove(file.name());
          }
          file = root.openNextFile();
        }
        Serial.write('E');
      }
    }
  
    else if (f == 'U') {
      //M5.Lcd.println("UPLOAD MODE");
  
      // ПОЛУЧЕНИЕ РАЗМЕРА ИМЕНИ ВЫГРУЖАЕМОГО ФАЙЛА
      size_t sizeOfName = 0;
      for (int i = 3; i > -1; i--) {
        while (Serial.available() < 1) delay(5);
        sizeOfName |= Serial.read() << (i * 8);
      }
      //M5.Lcd.print("Size Of Name: ");
      //M5.Lcd.println(sizeOfName);
  
      // ПОЛУЧЕНИЕ РАЗМЕРА ВЫГРУЖАЕМОГО ФАЙЛА
      size_t fileSize = 0;
      for (int i = 3; i > -1; i--) {
        while (Serial.available() < 1) delay(5);
        fileSize |= Serial.read() << (i * 8);
      }
      //M5.Lcd.print("Filesize: ");
      //M5.Lcd.println(fileSize);
  
      // ПОЛУЧЕНИЕ ИМЕНИ ВЫГРУЖАЕМОГО ФАЙЛА
      String fileName = "";
      for (size_t i = 0; i < sizeOfName; i++) {
        while (Serial.available() < 1) delay(5);
        fileName += (char)Serial.read();
      }
      //M5.Lcd.print("Filename: ");
      //M5.Lcd.println(fileName);
  
      // ПОПЫТКА ОТКРЫТИЯ ФАЙЛА ДЛЯ ЗАПИСИ
      File file = SPIFFS.open(fileName, FILE_WRITE);
      if (!file) {
        //M5.Lcd.print("E: can't open the file");
        Serial.write('u');
        while (true);
      } else {
        Serial.write('A');
        byte * buf = (byte*)malloc(bufsize);
        uint32_t bufindex = 0;
        // ПОЛУЧЕНИЕ СОДЕРЖИМОГО ФАЙЛА
        uint32_t nodata_count = 0;
        while (nodata_count < timeout_msec) {
          nodata_count = 0;
          while (Serial.available() == 0) { delay(1); if (++nodata_count >= timeout_msec) goto UPLOAD_TIMEOUT;  }
          char f = Serial.read();
          if (f == 'u')  break;
          if (f == 'U') {
            nodata_count = 0;
            Serial.write('A');
            while (Serial.available() == 0) { delay(1); if (++nodata_count >= timeout_msec) goto UPLOAD_TIMEOUT;  }
            uint8_t sizeOfPiece = (uint8_t)Serial.read();
            if (bufindex + sizeOfPiece > bufsize) {
              file.write(buf, bufindex);
              bufindex = 0;
            }
            uint8_t len = 0;
            while (sizeOfPiece) {
              nodata_count = 0;
              while ((len = Serial.available()) == 0) { delay(1); if (++nodata_count >= timeout_msec) goto UPLOAD_TIMEOUT;  }
              if (len > sizeOfPiece) len = sizeOfPiece;
              Serial.readBytes(&buf[bufindex], len);
              bufindex += len;
              sizeOfPiece -= len;
            }
            //M5.Lcd.print("*");
          }
        }
        if (bufindex) file.write(buf, bufindex);
UPLOAD_TIMEOUT:
        Serial.write('u');
        file.close();
        free(buf);
      }
    }
    
    else if (f == 'X') {
      //M5.Lcd.println("EXECUTE MODE");
      // ПОЛУЧЕНИЕ РАЗМЕРА ИМЕНИ ИСПОЛНЯЕМОГО ФАЙЛА
      size_t sizeOfName = 0;
      for (int i = 3; i > -1; i--) {
        while (Serial.available() < 1) delay(5);
        sizeOfName |= Serial.read() << (i * 8);
      }
      //M5.Lcd.print("sizeOfName: ");
      //M5.Lcd.println(sizeOfName);
  
      // ПОЛУЧЕНИЕ ИМЕНИ ИСПОЛНЯЕМОГО ФАЙЛА
      String fileName = "";
      for (size_t i = 0; i < sizeOfName; i++) {
        while (Serial.available() < 1) delay(5);
        fileName += (char)Serial.read();
      }
      //M5.Lcd.print("Filename: ");
      //M5.Lcd.println(fileName);
      String extension = "";
      int idx = fileName.lastIndexOf('.');
      bool success = (idx >= 0);
      if (success) {
        extension = fileName.substring(idx + 1);
        extension.toLowerCase();

        //M5.Lcd.print("Ext: ");
        //M5.Lcd.println(extension);
        if ((extension == "jpg") || (extension == "jpeg")) {
          M5.Lcd.drawJpgFile(SPIFFS, fileName.c_str(), 0, 0);
        }
        else if (extension == "png") {
          M5.Lcd.drawPngFile(SPIFFS, fileName.c_str(), 0, 0);
        } 
        else if (extension == "bmp") {
          M5.Lcd.drawBmpFile(SPIFFS, fileName.c_str(), 0, 0);
        } 
        else {
          success = false;
          //M5.Lcd.println("ext not suppored yet");
        }
      }
      Serial.write(success ? 'X' : 'x');

    }
    else if (f == 'Q') {
      //M5.Lcd.println("RESET MODE");
      ESP.restart();
    }
    delay(5);
  }
}

void setup() {
  M5.begin();
  xTaskCreatePinnedToCore(bridge, "SPIFFS", 4096, NULL, 1, NULL, 1);
}

void loop() {
  
}
