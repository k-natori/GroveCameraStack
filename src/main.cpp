#include <Arduino.h>
#include <M5Stack.h>
#include "esp_camera.h"

// put function declarations here:
String nextFileName();

boolean setupCompleted = false;
int fileIndex = 0;
int fileCount = 0;
size_t jpegSize = 0;

void setup()
{
  M5.begin(true, true, true, false);

  Serial1.begin(115200, SERIAL_8N1, G22, G21); // RX: 22, TX: 21

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(1, 10);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(3);

  M5.Lcd.println("Left: power off");
  M5.Lcd.println("Mid: capture");
  M5.Lcd.println("Right: toggle picture");
  if (!SD.begin())
  {
    M5.Lcd.println("No SD card");
  } else {
    Serial.println(nextFileName());
    fileIndex = fileCount;
    fileCount--;
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  M5.update();

  // Left button pressed: power off
  if (M5.BtnA.wasPressed())
  {
    delay(100);
    M5.Power.powerOFF();
  }

  // Middle button pressed: capture image
  if (M5.BtnB.wasPressed())
  {
    if (!setupCompleted) {
      // setup capture size and direction
      Serial.printf("SETUP_SIZE:%d\n", FRAMESIZE_QVGA);
      Serial1.printf("SETUP_SIZE:%d\n", FRAMESIZE_QVGA);

      Serial.println("SETUP_VFLIP:FALSE");
      Serial1.println("SETUP_VFLIP:FALSE");

      Serial.println("SETUP_HMIRROR:FALSE");
      Serial1.println("SETUP_HMIRROR:FALSE");

      setupCompleted = true;
    }
    // Command to start capture image
    Serial.println("CAPTURE:");
    Serial1.println("CAPTURE:");
  }

  // Right button pressed: show previous images
  if (M5.BtnC.wasPressed())
  {
    if (fileIndex == 0 && fileCount == 0) return;
    fileIndex--;
    if (fileIndex < 0) fileIndex = fileCount;
    String fileName = "/capture" + String(fileIndex) + ".jpg";
    M5.Lcd.drawJpgFile(SD, fileName.c_str());
  }

  // Serial1 port listening
  if (Serial1.available() > 0)
  {
    String line = Serial1.readStringUntil('\n');
    Serial.println(line);

    if (line.startsWith("JPEG_SIZE:"))
    { // JPEG_SIZE: response contains byte length of captured JPEG image
      // Store byte length in jpegSize

      String sizeString = line.substring(strlen("JPEG_SIZE:"));
      jpegSize = sizeString.toInt();
      Serial.printf("jpeg size:%d\n", jpegSize);
    }

    else if (line.startsWith("JPEG_START:"))
    { // JPEG_START: response is marker before binary transfer
      // Start receiving binary

      // allocate buffer with JPEG_SIZE: byte length
      size_t receivedSize = 0;
      byte *buffer = (byte *)malloc(jpegSize * sizeof(byte));
      if (buffer == NULL)
      {
        Serial.println("malloc failed");
        return;
      }
      
      int timeoutCount = 0;
      while (receivedSize < jpegSize)
      {
        if (Serial1.available() > 0)
        { // Receive available bytes from Serial1
          size_t readLength = Serial1.readBytes(buffer + receivedSize, jpegSize - receivedSize);
            receivedSize += readLength;
            if (receivedSize == jpegSize)
              break;
          timeoutCount = 0;
        }

        else
        { // If no bytes available, increment timeout counter
          timeoutCount++;
          if (timeoutCount > 100)
          {
            Serial.println("Timeout");
            M5.Lcd.println("Timeout");
            free(buffer);
            return;
          }
          delay(1);
        }
      }
      Serial.printf("jpeg received:%d / %d\n", receivedSize, jpegSize);

      // Draw JPEG on LCD
      M5.Lcd.drawJpg(buffer, jpegSize);

      // Save JPEG on micro SD card
      Serial.println("create .jpg file");
      String fileName = nextFileName();
      fileIndex = fileCount;
      Serial.println(fileName);
      File jpgFile = SD.open(fileName, FILE_WRITE);
      jpgFile.write(buffer, jpegSize);
      jpgFile.close();
      free(buffer);
      // buffer = NULL;

      Serial.println("wrote .jpg file");
    }
  }
}

String nextFileName()
{
  String fileName = "/capture" + String(fileCount) + ".jpg";
  while (SD.exists(fileName))
  {
    fileCount++;
    fileName = "/capture" + String(fileCount) + ".jpg";
  }
  return fileName;
}