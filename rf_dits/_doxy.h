/*! \mainpage Main Page
 * 
 * \section Files Files & Abreviations
 *  - Firmware DateCode [YYMD]
 *    - YY = Last two digits of the Year
 *    - M = (1-9 is Jan-Sept),(A-C is Oct-Dec)
 *    - D = (1-9 is 1-9),(A-V is 10-31)
 *  <br><br>
 *  - rf_dits.ino   = Example RF-DITS implementation
 *  - rf_dits.h     = Header File
 *  - rf_dits.cpp   = code file
 *  - DB.h          = Zero-cost debug macros
 *  - _doxy.h       = This doxygen main page
 *  
 *
 * \section Overview Conceptual Overview
 *    RF-DITS is an RF-LoRa protocol that indexes 'nodes', each having multiple 
 *    'devices' wired to it. The protocol stores in the 'thisnodes' persistent memory the 
 *    following information:
 *    - Names of other nodes
 *    - Device types (including RO/RW permission, value limits, enumerated-set options, and value scaling)
 *    - RF Addresses of other nodes it plans to communicate with.
 * 
 *    Each node stores information about the other nodes it communicates with, which allows 
 *    the protocol to use index-addressing rather than textual addressing for communication.
 *  
 *  - pmem      The persistant memory NDITs and DDITs information is stored-in.
 *  - NDIT      [N]ode   [D]ev [I]ndex [T]able;  An idx'ed list(NNodeIdx) of nodes to communicate with inc. itself.
 *  - DDIT      [D]evice [D]ev [I]ndex [T]able;  An idx'ed list(DevUID) of Devices belonging to a 'node'(DNodeIdx).
 *    
 * \section Limits Limits
 *    1.  255(0-254) Total Devices; that the DDIT Device Pool can store because it is byte addressed (0xFF is reserved).
 *    2.  255(0-254) Total Nodes; that the NDIT Device Pool can store because it is byte addressed (0xFF=Deleted).
 *
 * \section Concepts Index concepts that need to be pointed out.
 *    1.  'NodeIdx' - The NDIT table index count.  It is not used in RF and only used for local memory management.
 *          - A NodeIdx = 0xFF indicates that the table is flagged as deleted.
 *          - 'RFAddr' is the identifier used in Remote Acces Identity.
 *
 *    2.  'DevUID' - The Remote Acces Identifier for a Device.  (Not 'DDITidx').
 *          - 'DDITidx' is the index of a DDIT (device tables) record/slot stored in persistant memory.
 *          - 'DevUID' is stored inside the DDIT record itself.
 *          - Note: However, 'DevUID' is once-derived during AddThisNodeDevice on a continous-count of devices on a node.
 *          - That continous-count of added devices on a node is to ensure uniqueness per node RF-Address.
 *
 * \section DITPmem Persistant Memory Specification
 *    1.  Memory Limits:  Depending on _NameFieldBytes and _MaxNodeTables persistant memory imposes limits.
 *        - The memory manager requires (4 Bytes per node + _NameFieldBytes).
 *        - The memory manager requires (3 Bytes per Device + _NameFieldBytes).
 *        - Node-Tables are allocated separate from the device pool, so the overall memory boundary is determined by what fits.
 *
 * \section ProtoSpec Protocol Specification
 *    1.  Maximum bytes per packet communication:           512 bytes due to SecNet encoding.
 *        - The protocol is designed to perform segmentation when RadioPktMaxBytes is set to less than the number of payload bytes required.
 *    2.  Number of possible SecNet code combinations:      128 unique (0x00 to 0x7F)
 *    3.  Maximum devices per node depends on (_NameFieldBytes).
 *        - All configuration information needs to fit in the 512 maximum bytes per packet.
 *        - Use equation.  Max Devices a Node = (512 - (4 + NameBytes)) / (3 + NameBytes).
 *        - So with a _NameFieldBytes = 10
 *          - (512 - (4 + 10)) = 498 / (3 + 10) = 13.  Equals 38 devices per node.
 *            ** NEW DIT TABLE **
 *
 *    DITS Pmem Allocation will start say at 0x0800  
 *    Every Device can determine Node by it's NodeAddrOffset written at Byte0 from the PMEM_DITS_BASE address
 *    Devices can also use 0xFF as their delete because 0xFF offset put node at top of list
 *    Nodes will use Byte0 as thier display order or (0)this-node or 0xFF deleted
 *    Nodes Records will order incrementally from the base.
 *    Device Records will order from TOP to bottom of the DITS alloc.
 *    When the two meet in the middle the memory is full.
 *
 *                                          **Protocol Standard Bytes Table**
 *                       +------------------------------------------------------------------------------------+
 *                       |                              PACKET TYPE                                           |
 *  +----+---------------+-----------+---------+--------+----------+--------+-----+----------+---------+------+
 *  |Idx | Packet Order  | REQDEVITBL| REQVALS | REQVAL | REQNONCE | SETVAL | VAL | NONCERSP | DEVITBL | VALS |
 *  +----+---------------+-----------+---------+--------+----------+--------+-----+----------+---------+------+
 *  | 0  | PKB_TYPE      |     X     |    X    |   X    |    X     |   X    |  X  |    X     |    X    |  X   |
 *  | 1  | PKB_SECH      |     X     |    X    |   X    |    X     |   X    |  X  |    X     |    X    |  X   |
 *  | 2  | PKB_SECL      |     X     |    X    |   X    |    X     |   X    |  X  |    X     |    X    |  X   |
 *  | 3  | PKB_FROM_RFH  |     X     |    X    |   X    |    X     |   X    |  X  |    X     |    X    |  X   |
 *  | 4  | PKB_FROM_RFL  |     X     |    X    |   X    |    X     |   X    |  X  |    X     |    X    |  X   |
 *  | 5  | PKB_DITVER    |           |    X    |   X    |    X     |   X    |  X  |    X     |    X    |  X   |
 *  | 6  | PKB_DEVUID    |           |         |   X    |    X     |   X    |  X  |    X     |  (X*)   |(X*)  |
 *  | 7  | PKB_VALUEH    |           |         |        |    X     |   X    |  X  |    X     |         |      |
 *  | 8  | PKB_VALUEL    |           |         |        |    X     |   X    |  X  |    X     |         |      |
 *  +----+---------------+-----------+---------+--------+----------+--------+-----+----------+---------+------+
 *  (X*) = XDATA expansion to required payload; max allowed 512-bytes.
 *  +-----------------------+ +-----------------+
 *  | XDATA on DEVITBL      | | XDATA on VALS   |
 *  +------+----------------+ +------+----------+
 *  | Byte | Field          | | Byte | Field    |
 *  +------+----------------+ +------+----------+
 *  | 6    | NodeName0      | | 6    | DevUID   |
 *  | 7    | NodeName1      | | 7    | ValueH   |
 *  | 8    | NodeName2      | | 8    | ValueL   |
 *  | 9    | NodeName3      | | 9    | DevUID   |
 *  | 10   | NodeName4      | | 10   | ValueH   |
 *  | ...  | ... until '\0' | | 11   | ValueL   |
 *  | ?0   | DevType        | | ...  | ...      |
 *  | ?1   | DevAttr        | +------+----------+
 *  | ?2   | DevUID         |
 *  | ?3   | DevName0       |
 *  | ?4   | DevName1       |
 *  | ?5   | DevName2       |
 *  | ?6   | DevName3       |
 *  | ?7   | DevName4       |
 *  | ...  | ... until '\0' |
 *  +------+----------------+ 
 *
 *
 *
 *
 */
