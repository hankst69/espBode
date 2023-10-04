/* WiFi credentials */
#define WIFI_SSID   "wlan_ssid"
#define WIFI_PSK    "wlan_key"

#include <ESP8266WiFi.h>
#include "esp_config.h"
#include "EspNetwork.h"
#include "ESPTelnet.h"
#include "AwgDevice.h"
// include the available concrete Awg device implementations
#include "AwgFY6800.h"
#include "AwgFY6900.h"
#include "AwgJDS2800.h"

ConfigEspBode *g_espConfig;
WiFiServer *g_rpc_server;
WiFiServer *g_lxi_server;
AwgDevice *g_awgDevice;
HardwareSerial *g_serial;
ESPTelnet telnet;

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);

    g_serial = &Serial;
    g_serial->begin(115200);

    // workaround to get g_serial->print() working (synching esp with console)
    for (int i=0; i<6; i++) { g_serial->println("-"); delay(250); }


    g_serial->println("\n----- ESP info -----");
    // inform about ESP flash capacity
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();
    g_serial->printf("Flash real id:   %08X\n", ESP.getFlashChipId());
    g_serial->printf("Flash real size: %u bytes\n", realSize);
    g_serial->printf("Flash ide  size: %u bytes\n", ideSize);
    g_serial->printf("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
    g_serial->printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO  ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
    if (ideSize != realSize) {
      g_serial->println("Flash chip configuration wrong!");
    } else {
      g_serial->println("Flash chip configuration ok.");
    }


    g_serial->println("\n----- Loading config -----");
    // todo: load espConfig from Filesystem and/or define interactively via seial interface...
    g_espConfig = new ConfigEspBode();
    /* Select the target AWG device */
    g_espConfig->awgConfig.deviceType = ConfigAwgType::FY6900;
    /* Select Wifi configuration
        WIFI_MODE_AP      : creates new network that oscilloscope can connect to
        WIFI_MODE_CLIENT  : joins existing network 
        staticIp = false  : uses DHCP (make the router assigne always same IP) 
        staticIp = true   : uses static IP definiton (specificaly for AP mode) */
    g_espConfig->wifiConfig.wifiMode = ConfigWifiMode::WIFI_MODE_CLIENT;
    g_espConfig->wifiConfig.staticIp = false;
    g_espConfig->wifiConfig.staticIpAdr = "192.168.1.6";
    g_espConfig->wifiConfig.staticIpMask = "255.255.255.0";
    g_espConfig->wifiConfig.staticIpGateway = "192.168.1.1";


    g_serial->println("\n----- Enter setup -----");


    g_serial->println("\n----- Connecting WiFi -----");
    // configuring and connecting to WiFi
    if (g_espConfig->wifiConfig.staticIp) {
        IPAddress ip, mask, gateway;
        ip.fromString(g_espConfig->wifiConfig.staticIpAdr);
        mask.fromString(g_espConfig->wifiConfig.staticIpMask);
        gateway.fromString(g_espConfig->wifiConfig.staticIpGateway);
        WiFi.config(ip, gateway, mask);
    }
    if (g_espConfig->wifiConfig.wifiMode == ConfigWifiMode::WIFI_MODE_CLIENT) {
        WiFi.mode(WiFiMode_t::WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PSK);
    } else if (g_espConfig->wifiConfig.wifiMode == ConfigWifiMode::WIFI_MODE_AP) {
        WiFi.softAP(WIFI_SSID, WIFI_PSK);
    } else {
      //#error PLEASE SELECT WIFI_MODE_AP OR WIFI_MODE_CLIENT!
    }
    // We start by connecting to a WiFi network
    g_serial->printf("connecting to '%s'  ", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        g_serial->print(".");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
    g_serial->printf("\nWiFi connected (IP address %s)\n", WiFi.localIP().toString().c_str());


    g_serial->println("\n----- Connecting to UART AWG device -----");
    // create AwgDevice
    if (g_espConfig->awgConfig.deviceType == ConfigAwgType::FY6800) {
      g_awgDevice = new AwgFY6800(g_serial);
    } else if (g_espConfig->awgConfig.deviceType == ConfigAwgType::FY6900) {
      g_awgDevice = new AwgFY6900(g_serial);
    } else if (g_espConfig->awgConfig.deviceType == ConfigAwgType::JDS2800) {
      //g_awgDevice = new AwgDS2800(g_serial);
    }
    // init awg (should be done by Siglent emulator)
    g_awgDevice->initDevice(g_awgDevice->getDeviceDefaults());


    g_serial->println("\n----- Starting Siglent AWG LXI Network emulation -----");
    // network initialization
    g_rpc_server = new WiFiServer(g_espConfig->siglentConfig.rpcServerPort);
    g_lxi_server = new WiFiServer(g_espConfig->siglentConfig.lxiServerPort); 

    telnet.begin();
    g_rpc_server->begin();
    g_lxi_server->begin();
}


void loop() {

    WiFiClient rpc_client;
    do
    {
        rpc_client = g_rpc_server->accept();
    }
    while(!rpc_client);
    DEBUG("RPC connection established");

    EspNetwork *rpcHandler = new EspNetwork(&rpc_client, &(g_espConfig->siglentConfig), g_awgDevice);

    rpcHandler->handlePacket();
    delete(rpcHandler);
    rpc_client.stop();

    WiFiClient lxi_client;
    //lxi_client.setTimeout(1000);
    do
    {
        lxi_client = g_lxi_server->accept();
    }
    while(!lxi_client);
    lxi_client.setTimeout(1000);
    DEBUG("LXI connection established");

    EspNetwork *lxiHandler = new EspNetwork(&lxi_client, &(g_espConfig->siglentConfig), g_awgDevice);

    while(1)
    {
        telnet.loop();        
        if(0 != lxiHandler->handlePacket())
        {
            delete(lxiHandler);
            lxi_client.stop();
            DEBUG("RESTARTING");
            return;
        }else{
          // Lets give the user some feedback
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
    }        
}
