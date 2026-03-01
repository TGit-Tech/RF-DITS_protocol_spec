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
 * @defgroup PKB Radio Packet Byte Order/Offsets idx 
 * @{
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

#define PKB_XDATA_BEG   6   ///< XDATA Start DITINFO & VALS
///@}
/*********************************************/ /**
 * @defgroup PKT Radio Packet-Types 
 * @{
 ************************************************/
// Packet Constants
#define PKC_RADIOPKTMAXBYTES_MIN      10    ///< Minimum allowed value for 'RadioPktMaxBytes'
#define PKC_RADIOPKTMAXBYTES_DEFAULT  58    ///< Default for 'RadioPktMaxBytes'
#define PKC_RADIOPKTMAXBYTES_MAX      512   ///< Maximum allowed value for 'RadioPktMaxBytes'

// REQuest PacKet-Types (0x7F-0x60 bit5=1)
#define PKT_TYPEMIN                 0xF0  ///< Used to boundry check arguments
#define PKT_REQDITINFO              0xF0  ///< REQ the DIT info
#define PKT_REQVALS                 0xF1  ///< REQ all dev values
#define PKT_REQVAL                  0xF2  ///< REQ a dev value
#define PKT_REQNONCE                0xF3  ///< REQ a Nonce Response
#define PKT_SETVAL                  0xF4  ///< REQ to SET a Device Value
#define PKT_DITINFO                 0xF5  ///< The DIT-Table information
#define PKT_VALS                    0xF6  ///< All Device values on a Node
#define PKT_VAL                     0xF7  ///< A Device value
#define PKT_NONCERSP                0xF8  ///< The Req-nonce response
#define PKT_TYPEMAX                 0xF8  ///< Used to boundry check arguments
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
 *     - Outgoing packets can be generated internally when requested by the engine (e.g., REQDITINFO, PKT_VAL).
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
    const byte      DITNameChar;        ///< Use char[DITNameChar] to allocate; which is DITNameBytes +1 for null-term.
  private:
    const byte      DITNameBytes;       ///< Stores the raw byte-size of a Name Field.  See DITNameChar
    const uint16_t  pmMaxDITRecords;    ///< The maximum number of DIT Records allowed in persistant memory allocation.
    const uint16_t  pmSecNetAddr;       ///< Reserves 1-Byte for the 'SecNet' code.
    const uint16_t  pmDITbase;          ///< Stores the DITS records index'ed base address.
    const uint16_t  pmDITend;           ///< Stores the DITS records index'ed end address.
    const uint16_t  pmDITEndAddr;       ///< Stores the actual DITS records end addres including fields after index'ed.
    uint16_t pmNDITAddr(byte idx) const {return pmDITbase + (idx * (PMO_NAME + DITNameBytes));}
    uint16_t pmDDITAddr(byte idx) const {return pmDITend  - (idx * (PMO_NAME + DITNameBytes));}
  
  public:
  /// @defgroup DITS 1. DITS Core Management 
  /// @{
    ///@brief Initializes the DITS Engine and prepares memory structures.
    ///@important Must be called only once before any other calls (e.g. within setup()).
    void begin();

    ///@brief Engine periodic processing loop.
    ///@details Must be called regularly from main loop().
    ///Handles packet timeout and future time-based tasks.
    void ProcessLoop();

    ///@brief Updates the RF address for this node in DIT records.
    ///@details This RF address is used for communications within the protocol.
    ///@important Keeping this updated is critical for node identification.
    ///@param[in] RFAddr The RF address of this node.
    void UpdateThisNodeRFAddr(uint16_t RFAddr);

    ///@brief Updates the name of this node in DIT records.
    ///@details The human-readable name of this node.
    ///@note The name length is defined by the DITSEngine 'NameFieldBytes' constructor
    /// argument.  Use 'char[DITNameChar]' to allocate the char buffer.
    ///@param[in] name A string representing the human-readable name of the node.
    void UpdateThisNodeName(const char* name);

    ///@brief Adds a device to this node.
    ///@details Adds a local device of the specified type to this node's device table (DIT record).
    ///@param[in] DevType The type of device to add.
    ///@param[in] DevAttr The attributes for the device.
    ///@param[in] name The name of the device.
    ///@return true if successfully added, false if the table is full or an error occurred.
    bool AddThisNodeDevice(byte devType, byte devAttr, const char* name);

    ///@brief Deletes a device from this node.
    ///@details Removes a local device at the given index from this node's device table (DIT record).
    ///@param[in] devIdx Index of the device to remove.
    ///@return true if successfully removed, false if index is invalid.
    bool DelThisNodeDevice(byte devIdx);
    
    ///@class NodeDIT
    ///@brief A NodeDIT structure that points directly to persistent memory.
    class NodeDIT {
      private:
        friend class DITSEngine;  
        DITSEngine* pPtr;         ///< pointer to the parent DITSEngine object for 'pmem' access.
        byte NDITidx = 0;         ///< idx tracker for NodeDIT object.
        NodeDIT(DITSEngine* _ThisPtr, byte _NDITidx = 0) : pPtr(_ThisPtr), NDITidx(_NDITidx) {}
      public:
        void      Next()            { do{NDITidx++;} while(IsValid() && IsDeleted()); }
        bool      IsValid() const   {return (NDITidx < pPtr->NDITStopIdx);}
        byte      DITidx() const    {return NDITidx;}
        byte      NodeIdx() const   {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) : BNONE;}
        byte      NRFAddrH() const  {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRH) : BNONE;}
        byte      NRFAddrL() const  {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRL) : BNONE;}
        uint16_t  NRFAddr() const   {return IsValid() ? word(NRFAddrH(),NRFAddrL()) : BNONE;}
        byte      NDITVer() const   {return IsValid() ? pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NDITVER) : BNONE;}
        void NGetName(char* buffer) const {
          if (!IsValid()) return;
          for (int i = 0; i < pPtr->DITNameBytes; i++) {buffer[i] = pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NAME + i);}
          buffer[pPtr->DITNameBytes] = '\0';}
        bool IsDeleted() const {
          if (!IsValid()) return true; return pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) == BNONE; }
      protected:
        void      NextAll()            {NDITidx++;}
        void      NodeIdx(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX, value);}
        void      NRFAddrH(byte value) {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRH, value);}
        void      NRFAddrL(byte value) {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NRFADDRL, value);}
        void      NDITVer(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NDITVER, value);}
        void NSetName(const char* value) {
          if (!IsValid()) return; byte len = strlen(value);
          for (int i = 0; i < pPtr->DITNameBytes; i++) {pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NAME + i, (i < len) ? value[i] : '\0');}
        }
        bool IsDeleted(bool deleteStatus) {
          if (!IsValid()) return true;
          if (deleteStatus && NDITidx != 0) {pPtr->pmem_write(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX, BNONE);}
          return pPtr->pmem_read(pPtr->pmNDITAddr(NDITidx) + PMO_NODEIDX) == BNONE; }
    };
    ///@class DeviceDIT
    ///@brief A DeviceDIT structure that points directly to persistent memory.
    class DeviceDIT {
      private:
        friend class DITSEngine;
        DITSEngine* pPtr;
        byte DDITidx;
        DeviceDIT(DITSEngine* _ThisPtr, byte _DDITidx = 0) : pPtr(_ThisPtr), DDITidx(_DDITidx) {}
      public:
        void  Next()            { do{DDITidx++;} while(IsValid() && IsDeleted()); }
        byte  DITidx() const    {return DDITidx;}
        bool  IsValid() const   {return (DDITidx < pPtr->DDITStopIdx);}
        byte  DNodeIdx() const  {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) : BNONE; }
        byte  DevUID() const    {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVUID) : BNONE; }
        byte  DevType() const   {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVTYPE) : BNONE; }
        byte  DevAttr() const   {return IsValid() ? pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DEVATTR) : BNONE; }
        void DGetName(char* buffer) const {
          if (!IsValid()) return;
          for (int i = 0; i < pPtr->DITNameBytes; i++) {buffer[i] = pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_NAME + i);}
          buffer[pPtr->DITNameBytes] = '\0';
        }
        bool IsDeleted() const {
          if (!IsValid()) return true; return pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) == BNONE;
        }
      protected:
        void  NextAll()            {DDITidx++;}
        void  DNodeIdx(byte value) {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX, value); }
        void  DevUID(byte value)   {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVUID, value); }
        void  DevType(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVTYPE, value); }
        void  DevAttr(byte value)  {if (IsValid()) pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DEVATTR, value); }
        void DSetName(const char* value) {
          if (!IsValid()) return; byte len = strlen(value);
          for (int i = 0; i < pPtr->DITNameBytes; i++) {pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_NAME + i, (i < len) ? value[i] : '\0');}
        }
        bool IsDeleted(bool deleteStatus) {
          if (!IsValid()) return true;
          if (deleteStatus) {pPtr->pmem_write(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX, BNONE);}
          return pPtr->pmem_read(pPtr->pmDDITAddr(DDITidx) + PMO_DNODEIDX) == BNONE;
        }
    }; 
    ///@brief A NodeDIT object for implementers to access DIT records.
    NodeDIT Node(byte idx) const {return NodeDIT(const_cast<DITSEngine*>(this), idx);}
    ///@brief A DeviceDIT object for implementers to access DIT records.
    DeviceDIT Device(byte dditIdx) const {return DeviceDIT(const_cast<DITSEngine*>(this), dditIdx);} 
  ///@}

  ///@defgroup DITSPUBREM 2. DITS RF Control.
  ///@{
    ///@brief Get the 'SecNet' code.
    ///@details This function returns the security network code in persistent memory. 
    ///The SecNet code is used to uniquely secure and identify the nodes that are part of 
    ///a specific user's node network, ensuring that only authorized nodes can communicate.
    ///@return The current SecNet code stored in persistent memory.
    byte SecNetCode() const;
    
    ///@brief Set the 'SecNet' code.
    ///@see SecNetCode() const
    ///@param[in] _SecNetCode Sets the new security network code (0x00-0x7F).
    ///@return true if the code was in range and written without error.
    bool SecNetCode(byte _SecNetCode);

    ///@brief Sets the maximum time(ms) allowed to build an Rx packet.
    ///@details This timeout should reflect maximum RF range latency and data rate.
    /// Reducing this value can improve throughput by releasing the Rx byte collection from incomplete
    /// or corrupted frames but can also prevent slow incoming packets from ever getting recieved.
    ///@param _RxExpireMS The timeout in milliseconds.  Default is 30s (30000).
    ///@see RxData
    void RxExpireMS(uint32_t _RxExpireMS) { return RxExpireMillis=_RxExpireMS; }
    
    ///@brief Gets the RxExpireMS setting.
    ///@see RxExpireMS(uint32_t)
    uint32_t RxExpireMS() { return RxExpireMillis; }

    ///@brief Processes an incoming byte of radio data and appends it to the current packet.
    ///@param[in] _byte A byte of data from the radio to be processed.
    void RxData(byte _byte);

    ///@brief Sets the maximum packet size for the radio interface.
    ///@details This function sets the maximum allowable packet size (in bytes) for the 
    ///radio interface used by the DITSEngine. The value is used to segment outgoing packets 
    ///into smaller chunks (e.g., REQDITINFO device pools) that fit within the radio's 
    ///hardware limits. This ensures that the maximum allowed payload size per packet 
    ///does not exceed the radio's transmission capability.
    ///@param[in] maxBytes The maximum number of bytes a single radio packet can carry.
    ///@note The value is volatile and resets to 58 bytes after every power cycle.
    ///The valid range for `maxBytes` is 10 to 512 bytes. Any value out of this range 
    ///will be ignored. The 512-byte limit is defined by the protocol, not the radio hardware.
    ///@return true if the value was set successfully, or false if the value is out of range.
    bool RadioPktMaxBytes(uint16_t maxBytes);

    ///@brief Gets the maximum packet size for the radio interface.
    ///@see RadioPktMaxBytes(uint16_t)
    uint16_t RadioPktMaxBytes() { return mRadioPktMaxBytes; }

    ///@brief Send an RF request to upload a remote nodes DIT record to allow control of it.
    ///@details Transmits the PKT_REQDITINFO packet to a given RF address.  The remote nodes
    /// DIT table will exist in this nodes DIT records if the request is fulfilled.
    ///@param[in] RFAddr 16-bit RF address of the remote node to add.
    ///@return true if packet was created successfully, else false.
    bool TxAddRemoteNode(int RFAddr);

    ///@brief Delete a remote node.
    ///@details Removes the specified remote node record from this nodes DIT records.
    ///@param[in] nodeIdx Index of the remote node to delete.
    void DelRemoteNode(byte nodeIdx);

    ///@brief Send an RF request to write/set a new value for a device on a remote node.
    ///@details Transmits the PKT_SETVAL packet to a remote node.
    ///@param nodeIdx Index of the target node in DIT records.
    ///@param devUID UID of the device to write/set a new value for.
    ///@param value The value to set on the remote device.
    ///@return true if packet was created successfully, else false.
    bool TxSetRemoteDevVal(byte nodeIdx, byte devUID, int value);

    ///@brief Send an RF request to read/get the values of all devices on a remote node.
    ///@details Transmits the PKT_REQVALS packet to a remote node.
    ///@param nodeIdx Index of the target node in DIT records.
    ///@return true if packet was created successfully, else false.
    bool TxGetRemoteDevVals(byte nodeIdx);
  ///@}
  
  protected:
  /// @defgroup OVERLOAD 3. DITS Implementer Callback Functions to be overloaded. 
  /// Function overloading is how DITS utilizes call-back.  Implementer must overload all
  /// of these functions correctly to tie the protocol engine to the hardware it runs on.
  /// @{
    ///@brief Read a byte from persistent memory.
    ///@details The engine uses this function to retrieve stored Node or Device metadata from 
    ///persistent storage. Implementers should override this method to provide
    ///platform-specific access to EEPROM, flash, or other memory backends.
    ///@param addr Memory address to read.
    ///@return The byte stored at the given memory address.
    virtual byte pmem_read(int addr) = 0;

    ///@brief Write a byte to persistent memory.
    ///@details Provides the protocol engine acess to persist memory for DIT records.
    ///@param addr Memory address to write to.
    ///@param val  The byte value to store.
    virtual void pmem_write(int addr, byte val) = 0;
  
    ///@brief Called when the protol has bytes to send to the RF Radio.
    ///@param[in] _ToRFAddr The destination address of the packet waiting to be sent.
    ///@return true if transmission may proceed or false if UART or Radio is busy.
    ///@details Typical flow considerations:
    ///         - UART buffer availability (e.g., Serial.availableForWrite()).
    ///         - RF radio-ready signals (e.g., AUX pin on E22).
    ///         - Leading radio specific header requirments (e.g. address, channel, etc...)            
    ///@important It is the implementers responsibility to send the radio its appropriate header
    ///      just before returning true. The protocol engine will always provide the destination 
    ///      address of the packet by passing it to `_ToRFAddr` on every call.  Address 0xFFFF 
    ///      will be passed if there is no destination address. This allows radio usage flexibility.
    ///@note If RadioPktMaxBytes is known to always be less than the UART buffer depth, the
    ///      implementer may choose to wait until enough space is available in the UART buffer 
    ///      to signal ready. Otherwise, simply checking that the UART buffer is not full is sufficient.
    virtual bool TxReady(uint16_t _ToRFAddr) = 0;
  
    ///@brief Implements the bridge between protocol transmit data and the RF radio.
    ///@param _byte A byte from the protocol engine to pass to the RF radio for transmit.
    ///@note TxReady() indicates when the protocol can send a segment packet bytes.
    virtual void TxData(byte _TxByte) = 0;
  
    ///@brief Called when a remote node requests the current value of a device on this node.
    ///@param ditIdx Index of the device in the local DIT table.
    ///@return The current value of the device.
    virtual int RxReqDeviceValue(byte _DevUID) = 0;
  
    ///@brief Called when a remote node wants to set a device value on this node.
    ///@param _DevUID The Unique ID of the device on this node to set.
    ///@param value The Value to set the device at.
    virtual void RxReqDeviceSet(byte _DevUID, int value) = 0;
  
    ///@brief Called when this node recieves a remote nodes device reading(s).
    ///@details Callback is invoked for each device even if packet contains multiple readings.
    ///@param _NodeIdx Index of the remote node in the local DIT table.
    ///@param _DevUID Unique device identifier within the remote node.
    ///@param value The read value of the device on the remote node.
    virtual void RxDataDevValue(byte _NodeIdx, byte _DevUID, int value) = 0;
  
    ///@brief Notification that the DIT table has changed due to received network data.
    ///@details This callback is invoked when the DIT table is modified as a result of packet 
    ///processing, such as discovering a new remote node or synchronizing due to a detected 
    ///DIT table mismatch. All internal storage updates are completed before this function is called.
    ///@note DIT table change initiated by the implementer does not trigger this callback. 
    ///@param nodeIdx Index of the node entry affected by the change.
    virtual void RxDITUpdate(byte nodeIdx) {}
  ///@}
private:
  class rfPacket;
  class RxPacket;
  class TxPacket;
  //---------------------------------------------------------------------------------
  byte NDITStopIdx = 0;   ///< Last known Node block endstop idx
  byte DDITStopIdx = 0;   ///< Last known Device block endstop idx
  uint32_t RxExpireMillis = 30000;
  uint16_t mRadioPktMaxBytes = PKC_RADIOPKTMAXBYTES_DEFAULT;   ///< The RadioPktMaxBytes per Radio Packet.
  byte AddNDIT();
  byte AddDDIT(byte _DNodeIdx);
  byte FindNodeDIT(uint16_t rfAddr, bool NotFoundAdd = false);
  byte FindDeviceDIT(byte _DNodeIdx, byte _devUID, bool NotFoundAdd = false);
  //---------------------------------------------------------------------------------
  RxPacket* rxPacket = nullptr;
  TxPacket* txPacket = nullptr;
  //---------------------------------------------------------------------------------
  bool RxProcessPacket();
  bool RxSaveNodeDITINFO();
  bool TxSendThisNodeDITINFO(uint16_t RFAddr);
};
//____________________________________________________________________________________________________________________________________________
// ---- rfPacket Nested Base Class ----
class DITSEngine::rfPacket {
  public:
    rfPacket(uint16_t _RadioPktMaxBytes, byte _SecNet) 
    : rfPoolIdxs((_RadioPktMaxBytes<10)?52:(512 + (_RadioPktMaxBytes - 1)) / _RadioPktMaxBytes), 
      uiRadioPktMaxBytes((_RadioPktMaxBytes<10)?10:_RadioPktMaxBytes),
      SecNet(_SecNet) 
    { rfPool = new byte*[rfPoolIdxs]; 
      for (int i=0; i<rfPoolIdxs; i++) {rfPool[i]=nullptr;} 
      rfPool[0] = new byte[uiRadioPktMaxBytes];
    }
    virtual ~rfPacket() {
      for (int i = 0; i<rfPoolIdxs; i++) {if (rfPool[i]) {delete[] rfPool[i];rfPool[i] = nullptr;}}
      delete[] rfPool; rfPool = nullptr;
    }
#if (DB_INFO && DB_RF)
    void dumpRFPool() {
      if(!rfPool) return;
      int linear = 0;
      for (int i = 0; i < rfPoolIdxs; i++) {
        if (!rfPool[i]) continue; // Skip empty chunks
        for (int j = 0; j < uiRadioPktMaxBytes; j++) {
          if (rfPool[i][j] < 0x10) { DBRFINFOSP(("0")) }
          DBRFINFOSP((rfPool[i][j],HEX))
          DBRFINFOSP((","))
          if (++linear>=Size) break;
        }
        DBRFINFOSP(("\n"))
        if (linear>=Size) break;
      }      
    }
#endif
    uint16_t const rfPoolIdxs;          ///< The num of rfPool[<Idxs>]
    uint16_t const uiRadioPktMaxBytes;  ///< The num of rfPool[Idx][<RadioPktMaxBytes>]
    byte** rfPool;                      ///< Dyanmic pool of tx-bytes in [<Idxs>][<RadioPktMaxBytes>] form.
    uint16_t  Size = 0;                 ///< Max Packet size is 512-bytes due to SecNet encoding

  protected:
    byte SecNet = 0;              ///< The 'SecNet' code passed-in at Packet Constructor 
};
// ---- RxPacket Nested Derived ----
class DITSEngine::RxPacket : public DITSEngine::rfPacket {
  public:
    RxPacket(uint16_t _RadioPktMaxBytes, byte _SecNet, uint32_t _expireMillis = 30000)
    : rfPacket(_RadioPktMaxBytes, _SecNet), PcktExprMS(_expireMillis) { PcktBegMS = millis(); }
    RxPacket(const RxPacket&) = delete;             // no obj copies, destructor assurance
    RxPacket& operator=(const RxPacket&) = delete;  // no obj copies, destructor assurance
    
    bool  IsComplete(uint32_t _currMillis) {return (PcktBegMS == 0 || (_currMillis - PcktBegMS) > PcktExprMS);}
    bool  IsValid()  {return (PcktBegMS == 0 && Size > 0 && PktType() != BNONE && bIsSecure); }
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
    void IsSecure();             ///< Checks the Packet has correct SecNet and set the Size.
    bool bIsSecure = false;      ///< Flags true if IsSecure() passes, else false.
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
    void pktREQDITINFO() {
      Size+=1;
      rfPool[0][PKB_TYPE] = PKT_REQDITINFO;}
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
    void pktDITINFO(byte _DITVer) {
      Size+=2;
      rfPool[0][PKB_TYPE] = PKT_DITINFO;
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
