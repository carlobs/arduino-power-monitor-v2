# arduino-power-monitor-v2
Author: CARLO BENDINELLI

Arduino NodeMCU ESP8266 as advanced power monitor display for a Shelly EM device:

- OLED 1.3 display for visual informations:
  - Real time photovoltaic power production + orizzontal bar
  - Real Time power fed into or drawn from the electricity grid + orizzontal bar
  - WiFi LAN IP to reach the Configuration Web Page
  - Access Point IP (when WiFi access fails) to reach the Configuration Web Page

- WEB SERVER:
  - setup up to two Wifi
  - setup up to two Shelly EM device (for example, LAN and WAN address)
  - Setup the Max power for photovoltaic system
  - Setup the Max power available form the grid
  - Setup the Levels to activate the RGB LED indicator
  - Remote reboot
  - Reset default

- RGB LED light to see real time status, amount of power exchanged with the grid, full customizable reference level through Web Page:
  - Flashing Green -> Very high available power
  - Green -> High available power
  - Azure -> Mid available power
  - Blue -> Low available power
  - Pink -> Low drawn power from grid
  - Red -> High drawn power from grid
  - Flashing red -> Very high drawn power from grid

- Acoustic ALARM:
  - more withdrawals from the grid than allowed (risk of disconnection of the grid)
  - correct starting after power on
  - error wifi connections, Access Point startup

- NOTE:
  - i realized a portable case, with powerbank battery inside TP-LINK https://www.tp-link.com/it/home-networking/mobile-accessory/tl-pb10400/
  - so i edited the 3D Printer file SCAM, with 80mm diameter https://www.thingiverse.com/thing:3391397
  - 


- ShellyEM https://www.shellyitalia.com/shelly-em/
![immagine](https://user-images.githubusercontent.com/9199000/236873996-9918059e-24a6-498a-800f-9f3bb6c7ea32.png)

- Schematic
![Schema](https://user-images.githubusercontent.com/9199000/236876619-39e3269e-0f74-430c-937d-3ae92f49ee54.jpg)

- 3D Case
![IMG2](https://user-images.githubusercontent.com/9199000/236864406-ace13526-b83b-4fe0-92bf-77c72a49bcbe.jpg)

![IMG1](https://user-images.githubusercontent.com/9199000/236864378-3041daca-a9fd-4fc4-a5c6-dbc937ce76ba.jpg)

