#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// 1. CHOOSE NODE ID FOR THIS BOARD (1, 2, or 3)
// ==========================================
const int NODE_ID = 1; 

// ==========================================
// 2. HARDWARE PINOUT CONFIGURATION
// ==========================================
const long LORA_FREQUENCY = 433E6;
const int SCK_PIN  = 18;
const int MISO_PIN = 19;
const int MOSI_PIN = 23;
const int SS_PIN   = 5;
const int RST_PIN  = 14;
const int DIO0_PIN = 26;

// Hardware Input / Output Pins
const int HARDWARE_SOS_PIN = 4; // Wire between Pin 4 and GND
const int GREEN_LED_PIN    = 12; // New Message Indicator
const int RED_LED_PIN      = 13; // SOS Alert Indicator

// ==========================================
// 3. STORAGE & SEQUENCE TRACKING ENGINE
// ==========================================
WebServer server(80);

struct Message {
  int id;      // Unique message index identifier for unread counting logic
  int dir;     // 1 = Sent, 0 = Received
  int room;    // 0 = Squad Group, 1/2/3... = Private Room Node ID
  int sender;  // The actual ORIGINAL creator of the text
  String text;
};

const int MAX_MESSAGES = 40;
Message msgHistory[MAX_MESSAGES];
int msgCount = 0;
int nextMsgId = 1; // Auto-incrementing message counter sequence

bool txPending = false;
int txTarget = 0;
String txPayload = "";

void addMessage(int dir, int room, int sender, String text) {
  if (msgCount >= MAX_MESSAGES) {
    for (int i = 1; i < MAX_MESSAGES; i++) msgHistory[i - 1] = msgHistory[i];
    msgHistory[MAX_MESSAGES - 1] = {nextMsgId++, dir, room, sender, text};
  } else {
    msgHistory[msgCount++] = {nextMsgId++, dir, room, sender, text};
  }
}

String sanitizeText(String str) {
  str.replace("\\", "\\\\");
  str.replace("\"", "\\\"");
  return str;
}

// ==========================================
// 4. MESH PACKET TRACKING (Loop Prevention)
// ==========================================
uint16_t packetCounter = 0;

struct SeenPacket {
  int sender;
  uint16_t id;
};

SeenPacket seenPackets[40];
int seenCount = 0;

bool alreadySeen(int sender, uint16_t id) {
  for (int i = 0; i < seenCount; i++) {
    if (seenPackets[i].sender == sender && seenPackets[i].id == id)
      return true;
  }
  if (seenCount < 40) {
    seenPackets[seenCount++] = {sender, id};
  } else {
    for (int i = 1; i < 40; i++) seenPackets[i - 1] = seenPackets[i];
    seenPackets[39] = {sender, id};
  }
  return false;
}

// ** Non-Blocking LED Hardware State Timers **
unsigned long greenLedTurnOffTime = 0;
unsigned long redLedSequenceEndTime = 0;
unsigned long redLedLastToggleTime  = 0;
bool currentRedHardwareState       = false;

// ==========================================
// 5. MESH APP UI (Tactical Aesthetic + Gesture Management)
// ==========================================
const char MAIN_page[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Mesh Messenger</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Nunito:wght@400;600;700;800&display=swap');
    
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; font-family: 'Nunito', sans-serif; }
    
    body { 
      margin: 0; 
      background: linear-gradient(135deg, #d8f0e1 0%, #b8dfc7 100%); 
      color: #1a3b2b; 
      overflow-x: hidden; 
      min-height: 100vh;
      user-select: none; /* Disable standard context menus for better long-press */
    }
    
    /* Layout Views */
    #listView, #chatView { position: absolute; width: 100%; height: 100vh; transition: transform 0.4s cubic-bezier(0.25, 0.8, 0.25, 1); }
    #chatView { transform: translateX(100%); z-index: 10; display: flex; flex-direction: column; background: #eef7f1; }
    body.in-chat #chatView { transform: translateX(0); }
    body.in-chat #listView { transform: translateX(-30%); opacity: 0.5; }

    /* Top Identity Bar */
    .identity-bar {
      background: rgba(255, 255, 255, 0.85); backdrop-filter: blur(10px);
      margin: 15px; padding: 12px 20px; border-radius: 30px; display: flex;
      align-items: center; justify-content: space-between;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.05);
    }
    .identity-title { font-weight: 800; font-size: 20px; color: #072a1e; }
    .identity-subtitle { font-size: 12px; color: #6b8a7a; margin-top: -2px; }

    /* Add Device Button */
    .add-device-btn {
      background: rgba(47, 97, 70, 0.08); border: 1px solid rgba(47, 97, 70, 0.2);
      color: #2f6146; padding: 6px 14px; border-radius: 20px;
      font-weight: 800; font-size: 12px; cursor: pointer; transition: all 0.2s ease;
    }
    .add-device-btn:active { transform: scale(0.95); background: rgba(47, 97, 70, 0.15); }

    /* List View Cards */
    #contactsContainer { padding: 0 15px; display: flex; flex-direction: column; gap: 12px; padding-bottom: 20px; }
    
    .contact-item { 
      background: rgba(255, 255, 255, 0.95); padding: 15px; border-radius: 24px; 
      display: flex; align-items: center; cursor: pointer; 
      box-shadow: 0 4px 12px rgba(0,0,0,0.04);
      transition: all 0.2s ease;
      position: relative; overflow: hidden; /* For Delete Slide-in */
    }
    /* Long press visual feedback */
    .contact-item:active { background: #eef7f1; }

    /* Avatars */
    .avatar { 
      width: 50px; height: 50px; background: #7ca384; border-radius: 16px; 
      color: #fff; display: flex; align-items: center; justify-content: center; 
      font-weight: 800; margin-right: 15px; font-size: 18px; 
    }
    
    .contact-info { flex: 1; margin-right: 10px; transition: opacity 0.2s; }
    .contact-name { font-weight: 800; font-size: 17px; color: #0c2b1e;}
    .contact-status { font-size: 13px; color: #5a7a68; margin-top: 2px; display: flex; align-items: center;}

    /* WhatsApp Style Unread Counter Notification Badges */
    .unread-badge {
      background: #2f6146; color: #ffffff; border-radius: 50%;
      min-width: 22px; height: 22px; padding: 0 6px; font-size: 11px;
      font-weight: 800; display: flex; align-items: center; justify-content: center;
      box-shadow: 0 2px 5px rgba(47, 97, 70, 0.3); transition: opacity 0.2s;
    }

    /* ** SLIDE-IN DELETE UI ** */
    .delete-reveal-overlay {
      position: absolute; right: 0; top: 0; bottom: 0;
      width: 0; /* Hidden state */
      background: #ff3b30; border-radius: 24px;
      display: flex; align-items: center; justify-content: center;
      overflow: hidden; z-index: 5;
      transition: width 0.3s cubic-bezier(0.25, 0.8, 0.25, 1);
    }
    
    /* When editing-mode class is active (after long press) */
    .contact-item.editing-mode .delete-reveal-overlay { width: 40%; /* Slide-in 40% of card width */ }
    .contact-item.editing-mode .contact-info, .contact-item.editing-mode .unread-badge { opacity: 0.15; /* Blur main info */ }

    .delete-reveal-btn { color: white; font-weight: 800; font-size: 13px; cursor: pointer; text-transform: uppercase; white-space: nowrap;}
    .delete-reveal-btn:active { color: #ffebee; }

    /* Chat Header Styling */
    .chat-header { 
      background: #fff; padding: 15px 20px; box-shadow: 0 2px 10px rgba(0,0,0,0.05); 
      display: flex; align-items: center; justify-content: space-between;
    }
    .chat-header-left { display: flex; align-items: center; }
    
    /* Sleek Icon Arrow Back Button */
    .back-btn { 
      color: #2f6146; cursor: pointer; margin-right: 18px; font-size: 32px; 
      font-weight: 400; line-height: 1; user-select: none;
    }
    .chat-title-area { font-weight: 800; font-size: 18px; color: #0c2b1e; display: flex; align-items: center; gap: 2px; }
    
    /* Sleek Icon Pencil Edit Button */
    .rename-icon { 
      color: #7ca384; cursor: pointer; font-size: 16px; 
      padding: 2px 6px; margin-left: 4px; user-select: none;
    }
    .rename-icon:active { color: #2f6146; }

    /* Top-Right SOS Button */
    .sos-header-btn {
      background: linear-gradient(135deg, #ff5e3a 0%, #ff2a68 100%);
      color: white; border: none; padding: 8px 16px; border-radius: 16px;
      font-weight: 800; font-size: 13px; cursor: pointer;
      box-shadow: 0 4px 10px rgba(255, 42, 104, 0.25); transition: transform 0.1s;
    }
    .sos-header-btn:active { transform: scale(0.92); }
    
    /* Messaging Stream Styling */
    .message-list { flex: 1; overflow-y: auto; padding: 20px 15px; display: flex; flex-direction: column; gap: 8px;}
    .bubble { max-width: 75%; padding: 12px 16px; border-radius: 20px; font-size: 15px; line-height: 1.4; position: relative; box-shadow: 0 2px 5px rgba(0,0,0,0.03);}
    .sent { background: #2f6146; color: #fff; align-self: flex-end; border-bottom-right-radius: 4px; }
    .rcvd { background: #fff; color: #0c2b1e; align-self: flex-start; border-bottom-left-radius: 4px; border: 1px solid #e1ebe5; }
    
    .sos-bubble { background: linear-gradient(135deg, #ff5e3a, #ff2a68) !important; color: #fff !important; font-weight: bold; border: none; animation: pulse 1.5s infinite; cursor: pointer; }
    @keyframes pulse { 0% { box-shadow: 0 0 0 0 rgba(255,42,104,0.4); } 70% { box-shadow: 0 0 0 10px rgba(255,42,104,0); } 100% { box-shadow: 0 0 0 0 rgba(255,42,104,0); } }

    .sender-label { display: block; font-weight: 800; font-size: 11px; color: #7ca384; margin-bottom: 4px; text-transform: uppercase; }
    .sos-bubble .sender-label { color: #ffe5e5; }
    
    .map-card-link { background: rgba(255,255,255,0.2); text-align: center; padding: 6px; margin-top: 8px; border-radius: 10px; font-size: 12px; font-weight: 800; text-transform: uppercase; letter-spacing: 0.5px;}

    /* Input Controls */
    .input-bar { background: #fff; padding: 15px; border-top: 1px solid #e1ebe5; display: flex; gap: 10px; padding-bottom: max(15px, env(safe-area-inset-bottom));}
    #msgInput { flex: 1; border: 1px solid #d1ebd9; background: #f4fbf6; border-radius: 24px; padding: 12px 20px; outline: none; font-size: 15px; font-weight: 600; color: #0c2b1e;}
    #msgInput::placeholder { color: #9bb3a4; }
    
    .send-btn { background: #2f6146; color: #fff; border: none; border-radius: 50%; width: 45px; height: 45px; font-weight: bold; cursor: pointer; display: flex; align-items: center; justify-content: center; font-size: 20px; box-shadow: 0 4px 10px rgba(47, 97, 70, 0.2); transition: transform 0.1s;}
    .send-btn:active { transform: scale(0.9); }
  </style>
</head>
<body>

  <div id="listView">
    <div class="identity-bar">
      <div>
        <div class="identity-title" id="myTitle">Mesh Node</div>
        <div class="identity-subtitle">Secure Local Console</div>
      </div>
      <button class="add-device-btn" onclick="addNewDevice()">+ Add Device</button>
    </div>
    <div id="contactsContainer"></div>
  </div>

  <div id="chatView">
    <div class="chat-header">
      <div class="chat-header-left">
        <span class="back-btn" onclick="closeChat()">‹</span>
        <div class="chat-title-area">
          <span id="chatPartnerName">Chat</span>
          <span class="rename-icon" onclick="renameActiveDevice()">✎</span>
        </div>
      </div>
      <button class="sos-header-btn" onclick="triggerPersonalSOS()">SOS</button>
    </div>
    <div class="message-list" id="msgList"></div>
    <div class="input-bar">
      <input type="text" id="msgInput" placeholder="Type a message..." autocomplete="off" onkeypress="if(event.key==='Enter') doSend()">
      <button class="send-btn" onclick="doSend()">↑</button>
    </div>
  </div>

  <script>
    let myId = %NODE_ID%;
    let activePeer = null;
    let allMsgs = [];
    
    let maxSeenId = 0;       
    let unreadCounts = {};   

    // ** Gesture State Management Variables **
    let longPressTimer = null; // Stores setTimeout reference
    let longPressTriggered = false; // Flag if held long enough
    let isEditingNodeId = null; // Tracks which card has Delete revealed

    // Directory Storage Engine
    let defaultNodes = { 1: "device 1", 2: "device 2", 3: "device 3" };
    
    let nodeNames = JSON.parse(localStorage.getItem("mesh_directory"));
    if (!nodeNames) {
      nodeNames = defaultNodes;
      localStorage.setItem("mesh_directory", JSON.stringify(nodeNames));
    }

    function saveDirectory() {
      localStorage.setItem("mesh_directory", JSON.stringify(nodeNames));
    }

    function addNewDevice() {
      resetAllEdits(); // Close any revealed delete options
      let code = prompt("Enter Node Device ID Code (Numeric Integer):");
      if (!code) return;
      code = parseInt(code.trim());
      if (isNaN(code) || code <= 0) { alert("Invalid numerical code."); return; }
      
      let alias = prompt("Enter custom tactical name for Code " + code + ":");
      if (!alias || !alias.trim()) return;
      
      nodeNames[code] = alias.trim();
      saveDirectory();
      renderContacts();
    }

    function renameActiveDevice() {
      if (activePeer === null) return;
      if (activePeer === 0) { alert("The Squad Group broadcast room cannot be renamed."); return; }
      
      let currentName = nodeNames[activePeer] || "device " + activePeer;
      let newName = prompt("Rename tactical label for Code " + activePeer + ":", currentName);
      if (!newName || !newName.trim()) return;
      
      nodeNames[activePeer] = newName.trim();
      saveDirectory();
      document.getElementById('chatPartnerName').innerText = newName.trim();
      renderContacts();
    }

    // ** IMPLEMENTED GESTURE CONTROL LOGIC **

    // 1. Trigger when long-press timeframe completes
    function triggerLongPressHandler(id) {
        longPressTriggered = true;
        resetAllEdits(); // Clean other revealed cards
        isEditingNodeId = id;
        // Apply editing-mode class to reveal the red button via CSS
        document.getElementById(`card-${id}`).classList.add('editing-mode');
    }

    // 2. Clear Editing State (Slide-out red button)
    function resetAllEdits() {
        document.querySelectorAll('.contact-item').forEach(card => card.classList.remove('editing-mode'));
        isEditingNodeId = null;
    }

    // ** MODIFIED INTERACTION HANDLER **
    // Called when a card is released after being pressed
    function handleCardInteraction(id, event) {
        if (longPressTriggered) {
            // It was held long enough to trigger deletion, ignore this release
            event.preventDefault(); // Safety to drop accidental tap clicks
            return;
        }

        // It was a short release -> Detect intended action
        if (isEditingNodeId !== null) {
            // Some other node has Delete option revealed -> Reset them
            resetAllEdits();
        } else {
            // Regular tap -> open chat
            openChat(id);
        }
    }

    // The actual deletion action from the red button ( Confirmation prompt remains active)
    function deleteDeviceAction(id, event) {
      if(event && event.stopPropagation) event.stopPropagation(); // Stop parent release triggers
      
      const currentName = nodeNames[id] || "device " + id;
      if(!confirm("Permanently delete node [" + currentName + "] from directory?")) return;
      
      delete nodeNames[id];
      saveDirectory();
      isEditingNodeId = null; // State cleared
      renderContacts();
    }

    function renderContacts() {
      let myName = nodeNames[myId] || "device " + myId;
      document.getElementById('myTitle').innerText = myName;
      const container = document.getElementById('contactsContainer');
      
      let groupUnread = unreadCounts[0] || 0;
      let groupBadge = groupUnread > 0 ? `<div class="unread-badge">${groupUnread}</div>` : '';
      
      // Permanent Squad Group Header
      let html = `
        <div class="contact-item" onclick="openChat(0)">
          <div class="avatar" style="background: #568263;">SQ</div>
          <div class="contact-info">
            <div class="contact-name">Squad Group</div>
            <div class="contact-status">Group Broadcast</div>
          </div>
          ${groupBadge}
        </div>`;
        
      // Build individual contact cards with integrated gesture handlers
      Object.keys(nodeNames).forEach(key => {
        let intKey = parseInt(key);
        if(intKey === myId) return;
        let name = nodeNames[key];
        let initial = name.charAt(0).toUpperCase();
        
        let peerUnread = unreadCounts[intKey] || 0;
        let peerBadge = peerUnread > 0 ? `<div class="unread-badge">${peerUnread}</div>` : '';
        
        // Revised dynamic HTML including Slide-in overlay for Deletion
        html += `
          <div class="contact-item" id="card-${intKey}"
             onmousedown="longPressTimer=setTimeout(()=>triggerLongPressHandler(${intKey}), 750)"
             onmouseup="clearTimeout(longPressTimer); handleCardInteraction(${intKey}, event)"
             onmouseleave="clearTimeout(longPressTimer); longPressTriggered=false;"
             ontouchstart="longPressTimer=setTimeout(()=>triggerLongPressHandler(${intKey}), 750)"
             ontouchend="clearTimeout(longPressTimer); handleCardInteraction(${intKey}, event); longPressTriggered=false;"
             ontouchcancel="clearTimeout(longPressTimer); longPressTriggered=false;"
             >
            <div class="avatar">${initial}</div>
            <div class="contact-info">
              <div class="contact-name">${name}</div>
              <div class="contact-status">Mesh Link Active (ID: ${intKey})</div>
            </div>
            ${peerBadge}
            
            <div class="delete-reveal-overlay">
              <span class="delete-reveal-btn" onclick="deleteDeviceAction(${intKey}, event)">Delete Node</span>
            </div>
          </div>`;
      });
      container.innerHTML = html;
    }

    function openChat(id) {
      activePeer = id;
      resetAllEdits(); // Safety close
      unreadCounts[id] = 0; 
      renderContacts();
      document.getElementById('chatPartnerName').innerText = id === 0 ? "Squad Group" : (nodeNames[id] || "device " + id);
      document.body.classList.add('in-chat');
      updateUI();
    }

    function closeChat() {
      activePeer = null;
      document.body.classList.remove('in-chat');
      renderContacts();
    }

    function triggerPersonalSOS() {
      if(activePeer === null) return;
      let targetName = activePeer === 0 ? "EVERYONE (Broadcast)" : nodeNames[activePeer];
      if(!confirm("Send EMERGENCY SOS Mesh Alert to " + targetName + "?")) return;
      
      let sosWithGPS = "[EMERGENCY SOS] Urgent assistance required! Coordinates: 12.9239 N, 77.4997 E";
      fetch(`/sendMessage?dest=${activePeer}&msg=${encodeURIComponent(sosWithGPS)}`).then(() => {
        updateUI();
      });
    }

    function updateUI() {
      fetch('/getMessages').then(r => r.json()).then(data => {
        allMsgs = data;
        
        let counterUpdateNeeded = false;
        allMsgs.forEach(m => {
          if (m.id > maxSeenId) {
            if (m.dir === 0) { 
              if (activePeer !== null && m.room === activePeer) {
                // Active room open
              } else {
                unreadCounts[m.room] = (unreadCounts[m.room] || 0) + 1;
                counterUpdateNeeded = true;
              }
            }
            maxSeenId = m.id;
          }
        });
        
        if (counterUpdateNeeded) {
          renderContacts();
        }
        
        if(activePeer === null) return;
        
        if (unreadCounts[activePeer] && unreadCounts[activePeer] > 0) {
          unreadCounts[activePeer] = 0;
          renderContacts();
        }
        
        const list = document.getElementById('msgList');
        const filtered = allMsgs.filter(m => m.room == activePeer);
        let html = '';
        
        filtered.forEach(m => {
          const isSOS = m.text.includes("[EMERGENCY SOS]");
          let cls = m.dir === 1 ? 'sent' : 'rcvd';
          if(isSOS) cls += ' sos-bubble';
          
          let label = '';
          if (m.room == 0 && m.dir === 0) {
            let senderName = nodeNames[m.sender] || "device " + m.sender;
            label = `<span class="sender-label">${senderName}</span>`;
          }
          
          let content = m.text;
          if (isSOS) {
            content = `
              <div onclick="window.open('https://maps.google.com/?q=12.9239,77.4997', '_blank')">
                <div>${m.text}</div>
                <div class="map-card-link">📍 Tap to view Map</div>
              </div>`;
          }
          
          html += `<div class="bubble ${cls}">${label}${content}</div>`;
        });
        
        if (list.innerHTML !== html) {
          list.innerHTML = html;
          list.scrollTop = list.scrollHeight;
        }
      });
    }

    function doSend() {
      const input = document.getElementById('msgInput');
      const text = input.value.trim();
      if(!text || activePeer === null) return;
      input.value = '';
      fetch(`/sendMessage?dest=${activePeer}&msg=${encodeURIComponent(text)}`).then(() => updateUI());
    }

    renderContacts();
    setInterval(updateUI, 1200);
  </script>
</body>
</html>
)rawhtml";

// ==========================================
// 6. SERVER HANDLERS
// ==========================================
void handleRoot() {
  String html = MAIN_page;
  html.replace("%NODE_ID%", String(NODE_ID));
  server.send(200, "text/html", html);
}

void handleSendMessage() {
  int dest = server.arg("dest").toInt();
  String msg = server.arg("msg");
  if (msg.length() > 0 && !txPending) {
    txTarget = dest;
    txPayload = msg;
    txPending = true;
    addMessage(1, dest, NODE_ID, msg);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "BUSY");
  }
}

void handleGetMessages() {
  String json = "[";
  for (int i = 0; i < msgCount; i++) {
    json += "{\"id\":" + String(msgHistory[i].id) +
            ",\"dir\":" + String(msgHistory[i].dir) +
            ",\"room\":" + String(msgHistory[i].room) +
            ",\"sender\":" + String(msgHistory[i].sender) +
            ",\"text\":\"" + sanitizeText(msgHistory[i].text) + "\"}";
    if (i < msgCount - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

// ==========================================
// 7. LORA MESH PACKET ENGINE
// ==========================================
void sendLoRaPacket(int originalSender, int finalRecipient, uint16_t packetID, int hopCount, String payload) {
  uint8_t payloadBytes[256];
  int payloadLen = payload.length();
  if (payloadLen > 240) payloadLen = 240;
  
  for (int i = 0; i < payloadLen; i++) {
    payloadBytes[i] = (uint8_t)payload[i];
  }

  LoRa.beginPacket();
  LoRa.write((uint8_t)originalSender);
  LoRa.write((uint8_t)finalRecipient);
  LoRa.write((uint8_t)((packetID >> 8) & 0xFF));
  LoRa.write((uint8_t)(packetID & 0xFF));
  LoRa.write((uint8_t)hopCount);
  LoRa.write((uint8_t)payloadLen);
  LoRa.write(payloadBytes, payloadLen);
  LoRa.endPacket();
  LoRa.receive(); 
}

// ==========================================
// 8. CORE SYSTEM SETUP & OPERATION LOOP
// ==========================================
void setup() {
  Serial.begin(115200);

  // Setup Hardware Input/Output Pin Modes
  pinMode(HARDWARE_SOS_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  
  // Ensure LEDs start OFF
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW); delay(10);
  digitalWrite(RST_PIN, HIGH); delay(10);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(LORA_FREQUENCY)) { 
    while (1); 
  }

  // --- MAXIMUM RANGE SETTINGS ---
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.enableCrc();
  LoRa.receive();

  WiFi.softAP(("NODE_CONSOLE_" + String(NODE_ID)).c_str(), "12345678");
  server.on("/", handleRoot);
  server.on("/sendMessage", handleSendMessage);
  server.on("/getMessages", handleGetMessages);
  server.begin();
}

void loop() {
  server.handleClient();

  // --- 1. Physical Hardware SOS Button Handling ---
  static unsigned long lastButtonTrip = 0;
  if (digitalRead(HARDWARE_SOS_PIN) == LOW && (millis() - lastButtonTrip > 3000)) {
    lastButtonTrip = millis();
    if (!txPending) {
      txTarget = 0;
      txPayload = "[EMERGENCY SOS] Urgent assistance required! Coordinates: 12.9239 N, 77.4997 E";
      txPending = true;
      addMessage(1, 0, NODE_ID, txPayload);
    }
  }

  // --- 2. Local User Outbound Engine ---
  if (txPending) {
    alreadySeen(NODE_ID, packetCounter);
    sendLoRaPacket(NODE_ID, txTarget, packetCounter, 0, txPayload);
    
    packetCounter++;
    txPending = false;
  }

  // --- 3. Radio Reception & Real-Time Hop Relay Engine ---
  int packetSize = LoRa.parsePacket();
  if (packetSize >= 6) {
    int originalSender  = LoRa.read();
    int finalRecipient  = LoRa.read();
    uint16_t packetID   = ((uint16_t)LoRa.read() << 8) | (uint16_t)LoRa.read();
    int hopCount        = LoRa.read();
    int payloadLen      = LoRa.read();
    String payload = "";
    int bytesRead = 0;
    
    while (LoRa.available() && bytesRead < payloadLen) {
      payload += (char)LoRa.read();
      bytesRead++;
    }

    if (originalSender == NODE_ID) return;
    if (alreadySeen(originalSender, packetID)) return;
    
    // ** LED TRIGGERING **
    // Check relevancy before blinking (must be direct message to us OR group broadcast)
    if (finalRecipient == 0 || finalRecipient == NODE_ID) {
      // Check payload string for SOS pattern
      if (payload.indexOf("[EMERGENCY SOS]") != -1) {
        // Trigger Red rapid SOS sequence (for 5 seconds)
        redLedSequenceEndTime = millis() + 5000;
      } else {
        // Trigger Green normal blink (solid ON for 1 second)
        digitalWrite(GREEN_LED_PIN, HIGH);
        greenLedTurnOffTime = millis() + 1000;
      }
    }

    // Continue processing logic
    if (finalRecipient == 0) {
      addMessage(0, 0, originalSender, payload);
      if (hopCount < 3) {
        delay(random(100, 300));
        sendLoRaPacket(originalSender, 0, packetID, hopCount + 1, payload);
      }
    } 
    else if (finalRecipient == NODE_ID) {
      addMessage(0, originalSender, originalSender, payload);
    } 
    else {
      if (hopCount < 3) {
        delay(random(100, 300));
        sendLoRaPacket(originalSender, finalRecipient, packetID, hopCount + 1, payload);
      }
    }
  }

  // --- 4. Non-Blocking LED Hardware Engines ---
  
  // Normal Message Indicator Engine (GPIO 12)
  if (greenLedTurnOffTime > 0 && millis() >= greenLedTurnOffTime) {
    digitalWrite(GREEN_LED_PIN, LOW);
    greenLedTurnOffTime = 0; 
  }

  // SOS Emergency Indicator Engine (GPIO 13) - Rapid Blink
  if (redLedSequenceEndTime > 0) {
    if (millis() < redLedSequenceEndTime) {
      if (millis() - redLedLastToggleTime >= 150) {
        currentRedHardwareState = !currentRedHardwareState;
        digitalWrite(RED_LED_PIN, currentRedHardwareState ? HIGH : LOW);
        redLedLastToggleTime = millis();
      }
    } else {
      digitalWrite(RED_LED_PIN, LOW);
      redLedSequenceEndTime = 0;
      currentRedHardwareState = false;
    }
  }
}