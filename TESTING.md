# Bench testing checklist

Perform tests on a bench before any outdoor or emergency-use demonstration. Keep a normal phone/radio communication method available.

## 1. Individual board checks

- [ ] Confirm each board has the correct sketch and node ID.
- [ ] Confirm the LoRa module initializes successfully.
- [ ] Confirm the correct Wi-Fi access point appears (`NODE_CONSOLE_1`, `_2`, or `_3`).
- [ ] Connect to the access point and open `http://192.168.4.1`.
- [ ] Check the SOS button and LED wiring without assuming the coordinates are live GPS data.

## 2. Direct messaging

- [ ] Send a private message from Node 1 to Node 2.
- [ ] Send a private message from Node 2 to Node 3.
- [ ] Confirm messages appear at the intended receiver and not as private messages on unrelated nodes.
- [ ] Test broadcast (destination ID 0) and verify the other nodes receive it.

## 3. Multi-hop test

- [ ] First verify Node 1 ↔ Node 2 communication.
- [ ] Verify Node 2 ↔ Node 3 communication.
- [ ] Arrange the devices so Node 1 and Node 3 cannot directly hear each other, but both can hear Node 2.
- [ ] Send a private message from Node 1 to Node 3.
- [ ] Confirm Node 2 relays it and Node 3 receives it.
- [ ] Repeat with different messages and test broadcast separately.

## 4. Record results

Record the board model, radio module, antenna, frequency, settings, approximate separation, environment, and observed result. A successful bench test does not establish reliable range or emergency-service performance.

## Known limitations to account for

- The code uses a fixed placeholder SOS location, not live GPS.
- The relay mechanism is flooding-style and has no route discovery or acknowledgement/retry protocol.
- Duplicate history is finite and stored in RAM; it is lost on reset.
- The firmware does not provide encryption or authentication.
- Radio collisions and packet loss remain possible.
