#include "display.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <Timezone.h>

bool setupAP();

void disconnectWifi();

bool wifiConnectMenu( bool isAP = false);

void checkMAC();
