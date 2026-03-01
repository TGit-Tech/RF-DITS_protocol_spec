/*! @mainpage Main Page
 * 
 * @section DESC RF-DITS description.
 *    RF-DITS (RF-LoRa Device Index Table with Security) is a lightweight, binary-based P2P mesh protocol that operates entirely
 *    without any centralized infrastructure. The protocol is architectured around a station based RF control point rather than
 *    a one-radio-per-device model.  Functionality is established through a P2P discovery process (PKT_REQDITINFO) in which any
 *    node can upload the configuration (e.g. node name, wired devices names, device types and their attributes) of another node in
 *    order to provide binary-based control of it.
 *
 *    Station nodes are configured at that station only, by wiring devices, assigning those devices names, types and other attributes.
 *    When another node wants to remotely control that station it 'learns' and stores that stations configuration in persistent memory.
 *    Think of it like a regular 'learn' button on a small remote but instead of just learning how to connect, it learns everything
 *    that connection is setup to control.
 *
 * @section SYNC The DIT record and Synchronization Feature.
 *     
 *    Every **Device** has one DIT(Device Index Table) record (DDIT) and every **node** has one DIT record (NDIT).
 * 
 *    Every node (NDIT) record carries a version number (DITVER) that marks a static point of devices and node information.
 *    DITVER is incremented whenever a node adds, deletes or changes DIT information. Everytime a remote node tries to access 
 *    another node the remote node sends its last learned DITVER.  The reciever compares the two DITVER.  If the two do not
 *    match, the reciever denies the call and automatically sends the updated DIT information (PKT_DITINFO). This versioning 
 *    system ensures the binary-based, device-addressing, packets remain perfectly synchronized.
 *
 * @section DITSPMEM Persistent Memory
 * <ol>
 *    <li> Persistent memory is allocated by the DITEngine Constructor arguments (e.g. pmemBeginAddr, MaxDITRecords, NameFieldBytes)
 *        - The Total Allocation will be = MaxDITRecords * (4 + NameFieldBytes) + 1.
 *            - Example: If you plan to control 10 outside nodes with 10 devices on each having a Name alloc of 10 characters.
 *            - Total Allocation = (10x10+1'thisnode') * (4+10) + 1 = 1,415-bytes (too big for UNO)
 *            - Example: Adjust the NameFieldBytes or the MaxDITRecords to fit in 1KB for UNO.
 *            - For Example: at 10 character names, 14/1KB = ~71 total DIT records.
 *            - Check; Total Allocation = 70 * (4 + 10) + 1 = 981-bytes (Okay for UNO)
 *        - Node Records (NDIT) are stored from the 'base' onward (e.g. Addresses 1,15,29,43, etc...)
 *        - Node Records are matched 1:1 from their 'NDIT' record Idx to their NodeIdx.
 *        - Device Records (DDIT) are stored from the 'end' inward (e.g. Addresses 500,486,472, etc...)
 *        - Device Records are NOT matched 1:1 with their 'DDIT' record index; thus the devUID naming convention.
 *        - Out of Memory is determined when the two collide.
 *        - This onward/inward approach eliminates the need to set specific node boundaries and device boundaries/limits.
 *            - MaxDITRecords doesn't limit the number of nodes or devices specifically.
 *            - MaxDITRecords only limits the total sum of both together (e.g. 1-DIT per node & 1-DIT per device).
 *
 *    <li> Example Table below demonstrates the allocation.
 *        - Sentinel 0xFF on 'Byte0' (i.e. D/NodeIdx) marks the record as deleted.
 *        - Sentinel 0xFE on 'Byte0' (i.e. D/NodeIdx) marks the DITSTOP used to contain Node Records from Device Records.
 *        - DITS will track and move the DITSTOP as records are expanded to facilate fast scanning and record boundaries.
 *        - begin() scans persistant memory for these DITSTOP(s) making the markers persistent over power-cycles.
 * </ol><p></p>
 * <div style="margin-left: 40px;">
 * <table>
 * <tr><td colspan="3"><center>Local Record Management Only</center></td><td colspan="4"><center>Data Used in RF Communication</center></td><td></td></tr>
 * <tr><th>pmemAddr     </th><th>Idx</th><th>Byte0  </th><th>Byte1  </th><th>Byte2  </th><th>Byte3</th><th>Variable Name  </th><th>Content/Descriptor</th></tr>
 * <tr><td>pmemBeginAddr</td><td>NA </td><td>SecNet </td></tr>
 * <tr><td>^+1          </td><td>0  </td><td>NodeIdx</td><td>RFAddrH</td><td>RFAddrL</td><td>DITVer</td><td>NameFieldBytes</td><td>Node [0] DIT Record.</td></tr>
 * <tr><td>^+4+Name     </td><td>1  </td><td>NodeIdx</td><td>RFAddrH</td><td>RFAddrL</td><td>DITVer</td><td>NameFieldBytes</td><td>Node [1] DIT Record.</td></tr>
 * <tr><td>^+4+Name     </td><td>2  </td><td>NodeIdx</td><td>RFAddrH</td><td>RFAddrL</td><td>DITVer</td><td>NameFieldBytes</td><td>Node [2] DIT Record.</td></tr>
 * <tr><td>^+4+Name     </td><td>3  </td><td>DITSTOP</td><td>.......</td><td>.......</td><td>......</td><td>..............</td><td>DITSTOP Node Records.</td></tr>
 * <tr><td>.............</td><td>...</td><td>.......</td><td>.......</td><td>.......</td><td>......</td><td>..............</td><td>Blank Space         </td></tr>
 * <tr><td>v-4-Name     </td><td>3  </td><td>DITSTOP</td><td>.......</td><td>.......</td><td>......</td><td>..............</td><td>DITSTOP Device Records.</td></tr>
 * <tr><td>v-4-Name     </td><td>2  </td><td>DNodeIdx</td><td>DevType</td><td>DevAttr</td><td>DevUID</td><td>NameFieldBytes</td><td>Device[2] DIT Record</td></tr>
 * <tr><td>v-4-Name     </td><td>1  </td><td>DNodeIdx</td><td>DevType</td><td>DevAttr</td><td>DevUID</td><td>NameFieldBytes</td><td>Device[1] DIT Record</td></tr>
 * <tr><td>pmDITend     </td><td>0  </td><td>DNodeIdx</td><td>DevType</td><td>DevAttr</td><td>DevUID</td><td>NameFieldBytes</td><td>Device[0] DIT Record</td></tr>
 * <tr><td>pmDITEndAddr </td><td colspan="6"></td><td>Actual End is 4+Name</td></tr>
 * </table>
 * </div>
 *
 * @important Changing construtor arguments on a node will contaminate all previous records stored in persistent memory.  In theory (ill advised) one could
 *            change just the MaxDITRecords and retain previous records but their Device Scanning Loops will expand accordingly.  It is advised that anytime
 *            one needs to change a constructor argument that they write down previous records and start fresh.
 * 
 * @section ProtoSpec Protocol Specification
 *    1.  Maximum bytes per packet communication:           512 bytes due to SecNet encoding.
 *        - The protocol is designed to perform segmentation when RadioPktMaxBytes is set to less than the number of payload bytes required.
 *        - This limit will affect maximum devices per node.  All information of a node must fit within that 512 byte limit.
 *        - For Example:  Using NameFieldBytes = 10
 *          - DITINFO header requires 6-bytes.
 *          - NodeName would require 10-bytes + 1('\0') = 11 bytes. (17 Total so-far)
 *          - Each Device would require 3(Type,Attr,UID) + 10-Name + 1('\0') = 14 bytes.
 *          - Therefore to stay under 512 bytes:  35 Maximum devices could exist on each node.
 *          - Check.  35-devices at 14-bytes/ea + 17 = 507 byte payload.
 *    2.  Number of possible SecNet code combinations:      128 unique (0x00 to 0x7F)
 *    3.  Because the protocol uses name termination for DITINFO information it allows communication of two differently set `NameFieldBytes`.
 *        - Example:  One node uses 20-character names and one node uses 10-character names; the two can still communicate.
 *        - The 10-character NameField node will only load 10-characters of the 20-character NameField node.
 *
 *  
 * <div style="margin-left: 40px;">
 * <h3>Protocol Standard Bytes Table</h3>
 * <table class="doxtable">
 * <tr><th colspan="2"></th><th colspan="9" style="text-align:center;">PACKET TYPE</th></tr>
 * <tr><th>Idx</th><th>Byte Order  </th><th>REQDITINFO</th><th>REQVALS</th><th>REQVAL </th><th>REQNONCE </th><th>SETVAL </th><th>VAL</th><th>NONCERSP </th><th>DITINFO</th><th>VALS</th></tr>
 * <tr><td>0  </td><td>PKB_TYPE    </td><td>X         </td><td>X      </td><td>X      </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>X      </td><td>X</td></tr>
 * <tr><td>1  </td><td>PKB_SECH    </td><td>X         </td><td>X      </td><td>X      </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>X      </td><td>X</td></tr>
 * <tr><td>2  </td><td>PKB_SECL    </td><td>X         </td><td>X      </td><td>X      </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>X      </td><td>X</td></tr>
 * <tr><td>3  </td><td>PKB_FROM_RFH</td><td>X         </td><td>X      </td><td>X      </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>X      </td><td>X</td></tr>
 * <tr><td>4  </td><td>PKB_FROM_RFL</td><td>X         </td><td>X      </td><td>X      </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>X      </td><td>X</td></tr>
 * <tr><td>5  </td><td>PKB_DITVER  </td><td>          </td><td>X      </td><td>X      </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>X      </td><td>X</td></tr>
 * <tr><td>6  </td><td>PKB_DEVUID  </td><td>          </td><td>       </td><td>X      </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>(X*)   </td><td>(X*)</td></tr>
 * <tr><td>7  </td><td>PKB_VALUEH  </td><td>          </td><td>       </td><td>       </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>       </td><td></td></tr>
 * <tr><td>8  </td><td>PKB_VALUEL  </td><td>          </td><td>       </td><td>       </td><td>X        </td><td>X      </td><td>X  </td><td>X        </td><td>       </td><td></td></tr>
 * </table>
 * <p></p>
 * (X*) = XDATA expansion to required payload; max allowed 512-bytes.</i></p>
 * <p></p>
 * <table>
 * <tr>
 * <td style="vertical-align: top; border: none; padding-right: 20px;">
 * <table>
 * <tr><th colspan="2">XDATA on DITINFO</th></tr>
 * <tr><th>Byte</th><th>Field</th></tr>
 * <tr><td>6</td><td>NodeName0</td></tr>
 * <tr><td>7</td><td>NodeName1</td></tr>
 * <tr><td>8</td><td>NodeName2</td></tr>
 * <tr><td>9</td><td>NodeName3</td></tr>
 * <tr><td>10</td><td>NodeName4</td></tr>
 * <tr><td>...</td><td>... until '\0'</td></tr>
 * <tr><td>?0</td><td>DevType</td></tr>
 * <tr><td>?1</td><td>DevAttr</td></tr>
 * <tr><td>?2</td><td>DevUID</td></tr>
 * <tr><td>?3</td><td>DevName0</td></tr>
 * <tr><td>...</td><td>... until '\0'</td></tr>
 * <tr><td>?0</td><td>DevType</td></tr>
 * <tr><td>?1</td><td>DevAttr</td></tr>
 * <tr><td>?2</td><td>DevUID</td></tr>
 * <tr><td>?3</td><td>DevName0</td></tr>
 * <tr><td>...</td><td>... until '\0'</td></tr>
 * <tr><th colspan="2">etc, etc...</th></tr>
 * </table>
 * </td>
 * <td style="vertical-align: top; border: none;">
 * <table class="doxtable">
 * <tr><th colspan="2">XDATA on VALS</th></tr>
 * <tr><th>Byte</th><th>Field</th></tr>
 * <tr><td>6</td><td>DevUID</td></tr>
 * <tr><td>7</td><td>ValueH</td></tr>
 * <tr><td>8</td><td>ValueL</td></tr>
 * <tr><td>9</td><td>DevUID</td></tr>
 * <tr><td>10</td><td>ValueH</td></tr>
 * <tr><td>11</td><td>ValueL</td></tr>
 * <tr><td>...</td><td>...</td></tr>
 * </table>
 * </td>
 * </tr>
 * </table>
 * </div>
 * <p></p>
 * @section Files Files & Abreviations
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
 */