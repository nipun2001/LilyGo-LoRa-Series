/*
  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>

// Uncomment on one node to initiate the packet exchange.
//#define INITIATING_NODE

// Board pin definitions
#define I2C_SDA                     18
#define I2C_SCL                     17
#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              3
#define RADIO_MOSI_PIN              6
#define RADIO_CS_PIN                7
#define SDCARD_MOSI                 11
#define SDCARD_MISO                 2
#define SDCARD_SCLK                 14
#define SDCARD_CS                   13
#define BOARD_LED                   37
#define LED_ON                      HIGH
#define BUTTON_PIN                  0
#define ADC_PIN                     1
#define RADIO_RST_PIN               8
#define RADIO_DIO1_PIN              9       // SX1280 DIO1 = IO9
#define RADIO_BUSY_PIN              36      // SX1280 BUSY = IO36
#define RADIO_RX_PIN                21      // SX1280 RX = IO21
#define RADIO_TX_PIN                10      // SX1280 TX = IO10
#define BOARD_VARIANT_NAME          "T3-S3-SX1280PA"

#define CONFIG_RADIO_FREQ           2400.0
#define CONFIG_FLRC_BIT_RATE        520

#define USE_FLRC
// Comment out USE_FLRC to test with LoRa instead.

/*
* Important: SX1280 PA Version

* The 2.4G version does not have a power amplifier (PA). The permissible power setting is 13dBm.
* If it is a version with a built-in PA, do not set the maximum power beyond 3dBm.
* This is because a power amplifier is added to the RF front end; setting it to 3dBm will achieve an output power of 22dBm.
* Setting it beyond 3dBm may damage the PA.
*/
#define CONFIG_RADIO_OUTPUT_POWER   3

SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

void setFlag(void)
{
    // we sent or received a packet, set the flag
    operationDone = true;
}

void setup()
{
    Serial.begin(115200);

    pinMode(BOARD_LED, OUTPUT);

    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);

    // Both nodes must use the same modem and matching radio settings.
    Serial.print(F("[SX1280PA] Initializing ... "));
    int state;
#if defined(USE_FLRC)
    ConfigFLRC_t config;
    config.frequency = CONFIG_RADIO_FREQ;
    config.bitRate = CONFIG_FLRC_BIT_RATE;
    config.codingRate = RADIOLIB_FLRC_CR_2_3;
    config.power = CONFIG_RADIO_OUTPUT_POWER;
    config.preambleLength = 16;
    config.dataShaping = RADIOLIB_SHAPING_1_0;
    state = radio.beginFLRC(config);
#else
    state = radio.begin(CONFIG_RADIO_FREQ);
    if (state == RADIOLIB_ERR_NONE) {
        state = radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER);
    }
#endif

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true) {
            delay(10);
        }
    }

    // set the function that will be called
    // when new packet is received
    radio.setDio1Action(setFlag);

    // Set RF switch pins
    radio.setRfSwitchPins(RADIO_RX_PIN, RADIO_TX_PIN);

#if defined(USE_FLRC)
    uint8_t syncWord[] = {0x03, 0x23, 0x45, 0x68};
    state = radio.setSyncWord(syncWord, 4);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Unable to set configuration, code "));
        Serial.println(state);
        while (true) { delay(10); }
    }
#endif

#if defined(INITIATING_NODE)
    // send the first packet on this node
    Serial.print(F("[SX1280PA] Sending first packet ... "));
    transmissionState = radio.startTransmit("Hello World!");
    transmitFlag = true;
#else
    // Start listening for packets on this node.
    Serial.print(F("[SX1280PA] Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true) {
            delay(10);
        }
    }
#endif
}

void loop()
{
    if (operationDone) {
        operationDone = false;

        String str;
        int state = radio.readData(str);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("[SX1280PA] Received packet!");

            Serial.print("[SX1280PA] Data: ");
            Serial.println(str);

            Serial.print("[SX1280PA] Packet length: ");
            Serial.println(str.length());

            Serial.print("[SX1280PA] RSSI: ");
            Serial.println(radio.getRSSI());

            Serial.print("[SX1280PA] SNR: ");
            Serial.println(radio.getSNR());

        } else {
            Serial.print("[SX1280PA] Receive error: ");
            Serial.println(state);
        }

        radio.startReceive();
    }
}