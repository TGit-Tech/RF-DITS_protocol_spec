/*******************************************************************************************************************//**
 * @file DB.h
 * @brief Debug Pre-Compiler Constants
 **********************************************************************************************************************/
#ifndef _DB_H
#define _DB_H

/*********************************************//**
 * @defgroup DB Debug Directives
 * @brief    DBERROR | DBINIT | DBENTER | DBINFO.
 *         - Pre-Compiler Directives for Debug Messaging.
 *         - Each-Type (ERROR,INIT,ENTER,INFO) allows (1 or 2) [A]rgument Variables.
 *         - Tailing [L] determines 'on new line'
 *         - Message-Type output is '!ERROR!', '@'Init, '...'Info, '>'Enter
 *         - NO trailing ';' is needed.
 * @code{.c} 
 *           DBERROR(("Error Message - no new line"))
 *           DBERRORL(("Error Message - new line"))
 *           DBERRORAL(("Error Message"),(Variable))
 *           DBERRORAAL(("Error Message"),(Variable),(Variable))
 *           DBINITL(("Object Initialization Message"))
 *           DBINFOL(("General Information"))
 *           DBENTER(("Enter Function"))
 * @endcode
 * @{
 ************************************************/
#define DEBUGLEVEL         0   ///< [0]=NONE, [1]=ERROR, [2]=INIT, [3]=INFO, [4]=ENTER
//----------------------------------------------------------------------------------
// [D]E[B]UG Macros  DBERROR, DBINIT, DBINFO, DBENTER   A-Arg L-Line
//----------------------------------------------------------------------------------
#define COMMA ,
#if DEBUGLEVEL>0 // DEBUG Level-1: Include ERROR
#define DBERRORAAL(x,y,z) Serial.print(F("!ERROR!-"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.println(F(")"));
#define DBERRORAA(x,y,z) Serial.print(F("!ERROR!-"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.print(F(")"));
#define DBERRORAL(x,y) Serial.print(F("!ERROR!-"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.println(F(")"));
#define DBERRORA(x,y) Serial.print(F("!ERROR!-"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(")"));
#define DBERRORL(x) Serial.print(F("!ERROR!-"));Serial.println((F(x)));
#define DBERROR(x) Serial.print(F("!ERROR!-"));Serial.print((F(x)));
#else
#define DBERRORAAL(x,y,z)
#define DBERRORAA(x,y,z)
#define DBERRORAL(x,y)
#define DBERRORA(x,y)
#define DBERROR(x)
#define DBERRORL(x)
#endif

#if DEBUGLEVEL>1 // DEBUG Level-2; Constructors
#define DBINITAAL(x,y,z) Serial.print(F("@"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.println(F(")"));
#define DBINITAA(x,y,z) Serial.print(F("@"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.print(F(")"));
#define DBINITAL(x,y) Serial.print(F("@"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.println(F(")"));
#define DBINITA(x,y) Serial.print(F("@"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(")"));
#define DBINIT(x) Serial.print(F("@"));Serial.print((F(x)));
#define DBINITL(x) Serial.print(F("@"));Serial.println((F(x)));
#else
#define DBINITAAL(x,y,z)
#define DBINITAA(x,y,z)
#define DBINITAL(x,y)
#define DBINITA(x,y)
#define DBINIT(x)
#define DBINITL(x)
#endif

#if DEBUGLEVEL>2 // DEBUG Level-3; Include INFO
#define DBINFOAAL(x,y,z) Serial.print(F("..."));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.println(F(")"));
#define DBINFOAA(x,y,z) Serial.print(F("..."));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.print(F(")"));
#define DBINFOAL(x,y) Serial.print(F("..."));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.println(F(")"));
#define DBINFOA(x,y) Serial.print(F("..."));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(")"));
#define DBINFOL(x) Serial.print(F("..."));Serial.println((F(x)));
#define DBINFO(x) Serial.print(F("..."));Serial.print((F(x)));
#else
#define DBINFOAAL(x,y,z)
#define DBINFOAA(x,y,z)
#define DBINFOAL(x,y)
#define DBINFOA(x,y)
#define DBINFOL(x)
#define DBINFO(x)
#endif

#if DEBUGLEVEL>3 // DEBUG Level-4; Include ENTER
#define DBENTERAAL(x,y,z) Serial.print(F(">"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.println(F(")"));
#define DBENTERAA(x,y,z) Serial.print(F(">"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(","));Serial.print z;Serial.print(F(")"));
#define DBENTERAL(x,y) Serial.print(F(">"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.println(F(")"));
#define DBENTERA(x,y) Serial.print(F(">"));Serial.print((F(x)));Serial.print(F("("));Serial.print y;Serial.print(F(")"));
#define DBENTERL(x) Serial.print(F(">"));Serial.println((F(x)));
#define DBENTER(x) Serial.print(F(">"));Serial.print((F(x)));
#else
#define DBENTERAAL(x,y,z)
#define DBENTERAA(x,y,z)
#define DBENTERAL(x,y)
#define DBENTERA(x,y)
#define DBENTERL(x)
#define DBENTER(x)
#endif
///@}
//_____________________________________________________________________________________________________________________
#endif
