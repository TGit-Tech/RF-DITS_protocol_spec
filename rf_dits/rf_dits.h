/******************************************************************************************************************/ /**
 * @file    rf_dits.h
 * @brief   RF Device-Index-Table with Security protocol
 *********************************************************************************************************************/
#ifndef _RF_DITS_H
#define _RF_DITS_H
#include "DB.h"
#include <ArduinoQueue.h>

#define BNONE 0xFF        ///< Byte NONE/UNSET
#define INONE 0xFFFF      ///< Integer NONE/UNSET
#define SECNETMAX 0x7F    ///< Max Number SecNet can be (code tied, cannot change)
#define PMDITSTOP 0xFE    ///< A sentinel value that marks the end of NDIT and DDIT records in PMEM.
#define VALNOTSET 0x8000  ///< Old Code in RxPacket ValSet I believe
/******************************************************************************/ /**
 * @defgroup PMO [P]ersistant [M]emory [O]ffsets. 0 is Local Only @{
 *********************************************************************************/
// NodeDIT PMEM Offsets (per Node table)
#define PMO_NODEIDX     0   ///< Local Index (1 byte, 0xFF delete, 0xFE DITSTOP)
#define PMO_NRFADDRH    1   ///< Node RF high uint8_t (1 uint8_t)
#define PMO_NRFADDRL    2   ///< Node RF low uint8_t (1 uint8_t)
#define PMO_NDITVER     3   ///< Node DIT Version (Table Version) (1 uint8_t)
// DeviceDIT PMEM Offsets (per Device table)
#define PMO_DNODEIDX    0   ///< Device's Node-Index (1 byte, 0xFF delete, 0xFE DITSTOP)
#define PMO_DEVTYPE     1   ///< Cannot be 0 due to '\0' check on previous DIT name record.
#define PMO_DEVATTR     2   ///< Device UID per Node (1 uint8_t)
#define PMO_DEVUID      3   ///< Device Type (1 uint8_t)
// Both Node and Device Name Offset must be the same.
#define PMO_NAME        4   ///< Name offset for both [N]ode & [D]evice
///@}
/********************************************************/ /**
 * @defgroup PKB Radio Packet Byte Order/Offsets idx @{
 ************************************************************/
#define PKB_TO_RFH      0   ///< Tx Target Node RFAddr High-Byte
#define PKB_TO_RFL      1   ///< Tx Target Node RFAddr Low-Byte
#define PKB_CHANNEL     2   ///< Tx Channnel
#define PKB_SECH        3   ///< Rx/Tx SecNet High-Byte (Rx starts here via NextIdx = 3)
#define PKB_SECL        4   ///< Rx/Tx SecNet Low-Byte
#define PKB_TYPE        5   ///< Rx/Tx Packet-Type (This point forward may differ)
#define PKB_FROM_RFH    6   ///< Rx/Tx Source Node RFAddr High-Byte
#define PKB_FROM_RFL    7   ///< Rx/Tx Source Node RFAddr Low-Byte
#define PKB_DITVER      8   ///< Rx/Tx Source Node DIT-Table Version
#define PKB_DEVUID      9   ///< Rx/Tx Device UID per Node
#define PKB_DEVITBL     10  ///< Rx/Tx Start of DIT-Table Data
#define PKB_VALUEH      10  ///< Rx/Tx Device Value Int-High-Byte
#define PKB_VALUEL      11  ///< Rx/Tx Device Value Int-Low-Byte
///@}
/*********************************************/ /**
 * @defgroup PKT Radio Packet-Types 
 * @brief bit-5 being 1 determines a request @{
 ************************************************/
// Packet Constants
#define PKC_ISREQBIT                  5     ///< bit-5=1 in Packet-Type is for REQuests
#define PKC_STATICBYTES               58    ///< Number of statically allocated uint8_ts (assigned in Rx/Tx-Packet classes)
#define PKC_RADIOPKTMAXBYTES_MIN      20    ///< Minimum allowed value for 'RadioPktMaxBytes'
#define PKC_RADIOPKTMAXBYTES_DEFAULT  58    ///< Default for 'RadioPktMaxBytes'
#define PKC_RADIOPKTMAXBYTES_MAX      512   ///< Maximum allowed value for 'RadioPktMaxBytes'
#define PKC_RXPKTEXPIRE_DEFAULTMS     30000 ///< Maximum time (in milliseconds) that an Rx uint8_t collection can exist before expiration

// REQuest PacKet-Types (0x7F-0x60 bit5=1)
#define PKT_REQDEVITBL              0x7F  ///< REQ the DIT info
#define PKT_REQVALS                 0x7E  ///< REQ all dev values
#define PKT_REQVAL                  0x7D  ///< REQ a dev value
#define PKT_REQSETKEY               0x7C  ///< REQ a Set-Key Code
#define PKT_SETVAL                  0x7B  ///< REQ to SET a Device Value

// DATA Holding PacKet-Types (0x4F-0x40 bit5=0)
#define PKT_DEVITBL                 0x4F  ///< The DIT-Table information
#define PKT_VALS                    0x4E  ///< All Device values on a Node
#define PKT_VAL                     0x4D  ///< A Device value
#define PKT_RSPSETKEY               0x4C  ///< The Set-Key Code
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
 *     - Incoming uint8_ts are processed via RxByte(), which automatically collects packets and validates them.
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
 * - Deletion simply marks the first uint8_t of a block as 0xFF.
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

  DITSEngine(uint16_t _pmemBeginAddr, uint8_t _MaxDITRecords, uint8_t _NameFieldBytes);
  const uint8_t   DITNameBytes;     // Just a variable that stores the size of a Name Field
  const uint8_t   DITNameChar;      // Use this, which is +1 for null-term, to initialize name char[]s
  const uint16_t  pmMaxDITRecords;
  const uint16_t  pmSecNetAddr;     // 1-Byte for SecNet, Const
  const uint16_t  pmDITbase;
  const uint16_t  pmDITend;
  const uint16_t  pmDITEndAddr;
  uint16_t pmNDITAddr(uint8_t idx) const {return pmDITbase + (idx * (PMO_NAME + DITNameBytes));}
  uint16_t pmDDITAddr(uint8_t idx) const {return pmDITend  - (idx * (PMO_NAME + DITNameBytes));}
  //------------------------------------------------------------
  /// @defgroup DITSPUBLOC 1. DITS Public Local Management. @{
  class NodeDIT;
  class DeviceDIT;
  const NodeDIT* Node(uint8_t idx) const;                        // DIT Low-Level Access
  const DeviceDIT* Device(uint8_t nodeIdx, uint8_t devIdx) const;
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
  bool AddThisNodeDevice(uint8_t devType, const char* name);
  /************************************************************************************
     * @brief Deletes a device from this node.
     * @details Removes a local device at the given index from this node's device table (DIT record).
     * @param[in] devIdx Index of the device to remove.
     * @return true if successfully removed, false if index is invalid.
     ***************************************************************************************/
  bool DelThisNodeDevice(uint8_t devIdx);  
  ///@}
  //------------------------------------------------------------
  /// @defgroup DITSPUBREM 2. DITS Public Remote Management. @{
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
  uint8_t SecNetCode() const;
  /************************************************************************************
     * @brief Sets the current security network code (SecNet).
     * @details This function writes a new security network code to persistent memory. 
     * The SecNet code is used to uniquely secure and identify the nodes that are part of 
     * a specific user's node network, ensuring that only authorized nodes can communicate.
     * @param[in] _SecNetCode The new security network code to set (0x00-0x7F).
     ************************************************************************************/
  bool SecNetCode(uint8_t _SecNetCode);
  /***************************************************************************************
     * @brief Processes an incoming uint8_t of radio data and appends it to the current packet.
     * @param[in] _uint8_t A uint8_t of data from the radio to be processed.
     ***************************************************************************************/
  void RxData(uint8_t _uint8_t);
  /************************************************************************************
     * @brief Sets the maximum packet size for the radio interface.
     * @details This function sets the maximum allowable packet size (in uint8_ts) for the 
     * radio interface used by the DITSEngine. The value is used to segment outgoing packets 
     * into smaller chunks (e.g., REQDEVITBL device pools) that fit within the radio's 
     * hardware limits. This ensures that the maximum allowed payload size per packet 
     * does not exceed the radio's transmission capability.
     * @param[in] maxBytes The maximum number of uint8_ts a single radio packet can carry.
     * @note The value is volatile and resets to 58 uint8_ts after every power cycle.
     * The valid range for `maxBytes` is 20 to 512 uint8_ts. Any value out of this range 
     * will be ignored. The 512-uint8_t limit is defined by the protocol, not the radio hardware.
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
  void TxAddRemoteNode(int RFAddr);
  /************************************************************************************
     * @brief Delete a remote node.
     * @details Removes the specified remote node from the system's DIT records.
     * @param[in] nodeIdx Index of the remote node to delete.
     ***************************************************************************************/
  void DelRemoteNode(uint8_t nodeIdx);
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
  bool TxSetRemoteDevVal(uint16_t nodeAddr, uint8_t ditIdx, int value);
  ///@}
protected:
  //------------------------------------------------------------
  /// @defgroup DITSI DITS Implementer Callback Functions to be overloaded.
  /****************************************************************************************
   * @brief Read a uint8_t from persistent memory.
   * @details The engine uses this function to retrieve stored Node or Device metadata from 
   * persistent storage. Implementers should override this method to provide
   * platform-specific access to EEPROM, flash, or other memory backends.
   * @param addr Memory address to read.
   * @return The uint8_t stored at the given memory address.
   ****************************************************************************************/
  virtual uint8_t pmem_read(int addr) = 0;
  /****************************************************************************************
   * @brief Write a uint8_t to persistent memory.
   * @details The engine uses this function to persist Node or Device metadata. Implementers
   * should override this method to provide platform-specific storage access,
   * such as EEPROM, flash, or other memory mechanisms.
   * @param addr Memory address to write to.
   * @param val  The uint8_t value to store.
   ****************************************************************************************/
  virtual void pmem_write(int addr, uint8_t val) = 0;
  /****************************************************************************************
   * @brief Indicates when UART and the RF radio are ready to accept transmit data.
   * @return true if transmission may proceed, false otherwise.
   * @details Typical considerations include:
   *          - The configured RadioPktMaxBytes value.
   *          - UART buffer availability (e.g., Serial.availableForWrite()).
   *          - RF radio-ready signals (e.g., AUX pin on E22).
   *
   * @note If RadioPktMaxBytes is known to always be less than the UART buffer depth, the
   *       implementer may choose to wait until enough space is available in the UART buffer to
   *       signal ready.  Otherwise, simply checking that the UART buffer is not full is sufficient.
   ****************************************************************************************/
  virtual bool TxReady() = 0;
  /****************************************************************************************
   * @brief Implements the bridge between protocol transmit data and the RF radio.
   * @param _uint8_t A uint8_t from the protocol engine to pass to the RF radio for transmit.
   * @note TxReady() indicates when the protocol can send uint8_ts.
   ****************************************************************************************/
  virtual void TxData(uint8_t _uint8_t) = 0;
  /****************************************************************************************
   * @brief Called when a remote node requests the current value of a device on this node.
   * @param ditIdx Index of the device in the local DIT table.
   * @return The current value of the device.
   ****************************************************************************************/
  virtual int RxReqDeviceValue(uint8_t _DevUID) = 0;
  /****************************************************************************************
   * @brief Called when a remote node wants to set a value on this device.
   * @param ditIdx Index of the device in the local DIT table.
   * @param value Value to set.
   ****************************************************************************************/
  virtual void RxReqDeviceSet(uint8_t _DevUID, int value) = 0;
  /****************************************************************************************
   * @brief Notification that the DIT table has changed due to received network data.
   * @details This callback is invoked when the DIT table is modified as a result of packet 
   * processing, such as discovering a new remote node or synchronizing due to a detected 
   * DIT table mismatch. All internal storage updates are completed before this function is called.
   * @note DIT table change initiated by the implementer does not trigger this callback. 
   * @param nodeIdx Index of the node entry affected by the change.
   ****************************************************************************************/
  virtual void RxDITUpdate(uint8_t nodeIdx) {}
  ///@}
private:
  class RxPacket;
  class TxPacket;
  //---------------------------------------------------------------------------------
  uint8_t NDITStopIdx = 0;   ///< Last known Node block endstop idx
  uint8_t DDITStopIdx = 0;   ///< Last known Device block endstop idx
  uint16_t mRadioPktMaxBytes = PKC_RADIOPKTMAXBYTES_DEFAULT;   ///< The RadioPktMaxBytes per Radio Packet.
  void ScanPmemForDITSTOP();
  int8_t AddNDIT();
  int8_t AddDDIT();
  int8_t FindNodeByRF(uint16_t rfAddr);
  //---------------------------------------------------------------------------------
  RxPacket* rxPacket = nullptr;
  TxPacket* txPacket = nullptr;
  //---------------------------------------------------------------------------------
  void RxProcessPacket();
  bool TxSendPacket(TxPacket* _Tx);
  void SaveTodeConfig(RxPacket* rxPacket);
  void TxSendThisNodeDEVITBL(uint16_t RFAddr, uint8_t nodeIdx, uint8_t _SecNet);
};
//____________________________________________________________________________________________________________________________________________
// ---- NodeDIT ----
class DITSEngine::NodeDIT {
  private:
    friend class DITSEngine;  
    DITSEngine* pPtr;         ///< pointer to the parent DITSEngine object
    uint8_t NDITidx = 0;      ///< idx tracker for NodeDIT object.
    NodeDIT(DITSEngine* _ThisPtr, uint8_t _NDITidx = 0) : pPtr(_ThisPtr), NDITidx(_NDITidx) {}
  public:
    void    Next()                  {NDITidx++;}
    uint8_t DITidx() const          {return NDITidx;}
    bool    IsValid() const         {return (NDITidx < pPtr->NDITStopIdx);}
    uint8_t NodeIdx() const         {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) : BNONE;}
    void    NodeIdx(uint8_t value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX, value);}
    uint8_t NRFAddrH() const        {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRH) : BNONE;}
    void    NRFAddrH(uint8_t value) {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRH, value);}
    uint8_t NRFAddrL() const        {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRL) : BNONE;}
    void    NRFAddrL(uint8_t value) {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRL, value);}
    uint8_t NDITVer() const         {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NDITVER) : BNONE;}
    void    NDITVer(uint8_t value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NDITVER, value);}
    void NSetName(const char* value) {
      if (!IsValid()) return; uint8_t len = strlen(value);
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
    uint16_t DDITidx;
    DeviceDIT(DITSEngine* _ThisPtr, uint16_t _DDITidx = 0) : pPtr(_ThisPtr), DDITidx(_DDITidx) {}
  public:
    void    Next()                  {DDITidx++;}
    uint8_t DITidx() const          {DDITidx;}
    bool    IsValid() const         {return (DDITidx < pPtr->DDITStopIdx);}
    uint8_t DNodeIdx() const        {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) : BNONE; }
    void    DNodeIdx(uint8_t value) {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX, value); }
    uint8_t DevUID() const          {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVUID) : BNONE; }
    void    DevUID(uint8_t value)   {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVUID, value); }
    uint8_t DevType() const         {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVTYPE) : BNONE; }
    void    DevType(uint8_t value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVTYPE, value); }
    void DSetName(const char* value) {
      if (!IsValid()) return; uint8_t len = strlen(value);
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
// ---- RxPacket ----
class DITSEngine::RxPacket {
  public:
    RxPacket(uint16_t _RadioPktMaxBytes, uint8_t _SecNet, uint32_t _expireMillis = 30000);
    ~RxPacket();
    RxPacket(const RxPacket&) = delete;             // no obj copies, destructor assurance
    RxPacket& operator=(const RxPacket&) = delete;  // no obj copies, destructor assurance
    
    bool      IsComplete(uint32_t _currMillis) {return (PcktBegMS == 0 || (_currMillis - PcktBegMS) > PcktExprMS);}
    bool      IsValid()  {return (PcktBegMS == 0 || Size != 0 || Type() != BNONE || IsSecure()); }
    bool      IsREQ()    {return bitRead(Type(), PKC_ISREQBIT); }
    uint8_t   Type()     {return (rxPool && rxPool[0]) ? rxPool[0][PKB_TYPE] : BNONE;}
    uint8_t   FromRFH()  {return (rxPool && rxPool[0]) ? rxPool[0][PKB_FROM_RFH] : BNONE;}
    uint8_t   FromRFL()  {return (rxPool && rxPool[0]) ? rxPool[0][PKB_FROM_RFL] : BNONE;}
    int       FromRF()   {return (rxPool && rxPool[0]) ? word(rxPool[0][PKB_FROM_RFH], rxPool[0][PKB_FROM_RFL]) : INONE;}
    uint8_t   NDITVer()  {return (rxPool && rxPool[0]) ? rxPool[0][PKB_DITVER] : BNONE;}
    uint8_t   DevUID()   {return (rxPool && rxPool[0]) ? rxPool[0][PKB_DEVUID] : BNONE;}
    int       Value()    {return (rxPool && rxPool[0]) ? word(rxPool[0][PKB_VALUEH], rxPool[0][PKB_VALUEL]) : INONE;}
    void      RxByte(uint8_t _Byte);     ///< Receives UART uint8_t from RadioRx.
    uint16_t Size = 0;            ///< Carries expected 'Size' of the packet. (Max: 512 uint8_ts)
    uint8_t** rxPool;                    ///< Dyanmic pool of rx-uint8_ts in [<Idxs>][<RadioPktMaxBytes>] form.

  private:
    uint16_t rxPoolIdxs = 0;          ///< The num of rxPool[<Idxs>]
    uint16_t uiRadioPktMaxBytes = 0;  ///< The num of uint8_ts[<RadioPktMaxBytes] in each rxPool[Idx]
    uint32_t PcktBegMS = 0;           ///< 0=Packet Complete, Millis is set in Constructor, 1:1.29 Billionth chance for a set to 0.
    uint32_t PcktExprMS = 0;          ///< Max Millis allowed for uint8_t packeting process.
    uint8_t SecNet = 0;                  ///< The 'SecNet' code passed-in at Packet Constructor
    bool IsSecure();                  ///< Checks the Packet has correct SecNet
    int NextIdx = 3;                  ///< Starts at 3.  In Rx we have no ToAddr(0,1)&Chn(2)...but still uint8_t match w/Tx
};
// ---- TxPacket ----
class DITSEngine::TxPacket {
  public:
    TxPacket(uint8_t _RadioPktMaxBytes, uint8_t _SecNet, uint8_t _Type, int _ToRF, uint8_t _Ver = BNONE, uint8_t _DevUID = BNONE, int _Value = INONE);
    ~TxPacket();                                    // destruct txPool properly
    TxPacket(const TxPacket&) = delete;             // no obj copies, destructor assurance
    TxPacket& operator=(const TxPacket&) = delete;  // no obj copies, destructor assurance

    void  TxByte(uint8_t _Byte);
    int   Secure(uint8_t _SecNet);
    int   Size = 0;  // Max: 512 uint8_ts

  private:
    int ByteIdx = 0;
    uint16_t txPoolIdxs = 0;      ///< The num of txPool[<Idxs>]
    uint16_t uiRadioPktMaxBytes = 0; ///< The num of uint8_ts[<RadioPktMaxBytes] in each rxPool[Idx]
    uint8_t** txPool;                ///< Dyanmic pool of tx-uint8_ts in [<Idxs>][<RadioPktMaxBytes>] form.
    // Ex. txPool[0...8] = new uint8_ts[58]; then txPool[0][0...57] to get/'='set each uint8_t.
    // if (AUX == HIGH && txPool[0]) {   // E22 ready and packet exists
    //   Serial.write(txPool[0], 58);    // send all 58 uint8_ts
    //   delete[] txPool[0];             // free the heap memory
    //   txPool[0] = nullptr;            // avoid dangling pointer
};
//_____________________________________________________________________________________________________________________
#endif
