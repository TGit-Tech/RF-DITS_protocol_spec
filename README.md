---
---
# **RF-DITS Protocol Specification 📡**

**RF-DITS** (LoRa-RF Device Index Table with Security) is a lightweight, secure communication protocol that enables direct node-to-node **(peer-to-peer, P2P)** communication. It provides real-time device **control** and **monitoring**, including **alarm propagation**, while minimizing RF airtime using **indexed (non-text) communication**. RF-DITS operates without relying on any centralized infrastructure, making it ideal for portable control applications.

---

## **1. Overview 📝**

RF-DITS is built for **simplicity, efficiency, and security**. The **Device Index Table (DIT)** stores static data on all connected **devices**. Data that including each device's textual name, index alias and value datatype, read/write permissions, limits, and enumerated options.  Other nodes can request the DIT and store it in **permanent memory** as **remote-node configurations**, providing **reliable, persistent access** to device definitions and constraints without relying on a central system or transmitting repeatedly that information with every request.

<table>
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
<td>

### Node #2 — RF Address: 0x99887766

| Index | Device Name       | Device Type    |
| ----- | ----------------- | -------------- |
| 0     | Pump Relay        | DT_RW_ONOFF    |
| 1     | Heater Control    | DT_RW_ONOFF    |
| 2     | Valve Actuator    | DT_RW_ONOFF    |
| 3     | Temperature Input | DT_RO_ANAINPUT |
| 4     | LED Mode          | DT_RW_ONOFF    |
| 5     | Distance Sensor   | DT_RO_DIST     |
| 6     | Pressure Input    | DT_RO_ANAINPUT |

</td>
</tr>
</table>
---

## **2. Key Features and Advantages ⚡**

| **Feature**                  | **RF-DITS**                                                                                                                                  | **Common IoT Systems**                                                            |
| ---------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| **Direct P2P Communication** | Enables efficient **node-to-node** RF messaging without centralized infrastructure, allowing low-latency, real-time control.                 | Relies on cloud servers, routers, or centralized servers for communication.       |
| **Device Index Table (DIT)** | Provides a compact, structured mapping of device IDs to names and metadata for lightweight, efficient RF messaging.                          | Often requires repeated transmissions or cloud queries to access device metadata. |
| **Security (SecNet)**        | User-set **SecNet** code embedded in packets; value updates use a **time-based nonce** to ensure integrity and prevent unauthorized changes. | Static keys or centralized servers; often vulnerable to replay attacks.           |
| **Alarm Propagation**        | Push-based alarm notifications to multiple remotes.                                                                                          | Alarm notifications often require polling or querying centralized systems.        |
| **Portability**              | Nodes can be moved easily for flexibility with no cloud dependence.                                                                          | Dependent on network infrastructure; relocation requires reconfiguration.         |
| **Airtime Efficiency**       | Short packets, optimized for minimal airtime use.                                                                                            | Larger packets, causing higher airtime consumption.                               |
| **Value Pulling**            | Nodes can pull values as needed, saving bandwidth.                                                                                           | Continuous polling or subscription models can consume more bandwidth.             |

---

## **3. Device Tables (DIT) and MQTT Device Maps (‘Homie’) 🧩**

While **RF-DITS** is designed to operate completely without centralized infrastructure, it can also integrate into IoT systems such as **OpenHAB** and **Home Assistant** using **MQTT**, providing optional IP-based connectivity without altering the core RF protocol.

The **Device Index Table (DIT)** provides a structured representation of a node’s devices that can be directly mapped to the MQTT **Homie** discovery schema. Each device’s **textual name and metadata** (datatype, permissions, limits, or enumerated options) correspond naturally to MQTT device and property paths.

For example, devices defined in a DIT can be exposed to an MQTT broker using Homie-style topics such as:

```
homie/<node-id>/<device-name>/<property>
```

This mapping allows RF-DITS devices to be **automatically discoverable, hierarchically structured, and persistently represented** within MQTT ecosystems, without any additional configuration, while RF communication continues to use compact indexed identifiers for airtime efficiency.

By separating **RF efficiency** from **IP-level representation**, RF-DITS enables seamless coexistence between low-bandwidth RF control networks and higher-level IoT platforms—without imposing infrastructure requirements on the RF layer itself.

---

## **4. Packet Types 📦**

| **Data Packet** | **Request**   | **Description**                                              |
| --------------- | ------------- | ------------------------------------------------------------ |
| PKT_CONFIG      | PKT_REQCONFIG | Provides the node’s device definitions (Device Index Table). |
| PKT_VAL         | PKT_REQVAL    | Contains the current value of a device.                      |
| PKT_VALS        | PKT_REQVALS   | Contains the current values of multiple devices.             |
| PKT_SETVAL      | —             | Updates a remote device with a new value.                    |
| PKT_ALARM       | —             | Sends a one-time alarm notification.                         |

---

## **5. Summary 📚**

RF-DITS offers a simple, secure, and portable solution for device management in remote-control applications. It provides key advantages over traditional IoT systems, including **no dependency on infrastructure**, efficient **alarm propagation**, **portability**, and **minimal communication overhead**. The optional **IoT** integration provides the flexibility to extend RF-DITS to cloud-based or home automation systems if needed, but it remains fully functional without the need for such infrastructure.

---
