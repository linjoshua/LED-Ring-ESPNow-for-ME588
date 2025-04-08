# 🔵 LED Ring Controller with ESP-NOW (ESP32)

This project allows an ESP32-based robot controller to:
- Select and display a team color on an LED ring
- Send `ENABLE` / `DISABLE` commands to other ESP32 boards via ESP-NOW
- Receive and display FSM states using LED animations
- Use manual serial input (0–6) to debug or test animations

---

## 🧠 FSM State Mapping

| State | Meaning         | LED Behavior          |
|-------|------------------|------------------------|
| 0     | Drive Forward    | Clockwise rotation     |
| 1     | Drive Backward   | Counter-clockwise rot. |
| 2     | Launch Left      | Slow blinking          |
| 3     | Launch Right     | Fast blinking          |
| 4     | Stop             | Always on              |
| 5     | To Home          | Wipe fill              |
| 6     | At Home          | Comet tail             |

---

## 🧩 Hardware Setup

- **ESP32 DevKit-C**
- **LED Ring (WS2812)** connected to pin **13**
- **Color Select Button** connected to **GPIO 14**
- **Enable Button** connected to **GPIO 26**

---

## 📡 ESP-NOW Protocol

### Messages Sent:
- When Enable button is pressed → sends `ENABLE` command
- After 15 seconds → sends `DISABLE` command

### Messages Received:
- Command `3` with `state` 0–6 → updates LED ring to reflect FSM status

---

## 🧪 Debug Option

You can manually type numbers `0` to `6` in the Serial Monitor (baud rate `115200`) to trigger FSM state animations without using ESP-NOW.

---

## 📂 File Structure

