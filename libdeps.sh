#!/bin/bash

# Declare an array with Git URLs
arr=(
  "https://github.com/Makuna/Rtc"
  "https://github.com/bblanchon/ArduinoJson.git"
  "https://github.com/a7md0/WakeOnLan.git"
  "https://github.com/arduino-libraries/Ethernet.git"
  "https://github.com/Arduino-IRremote/Arduino-IRremote.git"
)

# Loop through each URL in the array and clone the repositories
for i in "${arr[@]}"
do 
  echo "Cloning $i"
  # git clone "$i"
done

