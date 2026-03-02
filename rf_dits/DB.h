/*******************************************************************************************************************//**
 * @file DB.h
 * @brief Debug Pre-Compiler Constants
 **********************************************************************************************************************/
#ifndef _DB_H
#define _DB_H

/*********************************************//**
 * @defgroup DB Debug Directives
 * @brief    DBERROR | DBENTER | DBINFO.
 *         - Pre-Compiler Directives for Debug Messaging.
 *         - Each-Type (ERROR,INFO) allows (1 or 2) [A]rgument Variables.
 *         - Use '\n' in debug message for new-line
 *         - Message-Type output is '!ERROR!', '...'Info
 *         - **NO** trailing ';' in source code.
 * @code{.c} 
 *          DBERROR(("no new line"))
 *          DBERROR(("new line \n"))
 *          DBAERROR((Variable),("Error Message"))
 *          DBAAERROR((Variable),(Variable),("Error Message"))
 *          DBAAINFO((Variable),(Variable),("Info Message with new line \n"))
 * @endcode
 * @{
 ************************************************/
// ON/OFF Importance-Level debug (All 0 for release; Code-Section won't matter.)
#define DB_ERROR       1
#define DB_ENTER       1
#define DB_INFO        1

// ON/OFF Code-Sections
// 0=All Importance-Levels OFF in this code-section.
// 1=Active; Importance-levels as assigned above.
#define DB_GEN         1
#define DB_DIT         1
#define DB_RF          1

#define SERLINEZ(z) Serial.print((F(z)));
#define SERLINEYZ(y,z) Serial.print y;Serial.print(F(")"));SERLINEZ(z)
#define SERLINEXYZ(x,y,z) Serial.print x;Serial.print(F(","));SERLINEYZ(y,z)
#define SERLINEWXYZ(w,x,y,z) Serial.print w;Serial.print(F(","));SERLINEXYZ(x,y,z)
//----------------------------------------------------------------------------------
// GEN Code-Section DBERROR, DBENTER, DBINFO Macros
//----------------------------------------------------------------------------------
#define COMMA ,
#if (DB_ERROR && DB_GEN)
#define DBAAAERROR(w,x,y,z) Serial.print(F("!ERROR! ("));SERLINEWXYZ(w,x,y,z)
#define DBAAERROR(x,y,z) Serial.print(F("!ERROR! ("));SERLINEXYZ(x,y,z)
#define DBAERROR(y,z) Serial.print(F("!ERROR! ("));SERLINEYZ(y,z)
#define DBERROR(z) Serial.print(F("!ERROR! "));SERLINEZ(z)
#define DBERRORSP(z) Serial.print z;
#else
#define DBAAAERROR(w,x,y,z)
#define DBAAERROR(x,y,z)
#define DBAERROR(y,z)
#define DBERROR(z)
#define DBERRORSP(z)
#endif

#if (DB_ENTER && DB_GEN)
#define DBAAAENTER(w,x,y,z) Serial.print(F("__> ("));SERLINEWXYZ(w,x,y,z)
#define DBAAENTER(x,y,z) Serial.print(F("__> ("));SERLINEXYZ(x,y,z)
#define DBAENTER(y,z) Serial.print(F("__> ("));SERLINEYZ(y,z)
#define DBENTER(z) Serial.print(F("__> "));SERLINEZ(z)
#define DBENTERSP(z) Serial.print z;
#else
#define DBAAAENTER(w,x,y,z)
#define DBAAENTER(x,y,z)
#define DBAENTER(y,z)
#define DBENTER(z)
#define DBENTERSP(z)
#endif

#if (DB_INFO && DB_GEN)
#define DBAAAINFO(w,x,y,z) Serial.print(F("...("));SERLINEWXYZ(w,x,y,z)
#define DBAAINFO(x,y,z) Serial.print(F("...("));SERLINEXYZ(x,y,z)
#define DBAINFO(y,z) Serial.print(F("...("));SERLINEYZ(y,z)
#define DBINFO(z) Serial.print(F("... "));SERLINEZ(z)
#define DBINFOSP(z) Serial.print z;
#else
#define DBAAAINFO(w,x,y,z)
#define DBAAINFO(x,y,z)
#define DBAINFO(y,z)
#define DBINFO(z)
#define DBINFOSP(z)
#endif
//----------------------------------------------------------------------------------
// DIT Code-Section DBDITERROR, DBDITENTER, DBDITINFO Macros
//----------------------------------------------------------------------------------
#if (DB_ERROR && DB_DIT)
#define DBDITAAAERROR(w,x,y,z) Serial.print(F("!ERROR!DIT ("));SERLINEWXYZ(w,x,y,z)
#define DBDITAAERROR(x,y,z) Serial.print(F("!ERROR!DIT ("));SERLINEXYZ(x,y,z)
#define DBDITAERROR(y,z) Serial.print(F("!ERROR!DIT ("));SERLINEYZ(y,z)
#define DBDITERROR(z) Serial.print(F("!ERROR!DIT "));SERLINEZ(z)
#define DBDITERRORSP(z) Serial.print z;
#else
#define DBDITAAAERROR(w,x,y,z)
#define DBDITAAERROR(x,y,z)
#define DBDITAERROR(y,z)
#define DBDITERROR(z)
#define DBDITERRORSP(z)
#endif

#if (DB_ENTER && DB_DIT)
#define DBDITAAAENTER(w,x,y,z) Serial.print(F("__DIT> ("));SERLINEWXYZ(w,x,y,z)
#define DBDITAAENTER(x,y,z) Serial.print(F("__DIT> ("));SERLINEXYZ(x,y,z)
#define DBDITAENTER(y,z) Serial.print(F("__DIT> ("));SERLINEYZ(y,z)
#define DBDITENTER(z) Serial.print(F("__DIT> "));SERLINEZ(z)
#define DBDITENTERSP(z) Serial.print z;
#else
#define DBDITAAAENTER(w,x,y,z)
#define DBDITAAENTER(x,y,z)
#define DBDITAENTER(y,z)
#define DBDITENTER(z)
#define DBDITENTERSP(z)
#endif

#if (DB_INFO && DB_DIT)
#define DBDITAAAINFO(w,x,y,z) Serial.print(F("...DIT("));SERLINEWXYZ(w,x,y,z)
#define DBDITAAINFO(x,y,z) Serial.print(F("...DIT("));SERLINEXYZ(x,y,z)
#define DBDITAINFO(y,z) Serial.print(F("...DIT("));SERLINEYZ(y,z)
#define DBDITINFO(z) Serial.print(F("...DIT "));SERLINEZ(z)
#define DBDITINFOSP(z) Serial.print z;
#else
#define DBDITAAAINFO(w,x,y,z)
#define DBDITAAINFO(x,y,z)
#define DBDITAINFO(y,z)
#define DBDITINFO(z)
#define DBDITINFOSP(z)
#endif
//----------------------------------------------------------------------------------
// RF Code-Section DBRFERROR, DBRFENTER, DBRFINFO Macros
//----------------------------------------------------------------------------------
#if (DB_ERROR && DB_RF)
#define DBRFAAAERROR(w,x,y,z) Serial.print(F("!ERROR!RF ("));SERLINEWXYZ(w,x,y,z)
#define DBRFAAERROR(x,y,z) Serial.print(F("!ERROR!RF ("));SERLINEXYZ(x,y,z)
#define DBRFAERROR(y,z) Serial.print(F("!ERROR!RF ("));SERLINEYZ(y,z)
#define DBRFERROR(z) Serial.print(F("!ERROR!RF "));SERLINEZ(z)
#define DBRFERRORSP(z) Serial.print z;
#else
#define DBRFAAAERROR(w,x,y,z)
#define DBRFAAERROR(x,y,z)
#define DBRFAERROR(y,z)
#define DBRFERROR(z)
#define DBRFERRORSP(z)
#endif

#if (DB_ENTER && DB_RF)
#define DBRFAAAENTER(w,x,y,z) Serial.print(F("__RF> ("));SERLINEWXYZ(w,x,y,z)
#define DBRFAAENTER(x,y,z) Serial.print(F("__RF> ("));SERLINEXYZ(x,y,z)
#define DBRFAENTER(y,z) Serial.print(F("__RF> ("));SERLINEYZ(y,z)
#define DBRFENTER(z) Serial.print(F("__RF> "));SERLINEZ(z)
#define DBRFENTERSP(z) Serial.print z;
#else
#define DBRFAAAENTER(w,x,y,z)
#define DBRFAAENTER(x,y,z)
#define DBRFAENTER(y,z)
#define DBRFENTER(z)
#define DBRFENTERSP(z)
#endif

#if (DB_INFO && DB_RF)
#define DBRFAAAINFO(w,x,y,z) Serial.print(F("...RF("));SERLINEWXYZ(w,x,y,z)
#define DBRFAAINFO(x,y,z) Serial.print(F("...RF("));SERLINEXYZ(x,y,z)
#define DBRFAINFO(y,z) Serial.print(F("...RF("));SERLINEYZ(y,z)
#define DBRFINFO(z) Serial.print(F("...RF "));SERLINEZ(z)
#define DBRFINFOSP(z) Serial.print z;
#else
#define DBRFAAAINFO(w,x,y,z)
#define DBRFAAINFO(x,y,z)
#define DBRFAINFO(y,z)
#define DBRFINFO(z)
#define DBRFINFOSP(z)
#endif
///@}
//_____________________________________________________________________________________________________________________
#endif
