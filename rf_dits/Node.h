/******************************************************************************************************************//**
 * @class   Node
 * @brief   **LIST** represents a Physical Node with a list of Device (s)
 *********************************************************************************************************************/
class Node {
  public:
    Node(byte _TodeIndex);                                      ///< Constructor Index(0-9) eqv-to. EEPROM Address of
    
    virtual const char*   Title() override;                     ///< Points to TodeName->Name()
    virtual void          Update() override;                    ///< Update the Node Information
    

    virtual MenuItem*     NewDevice(byte _DTKey);               ///< Creates & Adds a New Device
    virtual MenuItem*     AddDevice(byte _DTKey, byte _RFID, bool _NewDevice=false);   ///< Menu add Device using \ref KEY
    virtual void          DelDevice(MenuItem* _Item);           ///< Delete a Device
    virtual void          EELoadDevices();
    
    virtual unsigned int  RFAddr();                             ///< GET RF-Address stored in EEPROM
    virtual void          RFAddr(unsigned int _RFAddr);         ///< SET RF-Address stored in EEPROM
    virtual byte          Version();                            ///< GET Node's Configuration Ver
    virtual void          Version(byte _Version);               ///< SET Node's Configuration Ver
    virtual bool          IsLocal();                            ///< GET (TodeIndex == 0) Set in the Constructor
    virtual int           EEAddress();                          ///< GET EEPROM Address of This Node (Calc by TodeIndex)

    MenuTodeName*         TodeName=0;                           ///< TodeName Menu-Item
    Device*               Devices[AEB_MAXDEVICES] = {0};        ///< Device Pointers - Index is RFID
    //HdwSelect*            Hardware = 0;                         ///< This Node's Hardware Attached Menu-Item
    
    
  protected:
    
    
  private:
    bool            bIsLocal = false;
    byte            yVersion = 0;

};