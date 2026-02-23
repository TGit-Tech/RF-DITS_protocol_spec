/******************************************************************************************************************/ /**
 * @file    rf_dits.h
 * @brief   RF Device-Index-Table with Security protocol
 *********************************************************************************************************************/
#ifndef _RF_DITS_H
#define _RF_DITS_H
#include "DB.h"
#include <Arduino.h>

// Never change these top defines
#define BNONE     0xFF    ///< Byte NONE/UNSET
#define INONE     0xFFFF  ///< Integer NONE/UNSET
#define SECNETMAX 0x7F    ///< Max Number SecNet can be (code tied, cannot change)
#define PMDITSTOP 0xFE    ///< A sentinel value that marks the end of NDIT and DDIT records in PMEM.
#define THISNODE  0       ///< Just to make the code more readable.
#define VALNOTSET 0x8000  ///< Old Code in RxPacket ValSet I believe
/******************************************************************************/ /**
 * @defgroup PMO [P]ersistant [M]emory [O]ffsets. 0 is stored/used Locally Only  @{
 *********************************************************************************/
// NodeDIT PMEM Offsets (per Node table)
#define PMO_NODEIDX     0   ///< Local; Index (1 byte, 0xFF delete, 0xFE DITSTOP)
#define PMO_NRFADDRH    1   ///< Node RF high byte (1 byte)
#define PMO_NRFADDRL    2   ///< Node RF low byte (1 byte)
#define PMO_NDITVER     3   ///< Node DIT Version (Table Version) (1 byte)
// DeviceDIT PMEM Offsets (per Device table)
#define PMO_DNODEIDX    0   ///< Local; Device's Node-Index (1 byte, 0xFF delete, 0xFE DITSTOP)
#define PMO_DEVTYPE     1   ///< Cannot be 0 due to '\0' check on previous DIT name record.
#define PMO_DEVATTR     2   ///< Device ATTR (1 byte)
#define PMO_DEVUID      3   ///< Device UID per Node (1 byte)
// Both Node and Device Name Offset must be the same.
#define PMO_NAME        4   ///< Name offset for both [N]ode & [D]evice
///@}
/********************************************************/ /**
 * @defgroup PKB Radio Packet Byte Order/Offsets idx @{
 ************************************************************/
#define PKB_TYPE        0   ///< Rx/Tx Packet-Type
#define PKB_SECH        1   ///< Rx/Tx SecNet High-Byte
#define PKB_SECL        2   ///< Rx/Tx SecNet Low-Byte
#define PKB_FROM_RFH    3   ///< Rx/Tx Source Node RFAddr High-Byte
#define PKB_FROM_RFL    4   ///< Rx/Tx Source Node RFAddr Low-Byte
#define PKB_DITVER      5   ///< Rx/Tx Source Node DIT-Table Version
#define PKB_DEVUID      6   ///< Rx/Tx Device UID per Node
#define PKB_VALUEH      7   ///< Rx/Tx Device Value Int-High-Byte
#define PKB_VALUEL      8   ///< Rx/Tx Device Value Int-Low-Byte

#define PKB_XDATA_BEG   6   ///< XDATA Start DEVITBL & VALS
///@}
/*********************************************/ /**
 * @defgroup PKT Radio Packet-Types 
 * @brief bit-5 being 1 determines a request @{
 ************************************************/
// Packet Constants
#define PKC_ISREQBIT                  5     ///< bit-5=1 in Packet-Type is for REQuests
#define PKC_STATICBYTES               58    ///< Number of statically allocated bytes (assigned in Rx/Tx-Packet classes)
#define PKC_RADIOPKTMAXBYTES_MIN      20    ///< Minimum allowed value for 'RadioPktMaxBytes'
#define PKC_RADIOPKTMAXBYTES_DEFAULT  58    ///< Default for 'RadioPktMaxBytes'
#define PKC_RADIOPKTMAXBYTES_MAX      512   ///< Maximum allowed value for 'RadioPktMaxBytes'
#define PKC_RXPKTEXPIRE_DEFAULTMS     30000 ///< Maximum time (in milliseconds) that an Rx byte collection can exist before expiration

// REQuest PacKet-Types (0x7F-0x60 bit5=1)
#define PKT_REQ_MAX                 0x7F  ///< Used to boundry check arguments
#define PKT_REQDEVITBL              0x7F  ///< REQ the DIT info
#define PKT_REQVALS                 0x7E  ///< REQ all dev values
#define PKT_REQVAL                  0x7D  ///< REQ a dev value
#define PKT_REQNONCE                0x7C  ///< REQ a Nonce Response
#define PKT_SETVAL                  0x7B  ///< REQ to SET a Device Value
#define PKT_REQ_MIN                 0x7B  ///< Used to boundry check arguments

// DATA Holding PacKet-Types (0x4F-0x40 bit5=0)
#define PKT_DATA_MAX                0x4F  ///< Used to boundry check arguments
#define PKT_DEVITBL                 0x4F  ///< The DIT-Table information
#define PKT_VALS                    0x4E  ///< All Device values on a Node
#define PKT_VAL                     0x4D  ///< A Device value
#define PKT_NONCERSP                0x4C  ///< The Req-nonce response
#define PKT_DATA_MIN                0x4C  ///< Used to boundry check arguments
///@}
/******************************************************************************************************************/ /**
 * @class   DITSEngine
 * @brief   Central engine managing Node and Device DIT tables, packet transport, and hardware abstraction.
 *
 * Responsibilities:
 * - Encapsulates Node/Device metadata (DIT tables) and provides read-only access to implementers.
 * - Handles memory storage and retrieval of Node/Device metadata via a pluggable memory backend.
 *     - Supports EEPROM, RAM, or other persistent memory via overridable pmem_read()/pmem_write().
 * - Provides Rx/Tx packet handling for RF or UART transport.
 *     - Incoming bytes are processed via RxByte(), which automatically collects packets and validates them.
 *     - Outgoing packets can be generated internally when requested by the engine (e.g., REQDEVITBL, PKT_VAL).
 * - Defines abstract hardware interface functions (virtual methods) for implementers to override:
 *     - ReadPin(), WritePin(), or other device-specific control logic.
 *
 * Behavior Notes:
 * - On construction, scans Node Pool and Device Pool to track last-used memory blocks.
 * - When writing a new Node/Device:
 *     - If idx is provided, overwrites that block.
 *     - If idx==-1 (default), finds first deleted block or appends at the end.
 *     - Updates last-used address when writing at the end.
 * - Deletion simply marks the first byte of a block as 0xFF.
 * - Node/Device structures are exposed as read-only nested types (NodeDIT / DeviceDIT).
 * - Event propagation (alarms, device value changes) is outside the scope of this engine.
 *
 * Access Notes:
 * - Implementers interact via:
 *     - `const NodeDIT* GetNode(idx)` or `const DeviceDIT* GetDevice(nodeIdx, devIdx)` for read-only access.
 *     - Virtual hardware functions for physical control.
 *     - Rx/Tx functions for communication; engine handles protocol-specific packet construction and parsing.
 *
 * Purpose:
 * - Separates protocol logic, memory storage, and hardware control from implementer code.
 * - Provides a single, cohesive interface for accessing DIT metadata, handling packets, and controlling hardware.
 *********************************************************************************************************************/
class DITSEngine {
public:

  DITSEngine(uint16_t _pmemBeginAddr, byte _MaxDITRecords, byte _NameFieldBytes);
  const byte   DITNameBytes;     // Just a variable that stores the size of a Name Field
  const byte   DITNameChar;      // Use this, which is +1 for null-term, to initialize name char[]s
  const uint16_t  pmMaxDITRecords;
  const uint16_t  pmSecNetAddr;     // 1-Byte for SecNet, Const
  const uint16_t  pmDITbase;
  const uint16_t  pmDITend;
  const uint16_t  pmDITEndAddr;
  uint16_t pmNDITAddr(byte idx) const {return pmDITbase + (idx * (PMO_NAME + DITNameBytes));}
  uint16_t pmDDITAddr(byte idx) const {return pmDITend  - (idx * (PMO_NAME + DITNameBytes));}
  //------------------------------------------------------------
  /// @defgroup DITSPUBLOC 1. DITS Public Local Management. @{
  class NodeDIT;
  class DeviceDIT;
  const NodeDIT* Node(byte idx) const;                        // DIT Low-Level Access
  const DeviceDIT* Device(byte nodeIdx, byte devIdx) const;
    /************************************************************************************
     * @brief Updates or initializes this node's DIT record.
     * @details Stores or updates this node's information in persistent memory as a DIT record.
     * This record is shared with other nodes on the network via PKT_REQDEVITBL.
     * It must be performed once on empty memory to initialize the node, but the data
     * is persistent and can be updated later to keep the node record in sync
     * with system state. The implementer is responsible for ensuring any changes are synchronized
     * with the persistent storage layer and reflected in DIT communications.
     * @param[in] name The human-readable name of this node.
     * @param[in] RFAddr The RF address of this node.
     ***************************************************************************************/
    void UpdateThisNode(uint16_t RFAddr, const char* name);
    /************************************************************************************
     * @brief Adds a device to this node.
     * @details Adds a local device of the specified type to this node's device table (DIT record).
     * @param[in] name The name of the device.
     * @param[in] DevType The type of device to add.
     * @return true if successfully added, false if the table is full or an error occurred.
     ***************************************************************************************/
    bool AddThisNodeDevice(byte devType, const char* name);
    /************************************************************************************
     * @brief Deletes a device from this node.
     * @details Removes a local device at the given index from this node's device table (DIT record).
     * @param[in] devIdx Index of the device to remove.
     * @return true if successfully removed, false if index is invalid.
     ***************************************************************************************/
    bool DelThisNodeDevice(byte devIdx);  
  ///@}
  //------------------------------------------------------------
  /// @defgroup DITSPUBREM 2. DITS Public RF Management. @{
    /************************************************************************************
     * @brief Engine periodic processing loop.
     * @details Must be called regularly from main loop().
     * Handles packet timeout and future time-based tasks.
     ************************************************************************************/
    void ProcessLoop();
    /************************************************************************************
     * @brief Gets the current security network code (SecNet).
     * @details This function reads the security network code from persistent memory. 
     * The SecNet code is used to uniquely secure and identify the nodes that are part of 
     * a specific user's node network, ensuring that only authorized nodes can communicate.
     * @return The current SecNet code stored in persistent memory.
     ************************************************************************************/
    byte SecNetCode() const;
    /************************************************************************************
     * @brief Sets the current security network code (SecNet).
     * @details This function writes a new security network code to persistent memory. 
     * The SecNet code is used to uniquely secure and identify the nodes that are part of 
     * a specific user's node network, ensuring that only authorized nodes can communicate.
     * @param[in] _SecNetCode The new security network code to set (0x00-0x7F).
     ************************************************************************************/
    bool SecNetCode(byte _SecNetCode);
    /***************************************************************************************
     * @brief Processes an incoming byte of radio data and appends it to the current packet.
     * @param[in] _byte A byte of data from the radio to be processed.
     ***************************************************************************************/
    void RxData(byte _byte);
    /************************************************************************************
     * @brief Sets the maximum packet size for the radio interface.
     * @details This function sets the maximum allowable packet size (in bytes) for the 
     * radio interface used by the DITSEngine. The value is used to segment outgoing packets 
     * into smaller chunks (e.g., REQDEVITBL device pools) that fit within the radio's 
     * hardware limits. This ensures that the maximum allowed payload size per packet 
     * does not exceed the radio's transmission capability.
     * @param[in] maxBytes The maximum number of bytes a single radio packet can carry.
     * @note The value is volatile and resets to 58 bytes after every power cycle.
     * The valid range for `maxBytes` is 20 to 512 bytes. Any value out of this range 
     * will be ignored. The 512-byte limit is defined by the protocol, not the radio hardware.
     * @return true if the value was set successfully, or false if the value is out of range.
     ************************************************************************************/
    bool RadioPktMaxBytes(uint16_t maxBytes);
    /************************************************************************************
     * @brief Requests addition of a remote node and its device table.
     * @details Constructs and transmits a request to add a remote node with the given RF address.
     * Internally, this sends a PKT_REQDEVITBL packet to the remote node to retrieve
     * its current device table. Once the remote node responds, the local engine
     * updates the system's DIT records with the new node and its devices.
     * @param[in] RFAddr 16-bit RF address of the remote node to add.
     ***************************************************************************************/
    bool TxAddRemoteNode(int RFAddr);
    /************************************************************************************
     * @brief Delete a remote node.
     * @details Removes the specified remote node from the system's DIT records.
     * @param[in] nodeIdx Index of the remote node to delete.
     ***************************************************************************************/
    void DelRemoteNode(byte nodeIdx);
    /************************************************************************************
    * @brief Send a PKT_SETVAL to a remote device on a given node.
    * @details Constructs and transmits a PKT_SETVAL packet to the device specified by `ditIdx`
    * on the node identified by `nodeIdx`. The node's address is automatically
    * retrieved from the stored NodeDIT table.
    * @param nodeIdx Index of the target node in the NodeDIT table.
    * @param ditIdx Index of the device on the remote node.
    * @param value The value to set on the remote device.
    * @return true if the PKT_SETVAL packet was successfully constructed and transmitted; 
    * @return false if local validation or transmission failed.
    * @note The return value reflects only the success of sending the packet locally. 
    * It does NOT indicate that the remote device applied the value.
    ***************************************************************************************/
    bool TxSetRemoteDevVal(uint16_t nodeAddr, byte ditIdx, int value);
  ///@}
protected:
  //------------------------------------------------------------
  /// @defgroup DITSI DITS Implementer Callback Functions to be overloaded.
  /****************************************************************************************
   * @brief Read a byte from persistent memory.
   * @details The engine uses this function to retrieve stored Node or Device metadata from 
   * persistent storage. Implementers should override this method to provide
   * platform-specific access to EEPROM, flash, or other memory backends.
   * @param addr Memory address to read.
   * @return The byte stored at the given memory address.
   ****************************************************************************************/
  virtual byte pmem_read(int addr) = 0;
  /****************************************************************************************
   * @brief Write a byte to persistent memory.
   * @details The engine uses this function to persist Node or Device metadata. Implementers
   * should override this method to provide platform-specific storage access,
   * such as EEPROM, flash, or other memory mechanisms.
   * @param addr Memory address to write to.
   * @param val  The byte value to store.
   ****************************************************************************************/
  virtual void pmem_write(int addr, byte val) = 0;
  /**************************************************************************************************
   * @brief Indicates when UART and the RF radio are ready to accept transmit data.
   * @param[in] _ToRFAddr The destination address of the packet waiting to be sent.
   * @return true if transmission may proceed, false otherwise.
   * @details Typical considerations include:
   *          - UART buffer availability (e.g., Serial.availableForWrite()).
   *          - RF radio-ready signals (e.g., AUX pin on E22).
   *          - The configured RadioPktMaxBytes value.
   *          - The radio's specific header requirments (e.g. address, channel, etc...)            
   *
   * @note It is the implementers responsibility to send the radio its appropriate header
   *       just before returning true. The protocol will always provide the destination 
   *       address of the packet by passing it to `_ToRFAddr` on every call.  Address 0xFFFF 
   *       will be passed if there is no destination address. This allows radio usage flexibility.
   * @note If RadioPktMaxBytes is known to always be less than the UART buffer depth, the
   *       implementer may choose to wait until enough space is available in the UART buffer 
   *       to signal ready. Otherwise, simply checking that the UART buffer is not full is sufficient.
   **************************************************************************************************/
  virtual bool TxReady(uint16_t _ToRFAddr) = 0;
  /****************************************************************************************
   * @brief Implements the bridge between protocol transmit data and the RF radio.
   * @param _byte A byte from the protocol engine to pass to the RF radio for transmit.
   * @note TxReady() indicates when the protocol can send bytes.
   ****************************************************************************************/
  virtual void TxData(byte _TxByte) = 0;
  /****************************************************************************************
   * @brief Called when a remote node requests the current value of a device on this node.
   * @param ditIdx Index of the device in the local DIT table.
   * @return The current value of the device.
   ****************************************************************************************/
  virtual int RxReqDeviceValue(byte _DevUID) = 0;
  /****************************************************************************************
   * @brief Called when a remote node wants to set a value on this device.
   * @param ditIdx Index of the device in the local DIT table.
   * @param value Value to set.
   ****************************************************************************************/
  virtual void RxReqDeviceSet(byte _DevUID, int value) = 0;
  /****************************************************************************************
   * @brief Called when a remote node sends the current value of a device on its node.
   * @details This callback is invoked when a remote node sends the current value of a
   *          device by its `DevUID` to this node. 
   * @param _NodeIdx Index of the remote node in the local DIT table.
   * @param _DevUID Unique device identifier within the remote node.
   * @param value The value of the device on the remote node.
   ****************************************************************************************/
  virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) = 0;
  /****************************************************************************************
   * @brief Notification that the DIT table has changed due to received network data.
   * @details This callback is invoked when the DIT table is modified as a result of packet 
   * processing, such as discovering a new remote node or synchronizing due to a detected 
   * DIT table mismatch. All internal storage updates are completed before this function is called.
   * @note DIT table change initiated by the implementer does not trigger this callback. 
   * @param nodeIdx Index of the node entry affected by the change.
   ****************************************************************************************/
  virtual void RxDITUpdate(byte nodeIdx) {}
  ///@}
private:
  class rfPacket;
  class RxPacket;
  class TxPacket;
  //---------------------------------------------------------------------------------
  byte NDITStopIdx = 0;   ///< Last known Node block endstop idx
  byte DDITStopIdx = 0;   ///< Last known Device block endstop idx
  uint16_t mRadioPktMaxBytes = PKC_RADIOPKTMAXBYTES_DEFAULT;   ///< The RadioPktMaxBytes per Radio Packet.
  void ScanPmemForDITSTOP();
  byte AddNDIT();
  byte AddDDIT(byte _DNodeIdx);
  byte FindNodeDIT(uint16_t rfAddr, bool NotFoundAdd = false);
  byte FindDeviceDIT(byte _DNodeIdx, byte _devUID, bool NotFoundAdd = false);
  //---------------------------------------------------------------------------------
  RxPacket* rxPacket = nullptr;
  TxPacket* txPacket = nullptr;
  //---------------------------------------------------------------------------------
  bool RxProcessPacket();
  bool RxSaveNodeDEVITBL();
  bool TxSendThisNodeDEVITBL(uint16_t RFAddr);
};
//____________________________________________________________________________________________________________________________________________
// ---- NodeDIT ----
class DITSEngine::NodeDIT {
  private:
    friend class DITSEngine;  
    DITSEngine* pPtr;         ///< pointer to the parent DITSEngine object
    byte NDITidx = 0;      ///< idx tracker for NodeDIT object.
    NodeDIT(DITSEngine* _ThisPtr, byte _NDITidx = 0) : pPtr(_ThisPtr), NDITidx(_NDITidx) {}
  public:
    void      Next()               {NDITidx++;}
    byte      DITidx() const       {return NDITidx;}
    bool      IsValid() const      {return (NDITidx < pPtr->NDITStopIdx);}
    byte      NodeIdx() const      {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) : BNONE;}
    void      NodeIdx(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX, value);}
    byte      NRFAddrH() const     {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRH) : BNONE;}
    void      NRFAddrH(byte value) {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRH, value);}
    byte      NRFAddrL() const     {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRL) : BNONE;}
    void      NRFAddrL(byte value) {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRL, value);}
    uint16_t  NRFAddr() const      {return IsValid() ? word(NRFAddrH(),NRFAddrL()) : BNONE;}
    byte      NDITVer() const      {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NDITVER) : BNONE;}
    void      NDITVer(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NDITVER, value);}
    void NSetName(const char* value) {
      if (!IsValid()) return; byte len = strlen(value);
      for (int i = 0; i < pPtr->DITNameBytes; i++) {pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NAME + i, (i < len) ? value[i] : '\0');}
    }
    void NGetName(char* buffer) const {
      if (!IsValid()) return;
      for (int i = 0; i < pPtr->DITNameBytes; i++) {buffer[i] = pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NAME + i);}
      buffer[pPtr->DITNameBytes] = '\0';
    }
    bool IsDeleted(bool deleteStatus = false) {
      if (!IsValid()) return true;
      if (deleteStatus && NDITidx != 0) {pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX, BNONE);}
      return pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) == BNONE;
    }
};
// ---- DeviceDIT ----
class DITSEngine::DeviceDIT {
  private:
    friend class DITSEngine;
    DITSEngine* pPtr;
    byte DDITidx;
    DeviceDIT(DITSEngine* _ThisPtr, byte _DDITidx = 0) : pPtr(_ThisPtr), DDITidx(_DDITidx) {}
  public:
    void  Next()               {DDITidx++;}
    byte  DITidx() const       {DDITidx;}
    bool  IsValid() const      {return (DDITidx < pPtr->DDITStopIdx);}
    byte  DNodeIdx() const     {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) : BNONE; }
    void  DNodeIdx(byte value) {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX, value); }
    byte  DevUID() const       {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVUID) : BNONE; }
    void  DevUID(byte value)   {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVUID, value); }
    byte  DevType() const      {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVTYPE) : BNONE; }
    void  DevType(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVTYPE, value); }
    byte  DevAttr() const      {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVATTR) : BNONE; }
    void  DevAttr(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVATTR, value); }

    void DSetName(const char* value) {
      if (!IsValid()) return; byte len = strlen(value);
      for (int i = 0; i < pPtr->DITNameBytes; i++) {pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_NAME + i, (i < len) ? value[i] : '\0');}
    }
    void DGetName(char* buffer) const {
      if (!IsValid()) return;
      for (int i = 0; i < pPtr->DITNameBytes; i++) {buffer[i] = pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_NAME + i);}
      buffer[pPtr->DITNameBytes] = '\0';
    }
    bool IsDeleted(bool deleteStatus = false) {
      if (!IsValid()) return true;
      if (deleteStatus) {pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX, BNONE);}
      return pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) == BNONE;
    }
};
// ---- rfPacket Nested Base Class ----
class DITSEngine::rfPacket {
  public:
    rfPacket(uint16_t _RadioPktMaxBytes, byte _SecNet) 
    : rfPoolIdxs((_RadioPktMaxBytes<10)?52:(512 + (_RadioPktMaxBytes - 1)) / _RadioPktMaxBytes), 
      uiRadioPktMaxBytes((_RadioPktMaxBytes<10)?10:_RadioPktMaxBytes),
      SecNet(_SecNet) 
    { rfPool = new byte*[rfPoolIdxs]; for (int i=0; i<rfPoolIdxs; i++) {rfPool[i]=nullptr;} }
    virtual ~rfPacket() {
      for (int i = 0; i<rfPoolIdxs; i++) {if (rfPool[i]) {delete[] rfPool[i];rfPool[i] = nullptr;}}
      delete[] rfPool; rfPool = nullptr;
    }
    uint16_t const rfPoolIdxs;          ///< The num of rfPool[<Idxs>]
    uint16_t const uiRadioPktMaxBytes;  ///< The num of rfPool[Idx][<RadioPktMaxBytes>]
    byte** rfPool;                      ///< Dyanmic pool of tx-bytes in [<Idxs>][<RadioPktMaxBytes>] form.
    uint16_t  Size = 0;                 ///< Max Packet size is 512-bytes due to SecNet encoding
 
  protected:
    byte const SecNet = 0;              ///< The 'SecNet' code passed-in at Packet Constructor 
};
// ---- RxPacket Nested Derived ----
class DITSEngine::RxPacket : public DITSEngine::rfPacket {
  public:
    RxPacket(uint16_t _RadioPktMaxBytes, byte _SecNet, uint32_t _expireMillis = 30000)
    : rfPacket(_RadioPktMaxBytes, _SecNet), PcktExprMS(_expireMillis) { PcktBegMS = millis(); }
    RxPacket(const RxPacket&) = delete;             // no obj copies, destructor assurance
    RxPacket& operator=(const RxPacket&) = delete;  // no obj copies, destructor assurance
    
    bool  IsComplete(uint32_t _currMillis) {return (PcktBegMS == 0 || (_currMillis - PcktBegMS) > PcktExprMS);}
    bool  IsValid()  {return (PcktBegMS == 0 || Size != 0 || PktType() != BNONE || IsSecure()); }
    bool  IsREQ()    {return bitRead(PktType(), PKC_ISREQBIT); }
    byte  PktType()  {return (rfPool && rfPool[0]) ? rfPool[0][PKB_TYPE] : BNONE;}
    byte  FromRFH()  {return (rfPool && rfPool[0]) ? rfPool[0][PKB_FROM_RFH] : BNONE;}
    byte  FromRFL()  {return (rfPool && rfPool[0]) ? rfPool[0][PKB_FROM_RFL] : BNONE;}
    int   FromRF()   {return (rfPool && rfPool[0]) ? word(rfPool[0][PKB_FROM_RFH], rfPool[0][PKB_FROM_RFL]) : INONE;}
    byte  NDITVer()  {return (rfPool && rfPool[0]) ? rfPool[0][PKB_DITVER] : BNONE;}
    byte  DevUID()   {return (rfPool && rfPool[0]) ? rfPool[0][PKB_DEVUID] : BNONE;}
    int   Value()    {return (rfPool && rfPool[0]) ? word(rfPool[0][PKB_VALUEH], rfPool[0][PKB_VALUEL]) : INONE;}
    void  RxByte(byte _Byte);     ///< Receives UART byte from RadioRx.
  private:
    uint32_t PcktBegMS = 0;      ///< 0=Packet Complete, Millis is set in Constructor, 1:1.29 Billionth chance for a set to 0.
    uint32_t const PcktExprMS;   ///< Max Millis allowed for byte packeting process.
    bool IsSecure();             ///< Checks the Packet has correct SecNet
    int NextIdx = 0;             ///< Starts at 3.  In Rx we have no ToAddr(0,1)&Chn(2)...but still byte match w/Tx
};
// ---- TxPacket Nested Derived ----
class DITSEngine::TxPacket : public DITSEngine::rfPacket {
  public:
    // Constructor ensure RPMB is at least 10.
    TxPacket(byte _RadioPktMaxBytes, byte _SecNet): rfPacket(_RadioPktMaxBytes, _SecNet) {}
    TxPacket(const TxPacket&) = delete;             // no obj copies, destructor assurance
    TxPacket& operator=(const TxPacket&) = delete;  // no obj copies, destructor assurance

    void ToFrom(uint16_t _ToRF, uint16_t _FromRF) {
      Size=+4; // (2) for SecNet
      rfPool[0][PKB_FROM_RFH] = highByte(_FromRF);
      rfPool[0][PKB_FROM_RFL] = lowByte(_FromRF);}
    void pktREQDEVITBL() {
      Size+=1;
      rfPool[0][PKB_TYPE] = PKT_REQDEVITBL;}
    void pktREQVALS(byte _DITVer) {
      Size+=2;
      rfPool[0][PKB_TYPE] = PKT_REQVALS;
      rfPool[0][PKB_DITVER] = _DITVer;}
    void pktREQVAL(byte _DITVer, byte _devUID) {
      Size+=3;
      rfPool[0][PKB_TYPE] = PKT_REQVAL;
      rfPool[0][PKB_DITVER] = _DITVer;
      rfPool[0][PKB_DEVUID] = _devUID;}
    void pktREQNONCE(byte _DITVer, byte _devUID, int _challenge) {
      Size+=5;
      rfPool[0][PKB_TYPE] = PKT_REQNONCE;
      rfPool[0][PKB_DITVER] = _DITVer;
      rfPool[0][PKB_DEVUID] = _devUID;
      rfPool[0][PKB_VALUEH] = highByte(_challenge);
      rfPool[0][PKB_VALUEL] = lowByte(_challenge);}
    void pktSETVAL(byte _DITVer, byte _devUID, int _val) {
      Size+=5;
      rfPool[0][PKB_TYPE] = PKT_SETVAL;
      rfPool[0][PKB_DITVER] = _DITVer;
      rfPool[0][PKB_DEVUID] = _devUID;
      rfPool[0][PKB_VALUEH] = highByte(_val);
      rfPool[0][PKB_VALUEL] = lowByte(_val);}
    void pktDEVITBL(byte _DITVer) {
      Size+=2;
      rfPool[0][PKB_TYPE] = PKT_DEVITBL;
      rfPool[0][PKB_DITVER] = _DITVer;}
    void pktVALS(byte _DITVer) {
      Size+=2; // +XData
      rfPool[0][PKB_TYPE] = PKT_VALS;
      rfPool[0][PKB_DITVER] = _DITVer;
    }
    void pktVAL(byte _DITVer, byte _devUID, int _val) {
      Size+=5;
      rfPool[0][PKB_TYPE] = PKT_VAL;
      rfPool[0][PKB_DITVER] = _DITVer;
      rfPool[0][PKB_DEVUID] = _devUID;
      rfPool[0][PKB_VALUEH] = highByte(_val);
      rfPool[0][PKB_VALUEL] = lowByte(_val);
    }
    void pktNONCERSP(byte _DITVer, byte _devUID, int _val) {
      Size+=5;
      rfPool[0][PKB_TYPE] = PKT_NONCERSP;
      rfPool[0][PKB_DITVER] = _DITVer;
      rfPool[0][PKB_DEVUID] = _devUID;
      rfPool[0][PKB_VALUEH] = highByte(_val);
      rfPool[0][PKB_VALUEL] = lowByte(_val);
    }

    uint16_t  ToRFAddr = 0xFFFF;            ///< Destination RF Address of this packet.
    void      AddTxByte(byte _TxByte);
    void      Secure();
    bool      IsSecured() { return bSecured; }

  private:
    bool      bSecured = false;
};
//_____________________________________________________________________________________________________________________
#endif
