# **RF-DITS Protocol Specification** 📡
**Version:** 1.2 (Active WIP)
**Focus:** Lightweight, Secure, Peer-to-Peer (P2P) LoRa Communication

---

## **1. Core Concept**
RF-DITS (LoRa-RF Device Index Table with Security) is a **non-hierarchical** protocol. It treats RF as a long-distance bridge between high-density "Stations." Instead of one radio per sensor, RF-DITS uses **one radio per station** to manage up to 46 wired I/Os.

### **The "Learn" Mechanism**
To minimize airtime, nodes do not transmit text or metadata during normal operation.
1. **Discovery:** A Controller requests a **Device Index Table (DIT)** from a Remote.
2. **Caching:** The Controller stores the Remote’s device names, types, and scaling in permanent memory.
3. **Execution:** All subsequent traffic uses **1-byte Short-IDs**, maximizing range and battery life.

---

## **2. Packet Type Definitions**
Communication is split into **Requests (0x7X)** and **Data/Responses (0x4X)**.

| Define | Hex | Description |
| :--- | :--- | :--- |
| `PKT_REQDEVITBL` | `0x7F` | Request the Remote's DIT |
| `PKT_REQVALS` | `0x7E` | Request all device values from a node |
| `PKT_REQVAL` | `0x7D` | Request a specific device value |
| `PKT_REQNONCE` | `0x7C` | Request a Secure Challenge (Remote-driven) |
| `PKT_SETVAL` | `0x7B` | Command to set a value (Triggers staging if RWSS) |
| `PKT_DEVITBL` | `0x4F` | The DIT response (Metadata & Names) |
| `PKT_VALS` | `0x4E` | Bulk data update |
| `PKT_VAL` | `0x4D` | Single data update / XOR Response |
| `PKT_NONCERSP` | `0x4C` | The returned Nonce for XOR calculation |

---

## **3. The Secure-Set (RWSS) Handshake**
For sensitive devices, RF-DITS uses a **Defensive Staging** model:
1. **Controller** sends `PKT_SETVAL`.
2. **Remote** stores the value in a `PendingBuffer` and returns `PKT_REQNONCE` (with a random 32-bit Nonce).
3. **Controller** receives Nonce, performs `Response = Nonce ^ SecNet`, and sends `PKT_NONCERSP`.
4. **Remote** validates math; if correct, the `PendingBuffer` is committed to the physical IO.

---

## **4. Data Standardization**
* **Integer-Only:** All values are transmitted as integers to ensure compatibility with 8-bit and 32-bit MCUs.
* **Scaling:** Precision is handled by the **Scale Exponent** defined in the DIT. (e.g., Value `2345` with Scale `2` = `23.45`).

---

# **RF-DITS: Market Brochure** 🚜

### **Zero Infrastructure. Zero Monthly Fees.**
Why pay for a cellular subscription when you own the land? RF-DITS provides **5-mile** control and monitoring with no towers, no brokers, and no internet required.

### **Designed for Agriculture**
* **Gloves-On Control:** Optimized for physical keypads and sunlight-readable TFTs.
* **High-Density I/O:** Control 46 relays, valves, or sensors from a single station.
* **Rugged 5V Logic:** Native 5V operation means direct integration with industrial sensors—no fragile level shifters needed.

### **Security That Works**
Unlike standard RF remotes that can be "sniffed" and replayed, RF-DITS features **SecNet Handshaking**. Every sensitive command requires a unique, one-time mathematical proof before the station will activate.

### **Hardware Compatibility**
Optimized for the [Arduino Uno R4 Minima](http://googleusercontent.com/shopping_content/0_link) for industrial station control and the [Arduino Uno R4 WiFi](http://googleusercontent.com/shopping_content/1_link) for seamless MQTT bridging.

http://googleusercontent.com/shopping_content/2_card