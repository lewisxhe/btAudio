
#include <btAudio.h>
#include <TFT_eSPI.h>
#include<WebServer.h>
#include <Preferences.h>
#include <Esp.h>
/*
build_flags =
    -DUSER_SETUP_LOADED=1
    -DST7789_DRIVER=1
    -DSPI_FREQUENCY=40000000
    -DTFT_MISO=19
    -DTFT_MOSI=23
    -DTFT_SCLK=18
    -DTFT_CS=5
    -DCGRAM_OFFSET
    #TM
    -DTFT_WIDTH=240
    -DTFT_HEIGHT=320
    -DTFT_DC=16
    -DTFT_RST=17
    -DTFT_RGB_ORDER=TFT_BGR
    -DTFT_INVERSION_OFF
* */
btAudio *audio;
String command;
Preferences prefs;
TFT_eSPI tft = TFT_eSPI();


typedef struct {
    float volume;
    char  name[20];
} dev_info_t;

dev_info_t info;

void setup()
{
    // Opens the serial port
    Serial.begin(115200);

    prefs.begin("audio");

    if (prefs.getBytes("info", &info, sizeof(info)) == 0) {
        Serial.println("Restore config...");
        strcpy(info.name, "ESP_Speaker");
        info.volume = 0.8;
        prefs.putBytes("info",  &info, sizeof(info));
    }

    Serial.println("Device boot on:");
    Serial.print("Device Name:"); Serial.println(info.name);
    Serial.print("Device Volume:"); Serial.println(info.volume);

    Serial.println("Serial command : ");
    Serial.println("vol#0   set volume to zero");
    Serial.println("vol#0.8 set volume to 80%");
    Serial.println("vol#1   set volume to 100%");
    Serial.println("name#EspAudio   set device name,max 20 char");

    audio = new btAudio(info.name);

    assert(audio);

    tft.begin();
    tft.initDMA();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLUE);

    // Streams audio data to the ESP32
    audio->begin();

    // Re-connects to last connected device
    audio->reconnect();

    // Outputs the received data to an I2S DAC, e.g. https://www.adafruit.com/product/3678
    int bck = 26;
    int ws = 25;
    int dout = 19;
    audio->I2S(bck, dout, ws);

    audio->volume(info.volume);



}


void loop()
{
    // check if data is available
    if (Serial.available()) {
        // read until a terminator. after this point there should only be numbers
        command = Serial.readStringUntil('#');
        if (command.equals("vol")) {
            // read and set volume
            float vol = Serial.parseFloat();
            if (vol > 1) {
                Serial.println("Volume range is 0.0 ~ 1.0 (float)");
                return;
            }
            Serial.println("Changing Volume");
            audio->volume(vol);

            info.volume = vol;
            prefs.putBytes("info",  &info, sizeof(info));

        } else if (command.equals("name")) {
            String name = Serial.readStringUntil('\n');
            Serial.print("The proposed device name is: "); Serial.println(name);
            Serial.println("Please confirm by typing 'yes' or cancel by typing 'no':");

            unsigned long startTime = millis();
            const unsigned long timeoutDuration = 15000; 

            String confirmation;
            while (true) {
                if (Serial.available()) {
                    confirmation = Serial.readStringUntil('\n');
                    if (confirmation.equals("yes")) {
                        Serial.print("Set device name is "); Serial.println(name);
                        // esp_bt_dev_set_device_name(name.c_str());
                        strncpy(info.name, name.c_str(), 20);
                        prefs.putBytes("info",  &info, sizeof(info));
                        Serial.println("Reboot...");
                        delay(1000);
                        ESP.restart();

                        break;
                    } else if (confirmation.equals("no")) {
                        Serial.println("Device name setting cancelled.");
                        break;
                    } else {
                        Serial.println("Invalid input. Please type 'yes' or 'no'.");
                    }
                }

                unsigned long currentTime = millis();
                if (currentTime - startTime >= timeoutDuration) {
                    Serial.println("Confirmation timeout. Device name setting cancelled.");
                    break;
                }
            }
        }
    }
}
