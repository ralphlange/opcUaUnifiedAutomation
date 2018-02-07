/*************************************************************************\
* Copyright (c) 2016 Helmholtz-Zentrum Berlin
*     fuer Materialien und Energie GmbH (HZB), Berlin, Germany.
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU Lesser General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
\*************************************************************************/
#ifndef INCdevOpcUaH
#define INCdevOpcUaH

#include <dbScan.h>
#include <callback.h>
#include <epicsTypes.h>
#include <epicsMutex.h>
#include <ellLib.h>

#define ANY_VAL_STRING_SIZE 80
typedef union {                     /* A subset of the built in types we use */
        epicsInt32   Int32;
        epicsUInt32  UInt32;
        epicsFloat64 Double;
//        char        *cString;     /* need a buffer to receive strings, see sampleSubscription.cpp! */
        char         cString[ANY_VAL_STRING_SIZE];   /* find max defined stringsize of base: */
} epicsAnyVal;                      // perl -ne 'print "$2 $1\n" if($_=~/char\s+([\w\d_]+)\[(\d+)\]/);' base-3.14.12.5/include/*|sort -u

struct OPCUA_MonitoredItem;

typedef struct OPCUA_ItemINFO {
    ELLNODE node;
    const char *elementName;  /* Name of structure element; NULL if top element */
    int elementIndex;         /* Index of structure element; 0 if unknown */
    struct OPCUA_MonitoredItem *mItem; /* Monitored item (parent) */
    int itemDataType;         /* OPCUA data type */

    epicsAnyVal varVal;       /* buffer to hold the value got from OPC for all scalar values, including string   */

    void *pRecVal;            /* point to records val/rval/oval field */
    epicsType recDataType;    /* Data type of the records VAL/RVAL field */

    void *pInpVal;            /* Input field to set OUT-records by the opcUa server */
    epicsType inpDataType;    /* OUT records: the type of the records input = VAL field - may differ from RVAL type!. INP records = NULL */

    epicsMutexId lock;        /* mutex for item access */

    int isArray;
    int arraySize;
    int useServerTime;        /* 1: use server timestamp; 2: use source timestamp */
                              /* OPC UA properties of the monitored item */
    double samplingInterval;   /*  FIXME: these should go in the monitoredItem */
    epicsUInt32 queueSize;
    unsigned char discardOldest;

    int debug;                /* debug level of this item, defined in field REC:TPRO */
    int flagSuppressWrite;    /* flag for OUT-records: prevent write back of incomming values */

    dbCommon *prec;
} OPCUA_ItemINFO;

typedef struct OPCUA_MonitoredItem {
    const char *itemPath;     /* OPCUA item name */
    unsigned int nodeIndex;   /* OPCUA node index */
    int stat;                 /* Status of the OPC connection */

    epicsTimeStamp tsSrv;     /* Server time stamp */
    epicsTimeStamp tsSrc;     /* Source time stamp */

    IOSCANPVT ioscanpvt;      /* Input records' scan request */
    CALLBACK callback;        /* Output records' callback request */
    ELLLIST inItems;          /* List of connected input items */
    OPCUA_ItemINFO *outItem;  /* Connected output item */
} OPCUA_MonitoredItem;

#ifdef __cplusplus
extern "C" {
#endif
OPCUA_ItemINFO *getHead();
void setHead(OPCUA_ItemINFO *);
#ifdef __cplusplus
}
#endif

#endif
