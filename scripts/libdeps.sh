#!/bin/bash

# Declare an array with Git URLs
arr=(
  "https://github.com/Makuna/Rtc"
  "https://github.com/bblanchon/ArduinoJson.git"
  "https://github.com/a7md0/WakeOnLan.git"
  "https://github.com/arduino-libraries/Ethernet.git"
  "https://github.com/Arduino-IRremote/Arduino-IRremote.git"
)

# Check if the 'lib' directory exists, and if not, create it
if [ ! -d "../lib" ]; then
  echo "Creating ./lib directory"
  mkdir ./lib
fi

# Change to the 'lib' directory
pushd ../lib || { echo "Failed to change directory to ../lib"; exit 1; }

# Loop through each URL in the array and clone the repositories
for i in "${arr[@]}"
do 
  echo "Cloning $i"
  git clone "$i" || { echo "Failed to clone $i"; exit 1; }
done

# Return to the original directory
popd || { echo "Failed to return to the previous directory"; exit 1; }

echo "Finished installing dependencies"
exit 0

