#ifndef _ESP_CONFIG_H_
#define _ESP_CONFIG_H_

#include <stdint.h>

/* AWG device config */
class ConfigAwgDevice
{
public:
    enum ConfigAwgType
    {
        UNDEFINED = 0,
        FY6800 = 1,
        FY6900 = 2,
        JDS2800 = 3,
        _LAST_
    };

    enum ConfigAwgType deviceType;

    ConfigAwgDevice()
    {
        /* Select the target AWG from: FY6900, FY6800 or JDS2800 */
        deviceType = ConfigAwgType::FY6900;
    }
};


/* WiFi config */
class ConfigWifi
{
public:  
    enum ConfigWifiMode
    {
        WIFI_MODE_AP,
        WIFI_MODE_CLIENT,
    };

    enum ConfigWifiMode wifiMode;
    const char* wifiSsid;
    const char* wifiPsk;
    bool staticIp;
    const char* staticIpAdr;
    const char* staticIpMask;
    const char* staticIpGateway;

    ConfigWifi()
    {
      /* Select Wifi configuration
          WIFI_MODE_AP      : creates new network that oscilloscope can connect to
          WIFI_MODE_CLIENT  : joins existing network 
          staticIp = false  : uses DHCP (make the router assigne always same IP) 
          staticIp = true   : uses static IP definiton (specificaly for AP mode) */
      wifiMode = ConfigWifiMode::WIFI_MODE_CLIENT;
      staticIp = false;
      staticIpAdr = "192.168.1.6";
      staticIpMask = "255.255.255.0";
      staticIpGateway = "192.168.1.1";
    }
};


/* Siglent-AWG Network config */
class ConfigSiglent
{
    //https://www.lxistandard.org/About/VXI-11-and-LXI.aspx
    //https://www.vxibus.org/specifications.html
    /* vxi-11.pdf page 24f
    B.2.4.Core and Abort Channel Establishment Sequence
        Figure B.12 describes the order in which the connection establishment typically takes place for the core
        and abort channels. Note that the second and third create_link request / reply pairs are listed in the figure
        only to emphasize that the same port number is returned on subsequent create_links after the first, and
        that no additional channel creation is necessary after the first create_link sequence is complete.

        Network Client (e.g. PC)                                Network Instrument (e.g. SignalGenerator)
        --------------------------------------------------------------------------------------------------
                                                                create RPC server(abort channel)
                                                                create RPC server(core channel)
                                                                register core channel with portmapper
                                                                be ready to accept connection requests
        create RPC client(core channel.)
        create_link(1)
                                                                reply to create_link(1) - return abort port #
        create RPC client(abort chan., optional)

        create_link(2)
                                                                reply to create_link(2) - return same abort port #
        create_link(3)
                                                                reply to create_link(3) - return same abort port #
    */
    /* vxi-11.pdf page 65f
    Create_LinkResp create_link(Create_LinkParms) = 10;
    Device_WriteResp device_write(Device_WriteParms) = 11;
    Device_ReadResp device_read(Device_ReadParms) = 12;
    Device_ReadStbResp device_readstb(Device_GenericParms) = 13;
    Device_Error device_trigger(Device_GenericParms) = 14;
    Device_Error device_clear(Device_GenericParms) = 15;
    Device_Error device_remote(Device_GenericParms) = 16;
    Device_Error device_local(Device_GenericParms) = 17;
    Device_Error device_lock(Device_LockParms) = 18;
    Device_Error device_unlock(Device_Link) = 19;
    Device_Error device_enable_srq(Device_EnableSrqParms) = 20;
    Device_DocmdResp device_docmd(Device_DocmdParms) = 22;
    Device_Error destroy_link(Device_Link) = 23;
    Device_Error create_intr_chan(Device_RemoteFunc) = 25;
    Device_Error destroy_intr_chan(void) = 26;
    */
public:
    const char*     ID;
    uint16_t        rpcServerPort;
    uint16_t        lxiServerPort;

    const uint32_t  RPC_PROGRAM_PORTMAP          = 0x000186A0;
    const uint32_t  RPC_PROGRAM_VXI11            = 0x000607AF;
    
    const uint32_t  RPC_SINGLE_FRAG              = 0x80000000;
    const uint32_t  RPC_REPLY                    = 0x00000001;
    const uint32_t  PORTMAP_PROCEDURE_GETPORT    = 0x00000003;

    const uint32_t  VXI11_PROCEDURE_CREATE_LINK  = 10;
    const uint32_t  VXI11_PROCEDURE_DESTROY_LINK = 11;
    const uint32_t  VXI11_PROCEDURE_DEV_WRITE    = 12;
    const uint32_t  VXI11_PROCEDURE_DEV_READ     = 23;

    ConfigSiglent()
    {
        ID                           = "IDN-SGLT-PRI SDG1062X\n";
        rpcServerPort                = 111; //maybe 111 is fixed Protmapper channel id ?
        lxiServerPort                = 703;
    }
};


class ConfigEspBode
{
public:
    ConfigWifi      wifiConfig;
    ConfigAwgDevice awgConfig;
    ConfigSiglent   siglentConfig;
};



/* Specify DEBUG output target by defining DEBUG_TO_SERIAL or DEBUG_TO_TELNET (or NONE) */
//#define DEBUG_TO_SERIAL
#define DEBUG_TO_TELNET
// define PRINT macros
#ifndef PRINT_TO_SERIAL
    #define PRINT_TO_SERIAL(TEXT)   Serial.println(TEXT);
#endif
#ifndef PRINT_TO_TELNET
    #define PRINT_TO_TELNET(TEXT)   telnet.println(TEXT);
#endif
// define DEBUG output macro
#ifndef DEBUG
  #ifdef DEBUG_TO_SERIAL
    #define DEBUG(TEXT)         Serial.println(TEXT);
  #endif
  #ifdef DEBUG_TO_TELNET
    #include "ESPTelnet.h"
    extern ESPTelnet telnet;
    #define DEBUG(TEXT)         telnet.println(TEXT);
  #endif
#endif
#ifndef DEBUG
  #define DEBUG(TEXT)
#endif


#endif /* _ESP_CONFIG_H_ */
