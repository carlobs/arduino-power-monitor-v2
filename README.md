# arduino-power-monitor-v2
Author: CARLO BENDINELLI

Arduino NodeMCU ESP8266 as advanced power monitor display for a Shelly EM device:

- OLED 1.3 display for visual informations:
  - Real time photovoltaic power production + orizzontal bar
  - Real Time power fed into or drawn from the electricity grid + orizzontal bar
  - Overpower alert
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
  - the "Power Monitor V2" is inspired from PowerMonitor of Maurizio Giunti https://github.com/giuntim/arduino-power-monitor#readme
  - i realized a portable case, with powerbank battery inside (more then a week of autonomy): TP-LINK https://www.tp-link.com/it/home-networking/mobile-accessory/tl-pb10400/
  - i edited the 3D Printer SCAD files, setting 80mm diameter option for the battery dimensions. https://www.thingiverse.com/thing:3391397
  - added an external microUSB port to charge the powerbank
  - added an external swith to power on/off the device


- ShellyEM https://www.shellyitalia.com/shelly-em/

![immagine](https://user-images.githubusercontent.com/9199000/236873996-9918059e-24a6-498a-800f-9f3bb6c7ea32.png)

- Schematic:
![Schema](https://user-images.githubusercontent.com/9199000/236876619-39e3269e-0f74-430c-937d-3ae92f49ee54.jpg)

- 3D Case:

![immagine](https://user-images.githubusercontent.com/9199000/236880278-6ec6c8a8-54e0-449e-a8d7-ee09c16278f6.png)


- Configuration WebPage:

![immagine](https://user-images.githubusercontent.com/9199000/236879902-2f3c21aa-d107-4ac8-a173-78e364217e18.png)

