LoRa Mesh Messenger — ESP32 + SX1278
An offline messaging prototype using three ESP32 boards and SX1278/RA-02 LoRa radios. Each ESP32 creates its own Wi-Fi access point and hosts a local web messenger. Messages are sent between devices over LoRa; intermediate nodes can relay packets for multi-hop communication.
> **Project status:** This repository packages the firmware supplied for the project. It has not been independently hardware-tested as part of this packaging. Test with your actual boards and wiring before relying on it.
Features in this firmware
Local web interface hosted by each ESP32 (no internet router required).
Private messages addressed to a node ID.
Group/broadcast messages using destination ID `0`.
Basic flooding-style multi-hop forwarding, with a maximum forwarding hop count of 3.
Duplicate-packet tracking to reduce repeated relaying.
SOS broadcast from the web interface and a physical button.
Green message LED and red SOS LED.
Recent message history held in device memory.
Repository layout
```text
LoRa_Mesh_Messenger/
├── README.md
├── WIRING.md
├── TESTING.md
├── LICENSE
├── .gitignore
└── firmware/
    ├── node_1/node_1.ino
    ├── node_2/node_2.ino
    └── node_3/node_3.ino
```
Each folder contains a separate Arduino sketch. Upload the matching sketch to the corresponding ESP32. The three sketches are otherwise the same; their `NODE_ID` values are 1, 2, and 3.
Hardware used / expected
3 × ESP32 development boards
3 × SX1278 / RA-02 LoRa modules (matching frequency variant)
3 × suitable LoRa antennas
SOS push button per board (optional)
Green LED and red LED per board (optional), with suitable current-limiting resistors
Stable regulated 3.3 V supply for each LoRa module
Arduino IDE setup
Install the ESP32 board support package in Arduino IDE.
Install the LoRa library by Sandeep Mistry (`LoRa`).
Open only one sketch at a time, for example `firmware/node_1/node_1.ino`.
Select the correct ESP32 board and COM/serial port.
Verify the pinout in `WIRING.md` against your physical wiring before uploading.
Upload `node_1.ino` to the first board, `node_2.ino` to the second, and `node_3.ino` to the third.
Open Serial Monitor at 115200 baud.
Connect to each device
After boot, each board creates a Wi-Fi access point:
Device	Wi-Fi name	Password
Node 1	`NODE_CONSOLE_1`	`12345678`
Node 2	`NODE_CONSOLE_2`	`12345678`
Node 3	`NODE_CONSOLE_3`	`12345678`
Connect your phone or laptop to the relevant network, then open `http://192.168.4.1` in a browser. You must switch Wi-Fi networks to access another node's local interface.
Important configuration and safety notes
Check DIO0 wiring: this firmware uses GPIO 26 for LoRa DIO0. Earlier project wiring discussions mentioned GPIO 2/D2. Do not upload until the actual module wiring and this setting agree.
Check the radio frequency: this firmware is configured for `433E6`. All nodes must use compatible radios and identical radio settings. Confirm the permitted frequency, power, antenna, and duty-cycle requirements for your location before transmitting. Do not assume that a frequency is licence-exempt or permitted just because the module supports it.
Power: the RA-02/SX1278 is a 3.3 V device. Do not connect its supply pin to 5 V. Use a stable supply capable of handling radio transmit-current peaks and common ground with the ESP32.
SOS coordinates are placeholders: the firmware embeds `12.9239 N, 77.4997 E`; it does not read a GPS module. Replace these values or integrate and validate GPS before treating SOS coordinates as real.
No guaranteed delivery or encryption: this prototype does not implement acknowledgements, guaranteed delivery, authentication, or message encryption. Do not use it as a life-safety communication system.
The firmware's LoRa settings include TX power 20, spreading factor 9, bandwidth 125 kHz, coding rate 4/8, and CRC. Check radio/module and local regulatory limits before use.
Multi-hop overview
A sender puts the original sender ID, destination ID, packet ID, hop count, payload length, and payload into a LoRa packet. A node that receives a packet checks whether it has already seen that sender/packet-ID pair. If the packet is for another node, it can retransmit it with the hop count incremented, provided the hop count is below 3. A packet addressed to the receiving node is stored instead of forwarded. Destination `0` is treated as broadcast.
This is a basic flooding-style relay, not route discovery or shortest-path routing. Every relay must be within radio range of the previous and next hop.
Suggested first test
Put Node 1 and Node 2 close together and verify direct messaging.
Verify Node 1 to Node 3 directly if they are in range.
Then place Node 2 between Node 1 and Node 3 so Node 1 and Node 3 cannot communicate directly, while both can reach Node 2.
Send a private message from Node 1 to Node 3 and check whether Node 2 relays it.
Test broadcast and SOS separately. Keep a normal backup communication method available.
See WIRING.md and TESTING.md before field testing.
