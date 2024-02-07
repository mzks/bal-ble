/*********************************************************************
This is an example on a peripheral device
Edit 
  // --   User should edit here   --

  // -------------------------------
setup() and loop() functions for your sensors

*********************************************************************/

#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery

// --   User should edit here   --
// include and global variables
#include <Tle493d_w2b6.h>
Tle493d_w2b6 Tle493dMagnetic3DSensor = Tle493d_w2b6(Tle493d_w2b6::MASTERCONTROLLEDMODE, Tle493d_w2b6::TLE493D_W2B6_A1);


#define HICHG 22
// -------------------------------


void setup(){

  Bluefruit.autoConnLed(true);
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setTxPower(8); // nRF52840: -40dBm, -20, -16, -12, -8, -4, 0, 2, 3, 4, 5, 6, 7 and +8dBm are supported.

  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  bledfu.begin();
  bledis.setManufacturer("Manifucture nane");
  bledis.setModel("Bal-ble board");
  bledis.begin();

  bleuart.begin();
  blebas.begin();
  blebas.write(100);  // dummy

  startAdv();
  // --   User should edit here   --
  // write actual sensor setup
  Tle493dMagnetic3DSensor.begin();
  Tle493dMagnetic3DSensor.enableTemp();


  pinMode(HICHG, LOW); // high charging current
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(10);
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  // -------------------------------

}

void startAdv(void)
{
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

int get_battery_level(){
  int vbat_raw = analogRead(PIN_VBAT);
  return vbat_raw;
}

int sleep_time = 1000;  // unit : ms
bool data_sending = false;  // default false

void loop()
{
  // Send data
  if (data_sending)
  {
      // --   User should edit here   --
      // write actual sensor value(s)
      Tle493dMagnetic3DSensor.updateData();
      float x = Tle493dMagnetic3DSensor.getX();
      float y = Tle493dMagnetic3DSensor.getY();
      float z = Tle493dMagnetic3DSensor.getZ();
      int bat = get_battery_level();

      String send_text = String(x) + "," + String(y) + "," + String(z) + "," + String(bat) + "\n";
      char tx_string[40];
      send_text.toCharArray(tx_string, 40);
      // -------------------------------
      bleuart.write( tx_string, strlen(tx_string) );
  }


  // Recieve commands
  String command = "";
  while ( bleuart.available() )
  {
    uint8_t ch;
    ch = (uint8_t) bleuart.read();
    command += (char)ch;
  }

  // --   User should edit here   --
  // define your command here
  if (command == "stop\n") data_sending = false;
  if (command == "start\n") data_sending = true;
  if (command == "fast\n") sleep_time = 50;
  if (command == "slow\n") sleep_time = 1000;


  // -------------------------------

  delay(sleep_time);
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
}
