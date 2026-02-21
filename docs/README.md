---
---
This is Currently a WIP as I extract the code from my Tode-RC project which you can visit to see how it has been implemented.
Also there is a full user-manual and hardware deve guide at www.TGit-Tech.com
Please wait till a Release is published before trying to use the code here. TY.

# **RF-DITS Protocol Specification 📡**

**RF-DITS** (LoRa-RF Device Index Table with Security) is a lightweight, secure communication protocol that enables direct node-to-node **(peer-to-peer, P2P)** communication. It provides real-time device **control** and **monitoring**, including **alarm propagation**, while minimizing RF airtime using **indexed (non-text) communication**. RF-DITS operates without relying on any centralized infrastructure, making it ideal for portable control applications.

---

## **1. Overview 📝**

RF-DITS is built for **simplicity, efficiency, and security**. The **Device Index Table (DIT)** stores static data on all connected **devices**. Data that includes each device's textual name, index alias and value datatype, read/write permissions, limits, and enumerated options.  Other nodes can request the DIT and store it in **permanent memory** as **remote-node configurations**, providing **reliable, persistent access** to device definitions and constraints without relying on a central system or transmitting repeatedly that information with every request.

**Universal Firmware & Hardware:**
The **same firmware** runs on all nodes, whether they are **controller nodes** (handheld remotes) or **remote-controlled nodes** (with sensors and switches). The **hardware** and **firmware** are identical across all nodes; the **role of the node** is determined by the devices it connects to. A node could either act as a **controller** for remote devices or as a **remote-controlled node** that monitors and controls sensors and switches.  This allows for **flexible** and **interoperable** nodes that can seamlessly transition between roles, simplifying the system and eliminating the need for custom firmware for different node types.

This uniform firmware across all nodes also ensures that all devices follow the same **Device Type (DT) configurations**, where each **DT** packs the same properties—such as **scaling**, **limits**, **permissions**, and **enumerated options**—regardless of the node type. This consistency simplifies interoperability and ensures that all devices behave predictably across the network, with no need for customized firmware for each node's role.

---

## **2. RF-DITS vs Wi-Fi Models ⚡**

RF-DITS differs fundamentally from Wi-Fi-based IoT systems by treating RF as a long-distance transport, while relying on local wiring for closely grouped devices. In this manner it isn't competing with Wi-Fi IoT; it's refusing its assumptions.

| **Aspect**         | **RF-DITS**                         | **Wi-Fi IoT (Per-Device Radio)**  |
| ------------------ | ----------------------------------- | --------------------------------- |
| Local Connectivity | Wired devices in one station        | Wireless per device               |
| RF Usage           | Single long-range radio per station | One radio per device              |
| Range              | Multi-mile RF links                 | Short-range only                  |
| Reliability        | Deterministic, wired                | Susceptible to interference       |
| Complexity         | Simple, station-level addressing    | Many devices to configure         |
| RF Failure Points  | One RF endpoint per station         | Many independent RF failure points|

> **Key Idea:** The efficiency and simplicity of RF-DITS features stem from its **station-based approach** — multiple wired devices under one node, managed with a single long-range RF link, rather than Wi-Fi’s “one radio per device” model.

---

## **3. Key Features and Advantages ⚡**

| **Feature**                  | **RF-DITS**                                                                                                                                  | **Common IoT Systems**                                                            |
| ---------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| **Direct P2P Communication** | Enables efficient **node-to-node** RF messaging without centralized infrastructure, allowing low-latency, real-time control.                 | Relies on cloud servers, routers, or centralized servers for communication.       |
| **Multi-Device Nodes** | Each RF-DITS **node** supports multiple physically wired devices, reducing RF connections to a single long-range link.                             | Traditional IoT nodes typically support only one device, requiring Wi-Fi wireless connections for each sensor or actuator. |
| **Device Index Table (DIT)** | Provides a compact, structured mapping of device IDs to names and metadata for lightweight, efficient RF messaging.                          | Often requires repeated transmissions or cloud queries to access device metadata. |
| **Security (SecNet)**        | **SecNet** is a user-defined security code embedded in all packets; value updates also use a **time-based nonce** to ensure integrity and prevent unauthorized changes. | Static keys or centralized servers; often vulnerable to replay attacks.           |
| **Alarm Propagation**        | Push-based alarm notifications to multiple remotes.                                                                                          | Alarm notifications often require polling or querying centralized systems.        |
| **Portability**              | Nodes can be moved easily for flexibility with no cloud dependence.                                                                          | Dependent on network infrastructure; relocation requires reconfiguration.         |
| **Airtime Efficiency**       | Short packets, optimized for minimal airtime use.                                                                                            | Larger packets, causing higher airtime consumption.                               |
| **Value Pulling**            | Nodes can pull values as needed, saving bandwidth.                                                                                           | Continuous polling or subscription models can consume more bandwidth.             |

---

## **4. Device Tables (DIT) and MQTT Device Maps (‘Homie’) 🧩**

While **RF-DITS** is designed to operate completely without centralized infrastructure, it can also integrate into IoT systems such as **OpenHAB** and **Home Assistant** using **MQTT**, providing optional IP-based connectivity without altering the core RF protocol.

The **Device Index Table (DIT)** provides a structured representation of a node’s devices that can be directly mapped to the MQTT **Homie** discovery schema. Each device’s **textual name and metadata** (datatype, permissions, limits, or enumerated options) correspond naturally to MQTT device and property paths.

For example, devices defined in a DIT can be exposed to an MQTT broker using Homie-style topics such as:

```
homie/<node-id>/<device-name>/<property>
```

This mapping allows RF-DITS devices to be **automatically discoverable, hierarchically structured, and persistently represented** within MQTT ecosystems, without any additional configuration, while RF communication continues to use compact indexed identifiers for airtime efficiency.

By separating **RF efficiency** from **IP-level representation**, RF-DITS enables seamless coexistence between low-bandwidth RF control networks and higher-level IoT platforms—without imposing infrastructure requirements on the RF layer itself.

### Example DIT Tables
Below are examples of a Device Index Table (DIT) for Node #0 and a (DIT) for Node #1. Each table shows the index, device name, and device type physically connected to that particular node.
<table style="font-size:0.8em; border-collapse:collapse;">
<tr>
<td>

### Node #0 — RF Address: 0x43572849

| Index | Device Name       | Device Type    |
| ----- | ----------------- | -------------- |
| 0     | Fan Switch        | DT_RW_ONOFF    |
| 1     | Light Switch      | DT_RW_ONOFF    |
| 2     | Heater Control    | DT_RW_ONOFF    |
| 3     | Pump Relay        | DT_RW_ONOFF    |
| 4     | Valve Actuator    | DT_RW_ONOFF    |
| 5     | Temperature Input | DT_RO_ANAINPUT |
| 6     | Distance Sensor   | DT_RO_DIST     |

</td>
<td>

### Node #1 — RF Address: 0xAB12CD34

| Index | Device Name        | Device Type    |
| ----- | ------------------ | -------------- |
| 0     | LED Light          | DT_RW_ONOFF    |
| 1     | Temperature Sensor | DT_RO_ANAINPUT |
| 2     | Humidity Sensor    | DT_RO_ANAINPUT |
| 3     | Fan Switch         | DT_RW_ONOFF    |
| 4     | Alarm Threshold    | DT_DC_SETPOINT |
| 5     | Light Switch       | DT_RW_ONOFF    |
| 6     | Heater Control     | DT_RW_ONOFF    |

</td>
</tr>
</table>

---

## **5. Packet Types 📦**

| **Data Packet** | **Request**   | **Description**                                              |
| --------------- | ------------- | ------------------------------------------------------------ |
| PKT_CONFIG      | PKT_REQCONFIG | Provides the node’s device definitions (Device Index Table). |
| PKT_VAL         | PKT_REQVAL    | Contains the current value of a device.                      |
| PKT_VALS        | PKT_REQVALS   | Contains the current values of multiple devices.             |
| PKT_SETVAL      | —             | Updates a device value facilitating remote control.          |
| PKT_ALARM       | —             | Sends a one-time alarm notification.                         |

---

### **6. Device Type (DT) Configuration 🔧**

Each **Device Type (DT)** encapsulates the following configuration parameters, which are matched with the corresponding firmware code to ensure consistent device behavior:

* **Value Limits**: Each **DT** specifies valid value ranges for its corresponding device. For example, a **temperature sensor (DT_ANATEMP)** may have a valid range from **-40°C to 125°C**, while a **distance sensor (DT_ANADIST)** may be limited to **0-500 meters**.

* **Enumerated Options**: For devices with **enumerated states** (e.g., **ON/OFF**, **LOW/HIGH**), the **DT** defines the available states as integers for compact transmission. For example:

  * **Relay (DT_RELAY)**: Enumerated options might include **ON = 1** and **OFF = 0**.

* **Read/Write Permissions (RO/RW)**: Each **DT** defines whether a device’s value can be **read-only (RO)** or **read/write (RW)**. For example:

  * A **temperature sensor (DT_ANATEMP)** might be **read-only**, while a **relay device (DT_RELAY)** might be **read/write** (since it can be both monitored and controlled).

These configurations ensure that the **firmware** and **communication protocol** are in sync, providing a **reliable and consistent experience** across all devices.

---

### **7. Integer-Value Standard and Scaling 📏🔒**

To simplify **RF communication**, **RF-DITS** uses an **integer-only format** for transmitting device values. This format is particularly well-suited for **embedded systems** like **Arduino**, where **ADC (Analog-to-Digital Conversion)** and **DAC (Digital-to-Analog Conversion)** are limited to integer ranges (e.g., 0-1024 for analog readings).

While this standard works for many devices, some require **greater precision** (e.g., temperature or distance sensors). In these cases, **scaling** is applied, managed **by the firmware** specific to each **Device Type (DT)**.

For example:

* **Temperature Sensor (DT_ANATEMP)**: If the device reads **23.45°C**, the firmware scales the value by **100**, sending **2345** over the air.
* **Distance Sensor (DT_ANADIST)**: If the device measures **12.3 meters**, the firmware scales the value by **10**, sending **123**.

On reception, the scaled value is **converted back** to its original form by dividing it by the scaling factor (e.g., **2345** becomes **23.45°C** when divided by 100).

This approach ensures that devices requiring better precision can transmit values efficiently, while still adhering to the **integer-only communication** model for all other devices.

---


## **8. Summary 📚**

RF-DITS offers a simple, secure, and portable solution for device management in remote-control applications. It provides key advantages over traditional IoT systems, including **no dependency on infrastructure**, efficient **alarm propagation**, **portability**, and **minimal communication overhead**. The optional **IoT** integration provides the flexibility to extend RF-DITS to cloud-based or home automation systems if needed, but it remains fully functional without the need for such infrastructure. By consolidating multiple devices under single nodes, RF-DITS achieves long-range control with minimal RF overhead, a clear advantage over per-device wireless approaches.

---
