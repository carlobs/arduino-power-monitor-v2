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




![IMG2](https://user-images.githubusercontent.com/9199000/236864406-ace13526-b83b-4fe0-92bf-77c72a49bcbe.jpg)

![IMG1](https://user-images.githubusercontent.com/9199000/236864378-3041daca-a9fd-4fc4-a5c6-dbc937ce76ba.jpg)

