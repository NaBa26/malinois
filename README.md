# malinois
This firmware puts the ESP32 into Wi-Fi **promiscuous mode** and logs every received 802.11 packet with a timestamp from a **DS3231 RTC**.

## What It Does
- Captures nearby Wi-Fi packets (beacons, probe requests, probe responses, data, etc.).
- Retrieves the current time from the DS3231 over I²C.
- Prints logs as provided below.
<img width="400" height="300" alt="image" src="https://github.com/user-attachments/assets/df52259c-d934-4f65-a562-f9b71fee9e6c" />

## Requirements
- ESP32 dev board  
- DS3231 RTC module  
- ESP-IDF 5.x
- Working USB cable

<img width="720" height="480" alt="image" src="https://github.com/user-attachments/assets/dc3b3a3f-7315-402a-8ef0-731604c6da6f" />

## Build & Flash
- idf.py set-target esp32
- idf.py build
- idf.py flash
- idf.py monitor

## Directory Structure
<img width="250" height="400" alt="image" src="https://github.com/user-attachments/assets/3fcd08b6-57fe-4653-8e77-20b3f8304e6d" />

## Future Work
- Channel hopping  
- Dump frames into PCAP format  
- SD card logging  
- Add packet filters  
- Add watchdog for dropped packets  
- Add CLI (via UART) for channel/config commands  

## ⚠ Notes
- ESP32 cannot decrypt encrypted Wi-Fi payloads.  
- Heavy printing causes packet drops.  
- Busy environments = increase buffer size.  
- DS3231 time must be set once after wiring.  
- Use filtering later if you want only beacons/probes/data.





