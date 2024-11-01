#include "globals.h"
#include "wifi_common.h"
#include "mykeyboard.h"   // usinf keyboard when calling rename
#include "display.h"      // using displayRedStripe  and loop options
#include "settings.h"
#include "eeprom.h"
#include "powerSave.h"

/***************************************************************************************
** Function name: wifiConnect
** Description:   Connects to wifiNetwork
***************************************************************************************/
void updateTime(){
  timeClient.begin();
  timeClient.update();
  int tmz = read_eeprom(EEPROM_TMZ); 
  int timeOffset = 3600;

  switch(tmz){
    default:
    case 0: timeOffset *= -3; break;
    case 1: timeOffset *= -2; break;
    case 2: timeOffset *= -4; break;
    case 3: timeOffset *= 1; break;
    case 4: timeOffset *= 8; break;
    case 5: timeOffset *= 10; break;
    case 6: timeOffset *= 9; break;
    case 7: timeOffset *= 3; break;
    case 8: timeOffset *= 2; break;
  }

  timeClient.setTimeOffset(timeOffset);
  localTime = myTZ.toLocal(timeClient.getEpochTime());
  #if !defined(HAS_RTC)
    rtc.setTime(timeClient.getEpochTime());
    updateTimeStr(rtc.getTimeStruct());
    clock_set=true;
  #endif
}

String findPskInConfigs(const String& ssid, const JsonArray& config ){
  for (const JsonObject& wifiEntry : config) {
    const String name = wifiEntry["ssid"].as<String>();
    const String pass = wifiEntry["pwd"].as<String>();
    log_i("SSID: %s, Pass: %s", name, pass);
    if (name == ssid) {
      log_i("Found SSID: %s", name);
      return pass;
    }
  }
  
  return {};
}

bool connectWifi(const String& ssid, int encryption){
    getConfigs();
    JsonArray wifiList = settings[0]["wifi"].as<JsonArray>();
    auto password = findPskInConfigs(ssid, wifiList);
    bool wrongPass = false;
    bool newSsid = password.isEmpty();

  Retry:
    if (password.isEmpty() || wrongPass) {
      delay(200);
      if (encryption) 
        password = keyboard(password, 63, "Network Password:");
    }

    drawMainBorder();
    tft.setCursor(7,27);
    tft.print("Connecting to: " + ssid + ".");

    WiFi.begin(ssid, password);

    uint8_t retry;
    constexpr uint8_t MAX_RETRIES = 20;
    while (WiFi.status() != WL_CONNECTED) {
      if( tft.getCursorY() != 27 && !tft.getCursorX() ) tft.setCursor(7, tft.getCursorY());
      tft.print(".");
      if(retry++ > MAX_RETRIES) {
        displayError("Wifi Offline");
        delay(500);
        break;
      }
      delay(500);
    }

    if(WiFi.status() == WL_CONNECTED) {
      // update globals
      wifiConnected = true;
      wifiPSK = password;
      wifiIP = WiFi.localIP().toString();

      ::updateTime();

      // update saved password
      if (newSsid) {
        JsonObject newWifi = wifiList.add<JsonObject>();
        newWifi["ssid"] = ssid;
        newWifi["pwd"] = password;
        saveConfigs();
      } else if (sdcardMounted && wrongPass) {
        for (JsonObject wifiEntry : wifiList) {
          if (wifiEntry["ssid"].as<String>() == ssid) {
            wifiEntry["pwd"] = password;
            log_i("Mudou pwd de SSID: %s", ssid);
            break;
          }
        }
        saveConfigs();
      }


      return true;
    } else {
      bool ret = false;
      wrongPass = true;

      wakeUpScreen();

      options = {
        {"Retry", [&]() { ret=true; }},
        {"Cancel", [=]() { backToMenu(); }}
      };
      loopOptions(options);

      delay(200);

      if(ret) {
        goto Retry;
      } else {
        disconnectWifi();
        return false;
      }
    }
}

bool setupAP(){
  IPAddress AP_GATEWAY(172, 0, 0, 1);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_GATEWAY, AP_GATEWAY, IPAddress(255, 255, 255, 0));

  getConfigs();

  WiFi.softAP(ap_ssid, ap_pwd, 6,0,4,false);
  wifiIP = WiFi.softAPIP().toString(); // update global var

  Serial.print("IP: "); Serial.println(wifiIP);

  wifiConnected=true;
  return true;
}

/***************************************************************************************
** Function name: wifiDisconnect
** Description:   disconnects and turn off the WIFI module
***************************************************************************************/
void disconnectWifi() {
  WiFi.softAPdisconnect(true); // turn off AP mode
  WiFi.disconnect(true,true);  // turn off STA mode
  WiFi.mode(WIFI_OFF);         // enforces WIFI_OFF mode

  wifiConnected=false;

  wifiPSK.clear();
  returnToMenu=true;
}

/***************************************************************************************
** Function name: wifiConnectMenu
** Description:   Opens a menu to connect to a wifi
***************************************************************************************/
bool wifiConnectMenu(bool isAP) {
  if(isAP) {
    setupAP();
  }
  else if (WiFi.status() != WL_CONNECTED) {
    options.clear();

    int nets;
    WiFi.mode(WIFI_MODE_STA);
    displayRedStripe("Scanning..",TFT_WHITE,FGCOLOR);
    nets = WiFi.scanNetworks();
    for(int i = 0; i < nets; ++i){
      options.emplace_back(WiFi.SSID(i).c_str(), [=]() { connectWifi(WiFi.SSID(i), WiFi.encryptionType(i)); });
    }
    options.emplace_back("Main Menu", [=]() { backToMenu(); });
    delay(200);
    loopOptions(options);
    delay(200);
  }

  if (returnToMenu) return false;
  return wifiConnected;
}

void checkMAC() {
  drawMainBorderWithTitle("MAC ADDRESS");
  padprintln("\n");
  padprintln(WiFi.macAddress());

  delay(200);
  while(!checkAnyKeyPress()) { delay(80); }
}
