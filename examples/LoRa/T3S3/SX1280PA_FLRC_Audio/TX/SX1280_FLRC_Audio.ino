/*
  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>

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

// #define USE_FLRC
// Comment out USE_FLRC to test with LoRa instead.

/*
* Important: SX1280 PA Version

* The 2.4G version does not have a power amplifier (PA). The permissible power setting is 13dBm.
* If it is a version with a built-in PA, do not set the maximum power beyond 3dBm.
* This is because a power amplifier is added to the RF front end; setting it to 3dBm will achieve an output power of 22dBm.
* Setting it beyond 3dBm may damage the PA.
*/
#define CONFIG_RADIO_OUTPUT_POWER   3



// Binary packet: 4-byte sequence number followed by 120 signed 16-bit samples.
const uint16_t SAMPLE_RATE = 16000;
const uint16_t FREQUENCY = 440;
const size_t SAMPLES_PER_PACKET = 120;
const size_t PACKET_HEADER_SIZE = 4;
const size_t PACKET_SIZE = PACKET_HEADER_SIZE + SAMPLES_PER_PACKET * sizeof(int16_t);
uint8_t txPacket[PACKET_SIZE];


SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

int transmissionState = RADIOLIB_ERR_NONE;
volatile bool operationDone = false;
float phase = 0;
uint32_t packetSequence = 0;

void setFlag(void)
{
    operationDone = true;
}

void startAudioPacket()
{
    // Header of the packet
    txPacket[0] = packetSequence & 0xFF;
    txPacket[1] = (packetSequence >> 8) & 0xFF;
    txPacket[2] = (packetSequence >> 16) & 0xFF;
    txPacket[3] = (packetSequence >> 24) & 0xFF;
    packetSequence++;

    // Generating the samples
    for (size_t i = 0; i < SAMPLES_PER_PACKET; i++) {
        int16_t sample = 12000 * sin(phase);
        size_t offset = PACKET_HEADER_SIZE + i * sizeof(sample);
        uint16_t encodedSample = (uint16_t)sample;
        txPacket[offset] = encodedSample & 0xFF;
        txPacket[offset + 1] = encodedSample >> 8;

        phase += 2.0f * PI * FREQUENCY / SAMPLE_RATE;
        if (phase >= 2.0f * PI) {
            phase -= 2.0f * PI;
        }
    }

    transmissionState = radio.startTransmit(txPacket, sizeof(txPacket));

    if (transmissionState != RADIOLIB_ERR_NONE) {
        Serial.print(F("[SX1280PA] Transmit start failed, code "));
        Serial.println(transmissionState);
        while (true) {
            delay(10);
        }
    }

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
    Serial.println(F("Running FLRC mode"));

#else // Use LoRa mode
    ConfigLoRa_t config;
    config.frequency = CONFIG_RADIO_FREQ;
    config.bandwidth = 812.5;
    config.spreadingFactor = 5;
    config.codingRate = 7;  // 4/7
    config.syncWord = RADIOLIB_SX128X_SYNC_WORD_PRIVATE;
    config.power = CONFIG_RADIO_OUTPUT_POWER;
    config.preambleLength = 12;           

    state = radio.begin(config);
    Serial.println(F("Running LoRa mode"));

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

    radio.setPacketSentAction(setFlag);

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

    Serial.println(F("[SX1280PA] Starting binary packet stream"));
    startAudioPacket();
}

void loop()
{
    if (!operationDone) {
        return;
    }

    operationDone = false;
    int state = radio.finishTransmit();
    if (state != RADIOLIB_ERR_NONE) {
        // handle error
    }

    // delay(10);  // diagnostic only: pause between packets
    startAudioPacket();
}
