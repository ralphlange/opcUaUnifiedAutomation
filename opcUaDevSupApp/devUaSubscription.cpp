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

#include <uasubscription.h>
#include <uasession.h>
#include <uagenericunionvalue.h>

#include <epicsTypes.h>
#include <epicsPrint.h>
#include <epicsExport.h>
#include <epicsTime.h>
#include <dbScan.h>

#include "devOpcUa.h"
#include "drvOpcUa.h"
#include "devUaSubscription.h"

// Configurable default for publishing interval

static double drvOpcua_DefaultPublishInterval = 100.0;  // ms

extern "C" {
    epicsExportAddress(double, drvOpcua_DefaultPublishInterval);
}

DevUaSubscription::DevUaSubscription(int debug=0)
    : debug(debug)
{}

DevUaSubscription::~DevUaSubscription()
{
    deleteSubscription();
}

void DevUaSubscription::subscriptionStatusChanged(
    OpcUa_UInt32      clientSubscriptionHandle,
    const UaStatus&   status)
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle); // We use the callback only for this subscription
    errlogPrintf("DevUaSubscription: subscription no longer valid - failed with status %d (%s)\n",
                 status.statusCode(),
                 status.toString().toUtf8());
}

void DevUaSubscription::distributeData(const UaVariant &value, const epicsTimeStamp ts, OPCUA_ItemINFO *pinfo, const int debug) {
    if (value.type() == OpcUaType_ExtensionObject) {
        UaExtensionObject extensionObject;
        value.toExtensionObject(extensionObject);

        // Try to get the structure definition from the dictionary
        UaStructureDefinition definition = m_pSession->structureDefinition(extensionObject.encodingTypeId());
        if (!definition.isNull()) {
            if (!definition.isUnion()) {
                // Decode the ExtensionObject to a UaGenericValue to provide access to the structure fields
                UaGenericStructureValue genericValue;
                genericValue.setGenericValue(extensionObject, definition);

                for (int i = 0; i < definition.childrenCount(); i++) {
                    if (genericValue.value(i).type() == OpcUaType_ExtensionObject) {
                        errlogPrintf("element %d: nested structures not supported\n", i);
                    } else {
                        if (strcmp(definition.child(i).name().toUtf8(), pinfo->elementName) == 0) {
                            epicsMutexLock(pinfo->lock);
                            pinfo->itemDataType = genericValue.value(i).type();
                            setRecVal(genericValue.value(i), pinfo, maxDebug(debug, pinfo->debug)); //FIXME: Buffer locally!
                            if (pinfo->prec->tse == epicsTimeEventDeviceTime)
                                pinfo->prec->time = ts;
                            if (debug > 3) errlogPrintf("Wrote data into %-20s (info %p)\n",pinfo->prec->name,pinfo);
                            epicsMutexUnlock(pinfo->lock);
                        }
                    }
                }
            } else {
                // union
                // Decode the ExtensionObject to a UaGenericUnionValue to provide access to the structure fields
                UaGenericUnionValue genericValue;
                genericValue.setGenericUnion(extensionObject, definition);

                int switchValue = genericValue.switchValue();
                if (switchValue == 0) {
                    errlogPrintf("union value not set\n");
                } else {
                    if (genericValue.value().type() == OpcUaType_ExtensionObject) {
                        errlogPrintf("union: nested structures not supported\n");
                    } else {
                        epicsMutexLock(pinfo->lock);
                        pinfo->itemDataType = genericValue.value().type();
                        setRecVal(genericValue.value(), pinfo, maxDebug(debug, pinfo->debug)); //FIXME: Buffer locally!
                        if (pinfo->prec->tse == epicsTimeEventDeviceTime)
                            pinfo->prec->time = ts;
                        if (debug > 3) errlogPrintf("Wrote data into %-20s (info %p)\n",pinfo->prec->name,pinfo);
                        epicsMutexUnlock(pinfo->lock);
                    }
                }
            }
        } else
            errlogPrintf("Cannot get a structure definition for %s - check access to type dictionary\n",
                         extensionObject.dataTypeId().toString().toUtf8());
    } else {
        if (value.isArray() != pinfo->isArray) {
             if (debug)
                 errlogPrintf("DevUaClient::readAndSetItems() %s data type mismatch: %s OPCUA data for %s record\n",
                              pinfo->prec->name, value.isArray() ? "array" : "scalar",
                              pinfo->isArray ? "array" : "scalar");
        } else {
            epicsMutexLock(pinfo->lock);
            pinfo->itemDataType = value.type();
            setRecVal(value, pinfo, maxDebug(debug, pinfo->debug)); //FIXME: Buffer locally!
            if (pinfo->prec->tse == epicsTimeEventDeviceTime)
                pinfo->prec->time = ts;
            if (debug > 3) errlogPrintf("Wrote data into %-20s (info %p)\n",pinfo->prec->name,pinfo);
            epicsMutexUnlock(pinfo->lock);
        }
    }
}

void DevUaSubscription::dataChange(
    OpcUa_UInt32               clientSubscriptionHandle,
    const UaDataNotifications& dataNotifications,
    const UaDiagnosticInfos&   diagnosticInfos)
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle); // We use the callback only for this subscription
    OpcUa_ReferenceParameter(diagnosticInfos);
    OpcUa_UInt32 i = 0;
    char timeBuf[30];
    getTime(timeBuf);

    if (debug > 2) errlogPrintf("dataChange %s\n",timeBuf);
    for (i = 0; i < dataNotifications.length(); i++)
    {
        OPCUA_MonitoredItem* pitem = m_monitoredItems->at(dataNotifications[i].ClientHandle);
        if (debug > 3)
            errlogPrintf("\t%s\n", pitem->itemPath);

        if (OpcUa_IsBad(dataNotifications[i].Value.StatusCode)) {
            if(debug) errlogPrintf("%s %s dataChange FAILED with status %s, Handle=%d\n",timeBuf,pitem->itemPath,
                                   UaStatus(dataNotifications[i].Value.StatusCode).toString().toUtf8(),dataNotifications[i].ClientHandle);
            pitem->stat = 1;
        } else
            pitem->stat = 0;

        UaVariant tempValue(dataNotifications[i].Value.Value);
        OPCUA_ItemINFO *pinfo;
        epicsTimeStamp ts;

        UaDateTime dt(dataNotifications[i].Value.ServerTimestamp); //FIXME: Make configurable
        ts.secPastEpoch = dt.toTime_t() - POSIX_TIME_AT_EPICS_EPOCH;
        ts.nsec         = dt.msec()*1000000L; // msec is 100ns steps

        if (ellCount(&pitem->inItems)) {
            for (pinfo = (OPCUA_ItemINFO *) ellFirst(&pitem->inItems);
                 pinfo;
                 pinfo = (OPCUA_ItemINFO *) ellNext(&pinfo->node))
                distributeData(tempValue, ts, pinfo, debug);
            scanIoRequest(pitem->ioscanpvt);
        }
        if (pitem->outItem) {
            pinfo = (OPCUA_ItemINFO *) pitem->outItem;
            distributeData(tempValue, ts, pinfo, debug);
            callbackRequest(&pitem->callback);
        }
    }
}

void DevUaSubscription::newEvents(
    OpcUa_UInt32                clientSubscriptionHandle,
    UaEventFieldLists&          eventFieldList)
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle);
    OpcUa_ReferenceParameter(eventFieldList);
    if(debug) errlogPrintf("DevUaSubscription::newEvents called\n");
}

UaStatus DevUaSubscription::createSubscription(UaSession *pSession)
{
    m_pSession = pSession;

    UaStatus result;
    ServiceSettings serviceSettings;
    SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval = drvOpcua_DefaultPublishInterval;
    if(debug) errlogPrintf("Creating subscription\n");
    result = pSession->createSubscription(
        serviceSettings,
        this,
        1,
        subscriptionSettings,
        OpcUa_True,
        &m_pSubscription);
    if (result.isBad())
    {
        errlogPrintf("DevUaSubscription::createSubscription failed with status %#8x (%s)\n",
                     result.statusCode(),
                     result.toString().toUtf8());
    }
    return result;
}

UaStatus DevUaSubscription::deleteSubscription()
{
    UaStatus result;
    ServiceSettings serviceSettings;
    // let the SDK cleanup the resources for the existing subscription
    if(debug) errlogPrintf("Deleting subscription\n");
    result = m_pSession->deleteSubscription(
        serviceSettings,
        &m_pSubscription);
    if (result.isBad())
    {
        errlogPrintf("DevUaSubscription::deleteSubscription failed with status %#8x (%s)\n",
                     result.statusCode(),
                     result.toString().toUtf8());
    }
    //TODO: setting the pointer NULL if delete failed might be a memory leak?
    return result;
}

UaStatus DevUaSubscription::createMonitoredItems(std::vector<UaNodeId> &vUaNodeId, std::vector<OPCUA_MonitoredItem *> *m_vectorUaItems)
{
    if(debug) errlogPrintf("DevUaSubscription::createMonitoredItems\n");
    if( m_vectorUaItems->size() == vUaNodeId.size())
        m_monitoredItems = m_vectorUaItems;
    else
    {
        errlogPrintf("\nDevUaSubscription::createMonitoredItems Error: Nr of uaItems %i != nr of browsepathItems %i\n",(int)m_vectorUaItems->size(),(int)vUaNodeId.size());
        return OpcUa_BadInvalidState;
    }
    if(false == m_pSession->isConnected() ) {
        errlogPrintf("\nDevUaSubscription::createMonitoredItems Error: session not connected\n");
        return OpcUa_BadInvalidState;

    }

    UaStatus result;
    OpcUa_UInt32 i;
    ServiceSettings serviceSettings;
    UaMonitoredItemCreateRequests itemsToCreate;
    UaMonitoredItemCreateResults createResults;
    OPCUA_MonitoredItem *pitem;
    OPCUA_ItemINFO *pinfo;
    // Configure one item to add to subscription
    // We monitor the value of the ServerStatus -> CurrentTime
    itemsToCreate.create(vUaNodeId.size());
    for(i=0; i<vUaNodeId.size(); i++) {
        pitem = m_vectorUaItems->at(i);
        pinfo = ellCount(&pitem->inItems) ? (OPCUA_ItemINFO *) ellFirst(&pitem->inItems) : pitem->outItem;
        if ( !vUaNodeId[i].isNull() ) {
            vUaNodeId[i].copyTo(&(itemsToCreate[i].ItemToMonitor.NodeId));
            itemsToCreate[i].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
            itemsToCreate[i].RequestedParameters.ClientHandle = i;
            //FIXME: Next three should be in the item instead
            itemsToCreate[i].RequestedParameters.SamplingInterval = pinfo->samplingInterval;
            itemsToCreate[i].RequestedParameters.QueueSize = pinfo->queueSize;
            itemsToCreate[i].RequestedParameters.DiscardOldest = (pinfo->discardOldest ? OpcUa_True : OpcUa_False);
            itemsToCreate[i].MonitoringMode = OpcUa_MonitoringMode_Reporting;
        }
        else {
            errlogPrintf("Skip illegal node: %s\n",pitem->itemPath);
        }
    }
    if(debug) errlogPrintf("\nAdd monitored items to subscription ...\n");
    result = m_pSubscription->createMonitoredItems(
        serviceSettings,
        OpcUa_TimestampsToReturn_Both,
        itemsToCreate,
        createResults);
    if (result.isGood())
    {
        // check individual results
        for (i = 0; i < createResults.length(); i++)
        {
            if (OpcUa_IsGood(createResults[i].StatusCode))
            {
                if(debug>1) errlogPrintf("%4d: %s\n",i,
                    UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8());
            }
            else
            {
                if(debug) {
                    errlogPrintf("DevUaSubscription::createMonitoredItems failed for node: %s - Status %s\n",
                        UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8(),
                        UaStatus(createResults[i].StatusCode).toString().toUtf8());
                }
            }
        }
    }
    else
    {
       if(debug)  errlogPrintf("DevUaSubscription::createMonitoredItems service call failed with status %s\n", result.toString().toUtf8());
    }
    return result;
}
