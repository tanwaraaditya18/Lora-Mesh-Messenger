# LoRa Mesh Messenger

An offline, ESP32-based messaging system that uses LoRa radio communication to exchange messages between devices without cellular networks or internet connectivity.

The project creates a local Wi-Fi access point on each ESP32, allowing users to access a web-based messaging interface, send private messages, broadcast messages, and transmit SOS alerts. LoRa enables communication between devices, including multi-hop message forwarding through intermediate nodes.

## 🚀 Features

- **Offline Communication:** Send messages without internet or cellular connectivity.
- **Private Messaging:** Send messages to a specific device using its Node ID.
- **Group Broadcasting:** Send messages to all reachable nodes.
- **Multi-Hop Communication:** Intermediate devices can forward messages to extend communication range.
- **Duplicate Packet Detection:** Prevents repeated forwarding of the same packet.
- **Hop Limit:** Supports forwarding up to a maximum hop count of 3.
- **Web-Based Interface:** Access the messaging interface through a local Wi-Fi connection.
- **Message History:** Maintains a recent message history.
- **SOS Alert:** Send emergency alerts using the web interface or a hardware push button.

## 🛠️ Hardware Requirements

| Component | Purpose |
|---|---|
| ESP32 Development Board × 3 | Main processing and Wi-Fi |
| SX1278 / RA-02 LoRa Module × 3 | Wireless communication |
| Push Button | Hardware SOS trigger |
| Green LED | Status indication |
| Red LED | SOS indication |
| Antennas | LoRa transmission and reception |
| Power Supply | Powering the devices |

## 🔌 Circuit Connections

The current firmware uses the following ESP32 GPIO configuration:

| LoRa Module | ESP32 GPIO |
|---|---|
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| NSS / SS | GPIO 5 |
| RESET | GPIO 14 |
| DIO0 | GPIO 26 |

Additional connections:

| Component | ESP32 GPIO |
|---|---|
| SOS Push Button | GPIO 4 |
| Green LED | GPIO 12 |
| Red SOS LED | GPIO 13 |

**Important:** Verify the connections with your actual hardware before powering the circuit. The RA-02 module operates at 3.3 V, not 5 V. Connect a suitable antenna before transmitting.

## 💻 Software Requirements

- Arduino IDE
- ESP32 Board Support Package
- LoRa Library by Sandeep Mistry

The firmware also uses the ESP32 libraries:

- WiFi.h
- WebServer.h

Install the LoRa library through Arduino IDE Library Manager by searching for `LoRa`.

## ⚙️ Installation and Configuration

This project uses **one universal Arduino sketch** for all three ESP32 devices.

The only required configuration change is the Node ID.

### Step 1: Open the Firmware

Open `LoRa_Mesh_Messenger.ino` in Arduino IDE.

### Step 2: Configure the Node ID

Find the following line near the beginning of the code:

```cpp
const int NODE_ID = 1;
```

Configure each ESP32 as follows:

| Device | NODE_ID |
|---|---|
| ESP32 Device 1 | 1 |
| ESP32 Device 2 | 2 |
| ESP32 Device 3 | 3 |

Upload the same firmware to all three devices, changing the Node ID before each upload.

### Step 3: Upload the Code

1. Connect the ESP32 to your computer.
2. Select the correct ESP32 board in Arduino IDE.
3. Select the appropriate COM port.
4. Compile and upload the firmware.
5. Repeat the process for the other two devices with their respective Node IDs.

All devices must use compatible LoRa radio settings.

## 📡 Connecting to the Devices

Each ESP32 creates its own Wi-Fi access point.

| Device | Wi-Fi SSID |
|---|---|
| Node 1 | NODE_CONSOLE_1 |
| Node 2 | NODE_CONSOLE_2 |
| Node 3 | NODE_CONSOLE_3 |

**Default Wi-Fi Password:**

`12345678`

Connect your phone or computer to the desired device's Wi-Fi network and open the following address in a browser:

`http://192.168.4.1`

The messaging interface will load locally without requiring an internet connection.

Change the default Wi-Fi password before using the devices outside a controlled demonstration.

## 🔄 How Multi-Hop Communication Works

The system uses a basic LoRa mesh-style forwarding mechanism.

For example, consider three devices:

**Node 1 → Node 2 → Node 3**

Suppose Node 1 wants to send a message to Node 3, but Node 3 is outside the direct communication range of Node 1.

1. Node 1 transmits the message with Node 3 as the destination.
2. Node 2 receives the packet.
3. Node 2 checks the sender ID, destination ID, and packet ID.
4. Since Node 2 is not the destination, it forwards the packet.
5. The hop count is incremented.
6. Node 3 receives the forwarded packet and stores the message in its inbox.

Each packet contains:

- Original Sender ID
- Final Recipient ID
- Packet ID
- Hop Count
- Payload Length
- Message Payload

The firmware uses duplicate-packet detection and a maximum hop count of 3 to limit unnecessary retransmissions.

**Note:** This implementation uses flooding-style relaying rather than intelligent route discovery. Intermediate devices must be within communication range of the previous and next devices.

## 🆘 SOS Emergency Feature

The system supports SOS alerts through:

- Web-based SOS button
- Hardware push button connected to GPIO 4
- Red LED indication

**GPS limitation:** The current firmware uses hardcoded example coordinates. Live GPS tracking is not implemented in this version.

## ⚠️ Important Limitations

- Communication range depends on antenna quality, obstacles, environment, and LoRa configuration.
- The system does not guarantee message delivery.
- The current implementation does not perform intelligent route discovery.
- SOS coordinates are hardcoded, not live GPS coordinates.
- Packet collisions and radio interference may affect communication.
- The three-device setup requires real hardware testing.

## 🔧 Troubleshooting

| Problem | Possible Solution |
|---|---|
| LoRa initialization failed | Check power supply, SPI connections, and pin configuration. |
| Messages not received | Verify frequency and LoRa radio settings on all devices. |
| Webpage not opening | Connect to the correct ESP32 Wi-Fi network and visit `192.168.4.1`. |
| ESP32 upload failed | Check the COM port, USB cable, and board selection. |
| ESP32 randomly resets | Check power stability and wiring. |

## 📂 Repository Structure

```text
LoRa-Mesh-Messenger/
│
├── README.md
├── .gitignore
│
├── firmware/
│   └── LoRa_Mesh_Messenger.ino
│
├── WIRING.md
├── TESTING.md
│
├── images/
│   ├── Messenger_UI.jpg
│   └── Messenger_Hardware.jpg
│
└── LICENSE
```

## 🎯 Project Objective

The objective of this project is to develop a portable emergency communication system capable of exchanging messages in remote areas where conventional cellular networks and internet connectivity are unavailable.

By combining ESP32, LoRa wireless communication, and multi-hop forwarding, the system explores an alternative approach to decentralized communication.

## 👨‍💻 Author

Electronics and Communication Engineering Project

Built using ESP32, SX1278 LoRa modules, Arduino IDE, and embedded wireless communication technologies.
