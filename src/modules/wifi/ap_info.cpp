#include "ap_info.h"

#include <WiFi.h>
#include "esp_wifi.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/globals.h"
#include "lwip/etharp.h"

String autoMode2String(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN:
            return "OPEN";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA_WPA2_PSK";
        case WIFI_AUTH_ENTERPRISE:
            return "WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2_WPA3_PSK";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI_PSK";
        case WIFI_AUTH_WPA3_ENT_192:
            return "WPA3_ENT_192";
        default:
            return "UNKNOWN";
    }
}

String cypherType2String(wifi_cipher_type_t cipherType) {
    switch (cipherType) {
        case WIFI_CIPHER_TYPE_NONE:
            return "NONE";
        case WIFI_CIPHER_TYPE_WEP40:
            return "WEP40";
        case WIFI_CIPHER_TYPE_WEP104:
            return "WEP104";
        case WIFI_CIPHER_TYPE_TKIP:
            return "TKIP";
        case WIFI_CIPHER_TYPE_CCMP:
            return "CCMP";
        case WIFI_CIPHER_TYPE_TKIP_CCMP:
            return "TKIP_CCMP";
        case WIFI_CIPHER_TYPE_AES_CMAC128:
            return "AES_CMAC128";
        case WIFI_CIPHER_TYPE_SMS4:
            return "SMS4";
        case WIFI_CIPHER_TYPE_GCMP:
            return "GCMP";
        case WIFI_CIPHER_TYPE_GCMP256:
            return "GCMP256";
        case WIFI_CIPHER_TYPE_AES_GMAC128:
            return "AES_GMAC128";
        case WIFI_CIPHER_TYPE_AES_GMAC256:
            return "AES_GMAC256";
        case WIFI_CIPHER_TYPE_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

String phyModes2String(wifi_ap_record_t record) {
    String modes;
    if( record.phy_11b || record.phy_11g 
        || record.phy_11g || record.phy_11n ) modes = "11";
    if (record.phy_11b) modes += "b/";
    if (record.phy_11g) modes += "g/";
    if (record.phy_11n) modes += "n/";
    if (record.phy_lr)  modes += "low/";
    if ( !modes.isEmpty() ) modes[modes.length() - 1] = ' '; 

    if( record.ftm_responder || record.ftm_initiator ){
      modes += "FTM: ";
      if( record.ftm_responder ) modes += "RESP ";
      if( record.ftm_initiator ) modes += "INIT ";
    } 

    return modes.isEmpty() ? "None" : modes;
}

String getChannelWidth(wifi_second_chan_t secondChannel) {
    switch (secondChannel) {
        case WIFI_SECOND_CHAN_NONE:
            return "HT20";  // 20 MHz channel width (no secondary channel)
        case WIFI_SECOND_CHAN_ABOVE:
            return "HT40+"; // 40 MHz channel width with secondary channel above
        case WIFI_SECOND_CHAN_BELOW:
            return "HT40-"; // 40 MHz channel width with secondary channel below
        default:
            return "Unknown";
    }
}

int16_t y = 30;
int16_t yCoursor(){
  auto tmp = y;
  y += 12;
  return tmp;
}

void displayAPInfo(){
    drawMainBorder();
    tft.setTextSize(FP);
    tft.setCursor(8,yCoursor());

    wifi_ap_record_t ap_info;
    err_t res;
    if( (res = esp_wifi_sta_get_ap_info(&ap_info)) != ESP_OK ){
      String err;
      switch (res)
      {
      case ESP_ERR_WIFI_CONN:
        err = "iface is not initialized";
        break;
      case ESP_ERR_WIFI_NOT_CONNECT:
        err = "station disconnected";
        break;
      default:
        err = "failed with" + String(res);
        break;
      }
      
      tft.print(err);

      while(checkSelPress()) yield();
      while(!checkSelPress()) yield();
    }

    auto mac = MAC(ap_info.bssid);

    // organized in the most to least useful order

    // add internet connection
    // add rx/tx speed
    // make scrollable
    tft.print("SSID: " + String((char*)ap_info.ssid));
    tft.setCursor(8,yCoursor());
    tft.print("PSK: " + wifiPSK); // TODO: save upon connection and then print here
    tft.setCursor(8,yCoursor());
    tft.print("IP: " + WiFi.gatewayIP().toString()); // some AP might not have assigned ip (but we ignore that case)

    tft.setCursor(8,yCoursor());
    tft.print("Channels: " + String(ap_info.primary) + " " + getChannelWidth(ap_info.second) );

    tft.setCursor(8,yCoursor());
    tft.print("BSSID: " +  mac); // sometimes MAC != BSSID (but we ignore that case)
    tft.setCursor(8,yCoursor());
    tft.print("Manufacturer: " +  getManufacturer(mac)); // sometimes MAC != BSSID (but we ignore that case)

    tft.setCursor(8,yCoursor());
    tft.print("Signal strength: " + String(ap_info.rssi) + "db");

    tft.setCursor(8,yCoursor());
    tft.print("Auth mode: " + autoMode2String(ap_info.authmode));

    tft.setCursor(8,yCoursor());
    tft.print("Unitcast cypher: " + cypherType2String(ap_info.pairwise_cipher));

    tft.setCursor(8,yCoursor());
    tft.print("Multicast cypher: " + cypherType2String(ap_info.group_cipher));

    tft.setCursor(8,yCoursor());
    tft.print("Antenna: " + String(ap_info.ant));

    tft.setCursor(8,yCoursor());
    tft.print("Modes: " + phyModes2String(ap_info));

    while(checkSelPress()) yield();
    while(!checkSelPress()) yield();
}

// what
// ssid
// rssi
// encryption
// channel
// local ip
// outside ipv4/6
// protocol WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_11AX
// bandwidth
// speed both sides
// country info

// void displayWiFiInfo() {
//   // Get basic connection info
//   String ssid = WiFi.SSID();           // SSID of the connected network
//   int32_t rssi = WiFi.RSSI();          // Signal strength in dBm
//   uint8_t encryptionType = WiFi.encryptionType(); // Encryption type of the network
//   String bssid = WiFi.BSSIDstr();      // BSSID (MAC address) of the AP
//   uint8_t channel = WiFi.channel();    // WiFi channel
//   wl_status_t status = WiFi.status();  // Connection status

//   // Print WiFi information
//   Serial.println("===== WiFi Information =====");
//   Serial.println("SSID: " + ssid);
//   Serial.println("Signal Strength (RSSI): " + String(rssi) + " dBm");
//   Serial.println("BSSID: " + bssid);
//   Serial.println("Channel: " + String(channel));
//   Serial.println("Encryption Type: " + String(encryptionType));
//   Serial.println("Connection Status: " + String(status));
//   Serial.println("================================");

//   // Frequency information (for dual-band routers)
//   int32_t freq = WiFi.scanNetworks();  // Scan to get frequency data
//   if (channel > 14) {
//     Serial.println("Frequency: 5 GHz");
//   } else {
//     Serial.println("Frequency: 2.4 GHz");
//   }
// }