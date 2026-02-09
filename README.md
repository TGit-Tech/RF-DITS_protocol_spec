# RF-DITS_protocol_spec
LoRa-RF Standalone Portable IO remote control protocol using Peer-to-Peer with Device Indexing and Security


---

# **RF-DITS Protocol Specification 📡**

RF-DITS (Radio Frequency Device Index Table with Security) is a lightweight, secure communication protocol designed for **Master-Slave** node communication, enabling device management and control without requiring network infrastructure. It is ideal for **industrial** applications and environments with limited or no connectivity.

---

## **1. Overview 📝**

RF-DITS is built for environments that demand low power, efficiency, and security. It allows **Master nodes** to store device configurations and share them with **Slave nodes**. The protocol facilitates **device control** and **monitoring**, including **alarm propagation**, all without relying on a centralized infrastructure.

---

## **2. Key Features ⚡**

* **P2P Communication**: Supports direct **Master-Slave** communication via RF, removing the need for infrastructure or centralized network services.
* **Device Index Table (DIT)**: Maps device IDs to device names, enabling efficient communication between nodes.
* **Security Layer**: Implements a **SecNet** user-set security code for packet verification, preventing unauthorized changes to device values.
* **Alarm Propagation**: **Master nodes** push alarms to **Slave nodes**, notifying of specific device states or conditions.
* **Portability**: Master and Slave nodes are easily relocatable, enabling flexible configurations for various applications.
* **Airtime Efficiency**: Small, efficient packets minimize RF airtime usage, which is crucial in noisy environments.

---

## **3. Packet Types 📦**

### **1. PKT_GETCONFIG**

* **Description**: Sent by **Slave nodes** to request the configuration of the Master node's devices.
* **Direction**: Slave → Master

### **2. PKT_GOTCONFIG**

* **Description**: Sent by the **Master node** to provide the **Slave node** with its device configuration (Device Index Table).
* **Direction**: Master → Slave

### **3. PKT_SETVAL**

* **Description**: Sent by **Slave nodes** to set the value of a device attached to the **Master node**.
* **Direction**: Slave → Master

### **4. PKT_GOTVAL**

* **Description**: Sent by the **Master node** to respond to a value retrieval request from a **Slave node**.
* **Direction**: Master → Slave

### **5. PKT_ALARM**

* **Description**: Sent by **Master nodes** to notify one or more **Slave nodes** of an alarm condition triggered by a device.
* **Direction**: Master → Slave (One-time push)

---

## **4. Device Index Table (DIT) with Security 🔐**

The **Device Index Table (DIT)** is a list stored in the **Master node**, mapping device IDs to device names. This table is shared with **Slave nodes** when requested (via **PKT_GETCONFIG**), allowing the Slave to understand device names during value retrieval or setting operations.

### **Security Layer**

* **SecNet** is a **user-set security code** embedded in every packet for **legitimacy verification**.
* It is used to secure **SETVALUE** operations by combining the **SecNet** with **`time()`** to prevent replay attacks. This ensures that only authorized devices can alter device values.
* **SecNet** also guarantees that **SETVAL** packets are validated, ensuring the integrity and authenticity of the communication.

**Key Points**:

* **SecNet** ensures each packet is from an authorized source.
* **SecNet** provides **time-sensitive** protection against replay attacks during value setting operations.

---

## **5. Alarm Propagation 🚨**

* **Master nodes** send **PKT_ALARM** to all **Slave nodes** in the **Alarm Push List** when a device state change or fault condition occurs.
* **Alarms are a one-time push**, with no acknowledgement required from remote nodes. The **Master node** will only send an **Alarm** once, and it will not resend or clear the alarm.
* **Alarm list**: The **Master node** maintains a list of **Slave node addresses** (up to 8) that are eligible to receive alarm pushes. This list can be manually pruned.

**How it works**:

1. **Master node** triggers an alarm for a device state change.
2. It sends a **PKT_ALARM** to the Slave nodes in the Alarm Push List.
3. **Slave nodes** receive the alarm, process it, and handle it according to their logic.
4. The **Master node** does not resend or clear the alarm; it sends the notification only once.

---

## **6. Push and Pull Model 🔄**

### **Push Model**

* **PKT_ALARM** is pushed from the **Master node** to **Slave nodes** in the Alarm list whenever an alarm condition is triggered.

### **Pull Model**

* **PKT_GETVAL** allows **Slave nodes** to pull values from the **Master node** upon request. The **Master node** responds with the requested data in **PKT_GOTVAL**.

---

## **7. Security and Authentication 🔑**

* **SecNet** ensures secure communication between nodes, validating packet authenticity and preventing unauthorized operations.
* **SETVALUE operations** use a **challenge-response mechanism** that involves **SecNet** and **`time()`** to protect against replay attacks.

---

## **8. Key Advantages of RF-DITS ⚙️**

| **Feature**            | **RF-DITS**                                                         | **Common IoT Systems**                                                     |
| ---------------------- | ------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| **Network Dependency** | No infrastructure required; works in P2P mode.                      | Relies on cloud servers or routers for communication.                      |
| **Security**           | SecNet embedded in packets; **SETVALUE** operations secured.        | Uses static keys or centralized server for security.                       |
| **Portability**        | Master and Slave nodes can be moved easily for flexible deployment. | Dependent on network infrastructure; relocation requires reconfiguration.  |
| **Airtime Efficiency** | Short packets, optimized for minimal airtime use.                   | Larger packets, causing higher airtime consumption.                        |
| **Alarm Propagation**  | Push-based alarm notifications to multiple remotes.                 | Alarm notifications often require polling or querying centralized systems. |
| **Low Overhead**       | Low packet sizes, optimized communication.                          | May have higher communication overhead with larger data payloads.          |
| **Flexibility**        | Easily integrates into custom applications, no cloud dependence.    | Often limited to specific platforms (e.g., Alexa, Google Home).            |
| **Value Pulling**      | **Slave nodes** can pull values as needed, saving bandwidth.        | Continuous polling or subscription models can consume more bandwidth.      |

---

## **9. Optional IoT Integration 🌐**

Although RF-DITS excels as a standalone protocol without relying on any centralized infrastructure, it can optionally be integrated with **IoT platforms** like **MQTT** for advanced use cases such as remote monitoring or home automation integration.

While **RF-DITS** is designed to operate efficiently in field environments without **IoT infrastructure**, optional integrations like **MQTT** enable additional functionality in systems with an internet connection.

---

## **10. Summary 📚**

RF-DITS offers a simple, secure, and portable solution for device management in remote-control applications. It provides key advantages over traditional IoT systems, including **no dependency on infrastructure**, efficient **alarm propagation**, **portability**, and **minimal communication overhead**. The optional **IoT** integration provides the flexibility to extend RF-DITS to cloud-based or home automation systems if needed, but it remains fully functional without the need for such infrastructure.

---

