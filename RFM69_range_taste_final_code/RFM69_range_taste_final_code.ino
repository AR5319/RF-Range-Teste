/*
  RadioLib RF69 Transmit to Address Example

  This example transmits packets using RF69 FSK radio module.
  Packets can have 1-byte address of the destination node.
  After setting node (or broadcast) address, this node will
  automatically filter out any packets that do not contain
  either node address or broadcast address.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#rf69sx1231

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>
#include <Wire.h>             // OLED Library
#include <Adafruit_GFX.h>     // OLED Library
#include <Adafruit_SSD1306.h> // OLED Library

// RF69 has the following connections:
// CS pin:    2
// DIO0 pin:  15
// RESET pin: 16

#define DEVICE_ID_1 1
#define DEVICE_ID_2 2
 #define LOCAL_DEVICE_ID DEVICE_ID_1
// #define LOCAL_DEVICE_ID DEVICE_ID_2
#define FLAG_REQ 0b00000001
#define FLAG_RESP 0b00001000
#define FLAG_DEV_TEST 0b00000010
#define PROTOVERSION 100
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C // I2C address for the display
#define OLED_RESET -1
#define CS 2
#define DIO0 15
#define RESET 16
const uint32_t local_device_id = LOCAL_DEVICE_ID;

uint32_t seq_id;
uint32_t rec_packets;
float tx_total_snr;
float tx_avg_snr;
float tx_total_Rssi;
float tx_avg_Rssi;
float rx_total_snr;
float rx_avg_snr;
float rx_total_Rssi;
float rx_avg_Rssi;
int stateofbutton; // used for long press & short press pushbutton

RF69 radio = new Module(CS, DIO0, RESET);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Initialize the display object
unsigned long startTime = millis();

void sender()
{
  uint8_t flags = FLAG_REQ | FLAG_DEV_TEST;
  Serial.print("\n\nSending packet: ");
  Serial.println(seq_id);
  Serial.printf("Sending Packet %d", seq_id);
  Serial.println(" out of 20");

  // transmit C-string or Arduino string to node with address 0x02
  //  int state = radio.transmit("Hello World!", 0x02);
  //  Serial.println(state);

  byte senders_arr[] = {PROTOVERSION, flags, local_device_id, seq_id, 13};
  int state = radio.transmit(senders_arr, 5, 0x02);
  seq_id++;
  startTime = millis();

  if (state == RADIOLIB_ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("Sent successfully!"));
    display.clearDisplay();
    display.setTextSize(1); // Text size
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("success!");
    display.display();
  }
  else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
  {
    // the supplied packet was longer than 64 bytes
    Serial.println(F("too long!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait for a second before transmitting again
  
}

void senders_receiver()
{

  while ((millis() - startTime) < 4000)
  {
    yield();
    float tx_PacketSnr;
    byte senders_receiver_buff[11];
    int state = radio.receive(senders_receiver_buff, 11);
    memcpy(&tx_PacketSnr, &senders_receiver_buff[6], sizeof(float));

    Serial.print("PacketSnr with memcpy : ");
    Serial.println(tx_PacketSnr);

    if (state == RADIOLIB_ERR_NONE)
    {

      uint8_t protoVersion = senders_receiver_buff[0];
      uint8_t resp_flag = senders_receiver_buff[1];
      uint32_t Sender_id = senders_receiver_buff[2];
      uint32_t seq_id = senders_receiver_buff[3];
      uint32_t device_id = senders_receiver_buff[4];
      int tx_PacketRssi = senders_receiver_buff[5];
      // float tx_PacketSnr = senders_receiver_buff[6];

      Serial.print("Received PROTOVERSION: ");
      Serial.println(protoVersion);

      Serial.print("RESPONSE FLAG : ");
      for (int i = 7; i >= 0; i--)
      {                                     // Print each bit from MSB to LSB
        Serial.print((resp_flag >> i) & 1); // Shift and mask to print the correct bit
      }
      Serial.println("");

      if (protoVersion != PROTOVERSION || Sender_id != LOCAL_DEVICE_ID)
      {
        Serial.printf("Received invalid protoversion %d or sender id %lu\n", protoVersion, Sender_id);
        continue;
      }

      if (resp_flag & FLAG_RESP)
      {
        Serial.println("Received packet from Responder Device ");
      }
      else
      {
        Serial.printf("Invalid Flag :%d\n", resp_flag);
        continue;
      }

      Serial.print("Received LOCAL_DEVICE_ID: ");
      Serial.println(Sender_id);

      Serial.print("Received seq_id: ");
      Serial.println(seq_id);

      Serial.print("DEVICE_ID : ");
      Serial.println(device_id);

      Serial.print("PacketRssi : ");
      Serial.println(tx_PacketRssi);

      Serial.print("PacketSnr with memcpy : ");
      Serial.println(tx_PacketSnr);

      int rx_packetRssi = (-1 * radio.getRSSI());
      float rx_packetSnr = radio.getSNR();

      rx_total_Rssi = rx_total_Rssi + rx_packetRssi;
      rx_total_snr = rx_total_snr + rx_packetSnr;

      tx_total_snr = tx_total_snr + tx_PacketSnr;
      tx_total_Rssi = tx_total_Rssi + tx_PacketRssi;
      rec_packets++;
      break;
    }
  }
}

void actual_receiver()
{
  // Serial.print(F("[RF69] Waiting for incoming transmission ... \n"));

  // you can receive data as an Arduino String
  // String str;
  // int state = radio.receive(str);
  // Serial.println(state);

  byte actual_receiver_buff[5];
  int state = radio.receive(actual_receiver_buff, 5);

  if (state != RADIOLIB_ERR_NONE)
  {
    return;
  }

  Serial.print("Received packet \n ");

  uint8_t protoVersion = actual_receiver_buff[0];
  uint8_t flags = actual_receiver_buff[1];
  uint32_t senderId = actual_receiver_buff[2];
  uint8_t seq_Id = actual_receiver_buff[3];
  

  Serial.print("Received PROTOVERSION: ");
  Serial.println(protoVersion);

  Serial.print("Received Flag: ");
  for (int i = 7; i >= 0; i--)
  {                                 // Print each bit from MSB to LSB
    Serial.print((flags >> i) & 1); // Shift and mask to print the correct bit
  }
  Serial.println("");

  Serial.print("Received seq_id: ");
  Serial.println(seq_Id);

  Serial.print("Received SENDER_ID: ");
  Serial.println(senderId);

  if (protoVersion != PROTOVERSION)
  {
    return;
  }

  if (flags & FLAG_REQ)
  {
    Serial.println("Received packet from request device");
  }
  else
  {
    return;
  }

  // // packet was successfully received
  // Serial.println(F("success!"));
  // Serial.print(byteArr[2], DEC);
  // // print the data of the packet
  // Serial.print(F("[RF69] Data:\t\t"));
  // for (int i = 0; i < 3; i++)
  // {
  //   Serial.print(byteArr[i], DEC);
  //   Serial.print(" ");
  // }
  // Serial.println();

  // Serial.print(F("[RF69] Data:\t\t"));
  // for (int i = 3; i < 4; i++)
  // {
  //   Serial.print(byteArr[i], BIN);
  //   Serial.print(" ");
  // }
  // Serial.println();

    display.clearDisplay();
  display.setTextSize(1.9); // Text size
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Range Teste");
  display.display();
  int tx_packetRssi = (-1 * radio.getRSSI());
  float tx_packetSnr1 = radio.getSNR();
  delay(500);
  responder(protoVersion, senderId, seq_Id, tx_packetRssi, tx_packetSnr1);
}

void responder(uint8_t protoVersion, uint32_t senderId, uint32_t seq_Id, int tx_packetRssi, float tx_packetSnr1)
{
  const int tx_packetSnr1_position = 6;

  Serial.print("Responding to the packet : ");
  Serial.println(seq_Id);

// Serial.print("before arr snr: ");
// Serial.println(tx_packetSnr);

  uint8_t responder_flag = FLAG_DEV_TEST | FLAG_RESP;

  byte responder_arr[] = {protoVersion, responder_flag, senderId, seq_Id, local_device_id, tx_packetRssi, 0,0,0,0, 14};
  memcpy(&responder_arr[tx_packetSnr1_position], &tx_packetSnr1, sizeof(float));
  int state = radio.transmit(responder_arr, 11, 0x02);

  // Serial.print("after arr snr: ");
  // Serial.println(responder_arr[6]);
  if (state == RADIOLIB_ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("Successfully Responded"));
    // display.clearDisplay();
    // display.setTextSize(1); // Text size
    // display.setTextColor(WHITE);
    // display.setCursor(0, 0);
    // display.println("success!");
    // display.display();
  }
  else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
  {
    // the supplied packet was longer than 64 bytes
    Serial.println(F("too long!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // // send packet
  // LoRa.beginPacket();
  // LoRa.write(protoVersion);
  // LoRa.write(FLAG_DEV_TEST | FLAG_RESP);
  // LoRa.write((uint8_t *)&senderId, sizeof(senderId));
  // LoRa.write((uint8_t *)&seq_Id, sizeof(seq_Id));
  // LoRa.write((uint8_t *)&local_device_id, sizeof(local_device_id));

  // LoRa.write(tx_packetRssi);
  // LoRa.write(tx_packetSnr);
  // LoRa.endPacket();
}

void pushbutton()
{
  uint16_t data;
  uint32_t start_time;
  uint32_t total_time;

  data = analogRead(A0);
  if (data <500 && data >100)
  // if (data > 750)
  {
    start_time = millis();

    while (true)
    {
      yield();
      data = analogRead(A0);
      if (data < 100)
      {
        millis();
        break;
      }
    }

    total_time = millis() - start_time;
    if (total_time >= 4000)
    {
      stateofbutton = 2;
      Serial.println("long pressed");
    }
    else if (total_time >= 400)
    {
      Serial.println("  Short Pressed");
      stateofbutton = 1;
    }
    else
    {

      stateofbutton = 0;
    }
  }
  else
  {
    stateofbutton = 0;
  }
}

void combine_function()
{
  uint8_t loss_packets;
  uint8_t packet_loss_percent;
  Serial.println(stateofbutton);

  if (stateofbutton == 1)
  {
    tx_total_snr = 0;
    tx_total_Rssi = 0;
    rx_total_snr = 0;
    rx_total_Rssi = 0;
    rec_packets = 0;
    seq_id = 1;
    tx_avg_snr = 0;
    tx_avg_Rssi = 0;
    rx_avg_snr = 0;
    rx_avg_Rssi = 0;
    for (int i = 0; i < 20; i++)
    {
      sender();
      display.clearDisplay();
      display.setTextSize(1.9); // Text size
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print("SENDING: ");
      display.print(seq_id - 1);
      display.println("/20");
      display.println("");
      senders_receiver();

      tx_avg_snr = tx_total_snr / rec_packets;
      tx_avg_Rssi = (-1 * (tx_total_Rssi / rec_packets));
      if (rec_packets == 0)
      {
        tx_avg_Rssi = 0;
        tx_avg_snr = 0;
      }

      rx_avg_snr = rx_total_snr / rec_packets;
      rx_avg_Rssi = (-1 * (rx_total_Rssi / rec_packets));
      if (rec_packets == 0)
      {
        rx_avg_Rssi = 0;
        rx_avg_snr = 0;
      }

      Serial.printf("tx_Average Snr : %f \n", tx_avg_snr);
      Serial.printf("tx_Average Rssi : %f \n", tx_avg_Rssi);
      Serial.printf("rx_Average Snr : %f \n", rx_avg_snr);
      Serial.printf("rx_Average Rssi : %f \n", rx_avg_Rssi);

      //   display.clearDisplay();
      // display.setTextSize(1.3); // Text size
      // display.setTextColor(WHITE);
      // display.setCursor(0, 0);
      display.print("TX: AVG SNR: ");
      display.println(tx_avg_snr);
      display.print("TX: AVG RSSI: ");
      display.println(tx_avg_Rssi);
      display.println("");
      display.print("RX: AVG SNR: ");
      display.println(rx_avg_snr);
      display.print("RX: AVG RSSI: ");
      display.println(rx_avg_Rssi);
      display.display();
      delay(2000);
    }

    loss_packets = 20 - rec_packets;
    packet_loss_percent = (100 * loss_packets) / 20;

    tx_avg_snr = tx_total_snr / rec_packets;
    tx_avg_Rssi = (-1 * (tx_total_Rssi / rec_packets));
    if (rec_packets == 0)
    {
      tx_avg_Rssi = 0;
      tx_avg_snr = 0;
    }

    rx_avg_snr = rx_total_snr / rec_packets;
    rx_avg_Rssi = (-1 * (rx_total_Rssi / rec_packets));
    if (rec_packets == 0)
    {
      rx_avg_Rssi = 0;
      rx_avg_snr = 0;
    }

    Serial.printf("TX Average Snr : %f \n", tx_avg_snr);
    Serial.printf("TX Average Rssi : %f \n", tx_avg_Rssi);
    Serial.printf("RX Average Snr : %f \n", rx_avg_snr);
    Serial.printf("RX Average Rssi : %f \n", rx_avg_Rssi);
    display.clearDisplay();
    display.setTextSize(2); // Text size
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    if (tx_avg_Rssi >= (-30.0f) && tx_avg_Rssi != 0)
    {
      display.println("TX: STRONG");
    }
    else if (tx_avg_Rssi >= (-60.0f) && tx_avg_Rssi < (-30.0f))
    {
      display.println("TX: MEDIUM");
    }
    else if (tx_avg_Rssi >= (-90.0f) && tx_avg_Rssi < (-60.0f))
    {
      display.println("TX: FAIR");
    }
    else if (tx_avg_Rssi >= (-120.0f) && tx_avg_Rssi < (-90.0f))
    {
      display.println("TX: WEAK");
    }
    else if (tx_avg_Rssi < (-120.0f) || tx_avg_Rssi == 0)
    {
      display.println("TX: FAIL");
    }

    if (rx_avg_Rssi >= (-30.0f) && rx_avg_Rssi != 0)
    {
      display.println("RX: STRONG");
    }
    else if (rx_avg_Rssi >= (-60.0f) && rx_avg_Rssi < (-30.0f))
    {
      display.println("RX: MEDIUM");
    }
    else if (rx_avg_Rssi >= (-90.0f) && rx_avg_Rssi < (-60.0f))
    {
      display.println("RX: FAIR");
    }
    else if (rx_avg_Rssi >= (-120.0f) && rx_avg_Rssi < (-90.0f))
    {
      display.println("RX: WEAK");
    }
    else if (rx_avg_Rssi < (-120.0f) || rx_avg_Rssi == 0)
    {
      display.println("RX: FAIL");
    }

    display.setTextSize(1.9); // Text size
    display.setTextColor(WHITE);
    display.print("PKT LOSS: ");
    display.print(packet_loss_percent);
    display.print("%");
    display.print("(");
    display.print(rec_packets);
    display.print("/20");
    display.println(")");

    display.print("TX: ");
    display.print(tx_avg_Rssi);
    display.print("dBm");
    display.print("/");
    display.println(tx_avg_snr);
    display.print("RX: ");
    display.print(rx_avg_Rssi);
    display.print("dBm");
    display.print("/");
    display.println(rx_avg_snr);
    display.display();
  }
  else
  {
    actual_receiver();
  }
}

void setup()
{
  delay(1000);

  Serial.begin(9600);

  //  Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // 0x3C is OLED I2C address
    Serial.println(F("SSD1306 allocation failed"));
    while (1)
      ; // Stop if initialization fails
  }
  display.clearDisplay();

  // initialize RF69 with default settings
  Serial.print(F("[RF69] Initializing ... 0x01 "));
  int state = radio.begin();
  radio.variablePacketLengthMode(RADIOLIB_RF69_MAX_PACKET_LENGTH);
     state = radio.setOutputPower(20, true);

  
  yield();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("RFM69 Initialized"));
    Serial.println(state);
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
    {
      delay(10);
    }
  }

  state = radio.setFrequency(868.0f);
  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.println("Failed to set 868 MHz");
  }
  else
  {
    Serial.println("Set to 868 MHz");
  }
    //  Serial.print(" state of outpute power : ");
    //  Serial.println(state);


  // set node address
  // NOTE: calling this method will automatically enable
  // address filtering (node address only)
  Serial.print(F("[RF69] Setting node address ... "));
  state = radio.setNodeAddress(0x01);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
    Serial.println(state);
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
    {
      delay(10);
    }
  }

  // set broadcast address
  // NOTE: calling this method will automatically enable
  // address filtering (node or broadcast address)
  Serial.print(F("[RF69] Setting broadcast address ... "));
  state = radio.setBroadcastAddress(0xFF);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
    {
      delay(10);
    }
  }
}

void loop()
{
  pushbutton();
  combine_function();
}
