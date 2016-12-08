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
// standard header
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <iostream>
#include <signal.h>

// EPICS LIBS
#define epicsTypesGLOBAL
#include <epicsTypes.h>
#include <epicsPrint.h>
#include <epicsExport.h>
#include <registryFunction.h>
#include <dbCommon.h>
#include <devSup.h>
#include <drvSup.h>
#include <devLib.h>
#include <iocsh.h>

// toolbox header
#include "uaplatformlayer.h"
#include "uabase.h"
#include "uaclientsdk.h"
#include "uasession.h"
#include "drvOpcUa.h"
#include "devUaSubscription.h"

using namespace UaClientSdk;

const char *variantTypeStrings(int type)
{
    switch(type) {
        case 0:  return "OpcUa_Null";
        case 1:  return "OpcUa_Boolean";
        case 2:  return "OpcUa_SByte";
        case 3:  return "OpcUa_Byte";
        case 4:  return "OpcUa_Int16";
        case 5:  return "OpcUa_UInt16";
        case 6:  return "OpcUa_Int32";
        case 7:  return "OpcUa_UInt32";
        case 8:  return "OpcUa_Int64";
        case 9:  return "OpcUa_UInt64";
        case 10: return "OpcUa_Float";
        case 11: return "OpcUa_Double";
        case 12: return "OpcUa_String";
        case 13: return "OpcUa_DateTime";
        case 14: return "OpcUa_Guid";
        case 15: return "OpcUa_ByteString";
        case 16: return "OpcUa_XmlElement";
        case 17: return "OpcUa_NodeId";
        case 18: return "OpcUa_ExpandedNodeId";
        case 19: return "OpcUa_StatusCode";
        case 20: return "OpcUa_QualifiedName";
        case 21: return "OpcUa_LocalizedText";
        case 22: return "OpcUa_ExtensionObject";
        case 23: return "OpcUa_DataValue";
        case 24: return "OpcUa_Variant";
        case 25: return "OpcUa_DiagnosticInfo";
        default: return "Illegal Value";
    }
}

//inline int64_t getMsec(DateTime dateTime){ return (dateTime.Value % 10000000LL)/10000; }

class DevUaClient : public UaSessionCallback
{
    UA_DISABLE_COPY(DevUaClient);
public:
    DevUaClient();
    virtual ~DevUaClient();

    // UaSessionCallback implementation ----------------------------------------------------
    virtual void connectionStatusChanged(OpcUa_UInt32 clientConnectionId, UaClient::ServerStatus serverStatus);
    // UaSessionCallback implementation ------------------------------------------------------

    UaString applicationCertificate;
    UaString applicationPrivateKey;

    UaStatus connect(UaString url);
    UaStatus disconnect();
    UaStatus subscribe();
    UaStatus unsubscribe();
    long getNodes();
    void setBadQuality();

    long setOPCUA_Item(OPCUA_ItemINFO *h);
    UaStatus getAllNodesFromBrowsePath();
    long getNodeFromBrowsePath(OpcUa_UInt32 bpItem);
    long getNodeFromId(OpcUa_UInt32 bpItem);
    long getAllNodesFromId();
    UaStatus readFunc(UaDataValues &values,ServiceSettings &serviceSettings,UaDiagnosticInfos &diagnosticInfos);
    UaStatus createMonitoredItems();
    UaStatus writeFunc(ServiceSettings &serviceSettings,UaWriteValues &nodesToWrite,UaStatusCodeArray &results,UaDiagnosticInfos &diagnosticInfos);
    void writeComplete(OpcUa_UInt32 transactionId,const UaStatus&result,const UaStatusCodeArray& results,const UaDiagnosticInfos& diagnosticInfos);
    void itemStat(int v);
    std::vector<UaNodeId>         vUaNodeId;
    std::vector<OPCUA_ItemINFO *> vUaItemInfo;
    GetNodeMode mode;
    int debug;
private:
    UaSession* m_pSession;
    DevUaSubscription* m_pDevUaSubscription;
    UaClient::ServerStatus connectionStatusStored;
    int needNewSubscription;
};
void printVal(UaVariant &val,OpcUa_UInt32 IdxUaItemInfo);
void print_OpcUa_DataValue(_OpcUa_DataValue *d);

// global variables

DevUaClient* pMyClient = NULL;

extern "C" {
                                    /* DRVSET */
    struct {
        long      number;
        DRVSUPFUN report;
        DRVSUPFUN init;
    }  drvOpcUa = {
        2,
        (DRVSUPFUN) opcUa_io_report,
        NULL
    };
    epicsExportAddress(drvet,drvOpcUa);


    epicsRegisterFunction(opcUa_io_report);
    long opcUa_io_report (int level) /* Write IO report output to stdout. */
    {
        pMyClient->itemStat(level);
        return 0;
    }
}

void signalHandler( int signum )
{
    exit(1);
}

DevUaClient::DevUaClient()
{
    m_pSession            = new UaSession();
    m_pDevUaSubscription = NULL;
    connectionStatusStored= UaClient::Disconnected;
    needNewSubscription   = 0;
    DevUaClient::mode = BROWSEPATH;
    debug = 0;
}

DevUaClient::~DevUaClient()
{
    if (m_pDevUaSubscription)
    {
        // delete local subscription object
        delete m_pDevUaSubscription;
        m_pDevUaSubscription = NULL;
    }
    if (m_pSession)
    {
        // disconnect if we're still connected
        if (m_pSession->isConnected() != OpcUa_False)
        {
            ServiceSettings serviceSettings;
            m_pSession->disconnect(serviceSettings, OpcUa_True);
        }
        delete m_pSession;
        m_pSession = NULL;
    }
}


void DevUaClient::connectionStatusChanged(
    OpcUa_UInt32             clientConnectionId,
    UaClient::ServerStatus   serverStatus)
{
    OpcUa_ReferenceParameter(clientConnectionId);

    switch (serverStatus)
    {
    case UaClient::Disconnected:
        printf("Connection status changed to Disconnected %d\n",serverStatus);
        connectionStatusStored = UaClient::Disconnected;
        break;
    case UaClient::Connected:
        printf("opcUaClient: Connection status changed to Connected %d\n",serverStatus);
        if(connectionStatusStored == UaClient::ConnectionErrorApiReconnect) {
            this->unsubscribe();
            this->subscribe();
            this->getNodes();
            this->createMonitoredItems();
        }
        connectionStatusStored = UaClient::Connected;
        break;
    case UaClient::ConnectionWarningWatchdogTimeout:
        printf("UaClientConnection status changed to ConnectionWarningWatchdogTimeout %d\n",serverStatus);
        connectionStatusStored = UaClient::ConnectionWarningWatchdogTimeout;
        this->setBadQuality();
        break;
    case UaClient::ConnectionErrorApiReconnect:
        printf("UaClientConnection status changed to ConnectionErrorApiReconnect %d\n",serverStatus);
        connectionStatusStored = UaClient::ConnectionErrorApiReconnect;
        this->setBadQuality();
        break;
    case UaClient::ServerShutdown:
        printf("UaClientConnection status changed to ServerShutdown %d\n",serverStatus);
        connectionStatusStored = UaClient::ServerShutdown;
        this->setBadQuality();
        break;
    case UaClient::NewSessionCreated:
        printf("opcUaClient: Connection status changed to NewSessionCreated %d\n",serverStatus);
        connectionStatusStored = UaClient::NewSessionCreated;
        break;
    }
}

void DevUaClient::setBadQuality()
{
    for(OpcUa_UInt32 bpItem=0;bpItem<vUaItemInfo.size();bpItem++) {
        OPCUA_ItemINFO *pOPCUA_ItemINFO = vUaItemInfo[bpItem];
        pOPCUA_ItemINFO->stat = 1;
        if( pOPCUA_ItemINFO->inpDataType ) { // is OUT Record
            callbackRequest(&(pOPCUA_ItemINFO->callback));
        }
        else {// is IN Record
            scanIoRequest( pOPCUA_ItemINFO->ioscanpvt );
        }
    }
}

// add OPCUA_ItemINFO to vUaItemInfo Check and seutp nodes is done by getNodes()
long DevUaClient::setOPCUA_Item(OPCUA_ItemINFO *h)
{
    vUaItemInfo.push_back(h);
    h->itemIdx = vUaItemInfo.size()-1;
    if(h->debug >= 3) printf("%s\tDevUaClient::setOPCUA_ItemINFO: idx=%lu\n",h->prec->name,vUaItemInfo.size()-1);
/*    if(! getNodeFromBrowsePath(h->itemIdx ))
        return 0;
    if(getNodeFromId(h->itemIdx) )
        return 1;
*/
    return 0;
}

UaStatus DevUaClient::connect(UaString sURL)
{
    UaStatus result;

    // Provide information about the client
    SessionConnectInfo sessionConnectInfo;
    UaString sNodeName("unknown_host");
    char szHostName[256];
    if (0 == UA_GetHostname(szHostName, 256))
    {
        sNodeName = szHostName;
    }
    sessionConnectInfo.sApplicationName = "HelmholtzgesellschaftBerlin Test Client";
    // Use the host name to generate a unique application URI
    sessionConnectInfo.sApplicationUri  = UaString("urn:%1:HelmholtzgesellschaftBerlin:TestClient").arg(sNodeName);
    sessionConnectInfo.sProductUri      = "urn:HelmholtzgesellschaftBerlin:TestClient";
    sessionConnectInfo.sSessionName     = sessionConnectInfo.sApplicationUri;


    // Security settings are not initialized - we connect without security for now
    SessionSecurityInfo sessionSecurityInfo;

    if(debug) printf("\nConnecting to %s\n", sURL.toUtf8());
    result = m_pSession->connect(sURL,sessionConnectInfo,sessionSecurityInfo,this);

    if (result.isBad())
    {
        printf("Connect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus DevUaClient::disconnect()
{
    UaStatus result;

    // Default settings like timeout
    ServiceSettings serviceSettings;

    if(debug) printf("\nDisconnecting");
    result = m_pSession->disconnect(serviceSettings,OpcUa_True);

    if (result.isBad())
    {
        printf("Disconnect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}
UaStatus DevUaClient::subscribe()
{
    m_pDevUaSubscription = new DevUaSubscription(this->debug);
    return m_pDevUaSubscription->createSubscription(m_pSession);
}

UaStatus DevUaClient::unsubscribe()
{
    return m_pDevUaSubscription->deleteSubscription();
}
//get whole bunch of nodes from browsePaths, no direcet node access
UaStatus DevUaClient::getAllNodesFromBrowsePath()
{
    UaStatus status;
    OPCUA_ItemINFO          *pOPCUA_ItemINFO;
    UaDiagnosticInfos       diagnosticInfos;
    ServiceSettings         serviceSettings;
    UaRelativePathElements  pathElements;
    UaBrowsePathResults     browsePathResults;
    UaBrowsePaths           browsePaths;
    OpcUa_UInt32            bpItem;
    OpcUa_UInt32            itemCount=vUaItemInfo.size();
    std::string             partPath;

    if(debug) printf("DevUaClient::getAllNodesFromBrowsePath mode:%d",mode);
    printf("  Show items\n"); for(bpItem=0;bpItem<itemCount;bpItem++)        printf("\t%d: %s\n",bpItem,(vUaItemInfo[bpItem])->ItemPath);
    browsePaths.create(itemCount);
    for(bpItem=0;bpItem<itemCount;bpItem++) {
        std::vector<std::string> devpath; // parsed item path
        pOPCUA_ItemINFO = vUaItemInfo[bpItem];

        OpcUa_UInt16             NdIdx;
        std::vector<std::string> itempath; // parsed item path
        char            *endptr;
        boost::split(itempath,pOPCUA_ItemINFO->ItemPath,boost::is_any_of(":"));
        if(itempath.size() != 2)
            return 1;
        NdIdx = (OpcUa_UInt16) strtol(itempath[0].c_str(),&endptr,10); // ItemPath is nodeId

        boost::split(devpath,itempath[1],boost::is_any_of("."));
        int lenPath = devpath.size();
        browsePaths[bpItem].StartingNode.Identifier.Numeric = OpcUaId_ObjectsFolder;
        pathElements.create(lenPath);
        for(int i=0; i<lenPath;i++){
            if(debug) printf("%s|",devpath[i].c_str());

            pathElements[i].IncludeSubtypes = OpcUa_True;
            pathElements[i].IsInverse       = OpcUa_False;
            pathElements[i].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
            if(mode==BROWSEPATH_CONCAT) {
                partPath = devpath[0];
                for(int j=1;j<=i;j++) partPath += "."+devpath[j];
            } else {
                partPath = devpath[i];
            }
            OpcUa_String_AttachCopy(&pathElements[i].TargetName.Name, partPath.c_str());
            pathElements[i].TargetName.NamespaceIndex = NdIdx;
        }
        browsePaths[bpItem].RelativePath.NoOfElements = pathElements.length();
        browsePaths[bpItem].RelativePath.Elements = pathElements.detach();
        if(debug) printf("\n");
    }


    status = m_pSession->translateBrowsePathsToNodeIds(
        serviceSettings, // Use default settings
        browsePaths,
        browsePathResults,
        diagnosticInfos);
    vUaNodeId.clear();
    for(OpcUa_UInt32 i=0; i<browsePathResults.length(); i++) {
        if ( OpcUa_IsGood(browsePathResults[i].StatusCode) ) {
            UaNodeId tempNode(browsePathResults[i].Targets[0].TargetId.NodeId);
            vUaNodeId.push_back(tempNode);
        }
        else {
            vUaNodeId.push_back(UaNodeId());
            if(vUaNodeId.at(i).isNull())
                printf("%s isNULL\n",(vUaItemInfo[bpItem])->ItemPath);
        }
    }
    return status;
}

long DevUaClient::getNodeFromBrowsePath(OpcUa_UInt32 bpItem)
{
    UaStatus status;
    OPCUA_ItemINFO  *pOPCUA_ItemINFO;
    OpcUa_UInt16    NdIdx;
    char            ItemPath[ITEMPATHLEN];
    char            *endptr;

    if(debug) printf("DevUaClient::getNodeFromBrowsePath\n");
    pOPCUA_ItemINFO = vUaItemInfo[bpItem];
 
    std::vector<std::string> itempath; // parsed item path
    boost::split(itempath,pOPCUA_ItemINFO->ItemPath,boost::is_any_of(":"));
    if(itempath.size() != 2)
        return 1;
    NdIdx = (OpcUa_UInt16) strtol(itempath[0].c_str(),&endptr,10); // ItemPath is nodeId
    strncpy(ItemPath,itempath[1].c_str(),ITEMPATHLEN);

    if(debug) printf("\tparsed NS:%hu PATH: %s\n",NdIdx,ItemPath);
    UaDiagnosticInfos       diagnosticInfos;
    ServiceSettings         serviceSettings;
    UaRelativePathElements  pathElements;
    UaBrowsePaths           browsePaths;
    UaBrowsePathResults     browsePathResults;
    std::string             partPath;

    browsePaths.create(1);
    std::vector<std::string> devpath; // parsed item path
    boost::split(devpath,ItemPath,boost::is_any_of("."));
    int lenPath = devpath.size();
    browsePaths[0].StartingNode.Identifier.Numeric = OpcUaId_ObjectsFolder;
    pathElements.create(lenPath);
    for(int i=0; i<lenPath;i++){
        if(debug) printf("%s|",devpath[i].c_str());

        pathElements[i].IncludeSubtypes = OpcUa_True;
        pathElements[i].IsInverse       = OpcUa_False;
        pathElements[i].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
        if(DevUaClient::mode==3) {
            partPath = devpath[0];
            for(int j=1;j<=i;j++) partPath += "."+devpath[j];
        } else {
            partPath = devpath[i];
        }
        OpcUa_String_AttachCopy(&pathElements[i].TargetName.Name, partPath.c_str());
        pathElements[i].TargetName.NamespaceIndex = NdIdx;
    }
    browsePaths[0].RelativePath.NoOfElements = pathElements.length();
    browsePaths[0].RelativePath.Elements = pathElements.detach();
    if(debug) printf("\n");

    status = m_pSession->translateBrowsePathsToNodeIds(
        serviceSettings, // Use default settings
        browsePaths,
        browsePathResults,
        diagnosticInfos);

    if(debug) printf("DONE translateBrowsePathsToNodeIds: %s len=%d\n",status.toString().toUtf8(),browsePathResults.length());

    if( (browsePathResults.length() == 1) && ( OpcUa_IsGood(browsePathResults[0].StatusCode) )) {
        UaNodeId tempNode(browsePathResults[0].Targets[0].TargetId.NodeId);
        vUaNodeId.push_back(tempNode);
    }
    else {
        vUaNodeId.push_back(UaNodeId());
        if(vUaNodeId.at(bpItem).isNull())
            printf("DevUaClient::getNodeFromBrowsePath can't find '%s'\n",(vUaItemInfo[bpItem])->ItemPath);
        return 1;

    }
    return 0;
}

long DevUaClient::getNodeFromId(OpcUa_UInt32 bpItem)
{
    UaStatus status;

    ServiceSettings     serviceSettings;
    UaDataValues        values;
    UaDiagnosticInfos   diagnosticInfos;
    UaReadValueIds      nodeToRead;
    OPCUA_ItemINFO      *pOPCUA_ItemINFO;
    UaNodeId            tempNode;
    char                *endptr;
    OpcUa_UInt16        NdIdx;
    char                ItemId[ITEMPATHLEN];

    nodeToRead.create(1);
    pOPCUA_ItemINFO = vUaItemInfo[bpItem];

    std::vector<std::string> itempath; // parsed item path
    boost::split(itempath,pOPCUA_ItemINFO->ItemPath,boost::is_any_of(","));
    if(itempath.size() != 2)
        return 1;

    NdIdx = (OpcUa_UInt16) strtol(itempath[0].c_str(),&endptr,10); // ItemPath is nodeId
    strncpy(ItemId,itempath[1].c_str(),ITEMPATHLEN);
    OpcUa_UInt32 itemId = (OpcUa_UInt32) strtol(ItemId,&endptr,10); // ItemPath is nodeId
    if(ItemId != endptr)  // is numeric id
        tempNode.setNodeId( itemId, NdIdx);
    else            // is string id
        tempNode.setNodeId(UaString(ItemId), NdIdx);
//    std::cout <<"SETUP NODE: "<< tempNode.toString() << std::endl;
//    printf("Item num:%d str:'%s'\n",itemId,ItemId);
    nodeToRead[0].AttributeId = OpcUa_Attributes_Value;
    tempNode.copyTo(&(nodeToRead[0].NodeId)) ;

    status = m_pSession->read(serviceSettings,0,OpcUa_TimestampsToReturn_Both,nodeToRead,values,diagnosticInfos);
    if(status.isBad()){
        vUaNodeId.push_back(UaNodeId());
        return 1;
    }
    else {
        vUaNodeId.push_back(tempNode);
    }
    return 0;
}

long DevUaClient::getAllNodesFromId()
{
    UaStatus status;

    ServiceSettings     serviceSettings;
    UaDataValues        values;
    UaDiagnosticInfos   diagnosticInfos;
    UaReadValueIds      nodeToRead;
    OpcUa_UInt32        nrOfItems;

    nrOfItems = vUaItemInfo.size();
    nodeToRead.create(nrOfItems);

    for(OpcUa_UInt32 i=0;i<nrOfItems;i++) {
        OPCUA_ItemINFO      *pOPCUA_ItemINFO;
        UaNodeId            tempNode;
        char                *endptr;
        OpcUa_UInt16        NdIdx;
        char                ItemId[ITEMPATHLEN];

        pOPCUA_ItemINFO = vUaItemInfo[i];

        std::vector<std::string> itempath; // parsed item path
        boost::split(itempath,pOPCUA_ItemINFO->ItemPath,boost::is_any_of(","));
        if(itempath.size() != 2)
            return 1;

        NdIdx = (OpcUa_UInt16) strtol(itempath[0].c_str(),&endptr,10); // ItemPath is nodeId
        strncpy(ItemId,itempath[1].c_str(),ITEMPATHLEN);
        OpcUa_UInt32 itemId = (OpcUa_UInt32) strtol(ItemId,&endptr,10); // ItemPath is nodeId
        if(ItemId != endptr)  // is numeric id
            tempNode.setNodeId( itemId, NdIdx);
        else            // is string id
            tempNode.setNodeId(UaString(ItemId), NdIdx);
    //    std::cout <<"SETUP NODE: "<< tempNode.toString() << std::endl;
    //    printf("Item num:%d str:'%s'\n",itemId,ItemId);
        nodeToRead[0].AttributeId = OpcUa_Attributes_Value;
        tempNode.copyTo(&(nodeToRead[i].NodeId)) ;
        vUaNodeId.push_back(tempNode);
    }

    status = m_pSession->read(serviceSettings,0,OpcUa_TimestampsToReturn_Both,nodeToRead,values,diagnosticInfos);
    for(OpcUa_UInt32 i=0;i<nrOfItems;i++) {
        if (OpcUa_IsBad(values[i].StatusCode))
            vUaNodeId[i] = (UaNodeId());
    }
    return 0;
}

// clear vUaNodeId and recreate all nodes
long DevUaClient::getNodes()
{
    UaStatus status;
    int ret=0;
    OpcUa_UInt32            itemCount=vUaItemInfo.size();
    if(debug) printf("DevUaClient::getNodes mode:%d\n",mode);
    vUaNodeId.clear();
    switch(mode) {
    case BOTH:
        printf("DevUaClient::getNodes(BOTH)\n");
        for(OpcUa_UInt32 bpItem=0;bpItem<itemCount;bpItem++) {
            if(debug) printf("\t%d: %s\n",bpItem,(vUaItemInfo[bpItem])->ItemPath);
            if(getNodeFromBrowsePath( bpItem))
                if(getNodeFromId(bpItem) )
                    return 1;
        }
        break;
    case NODEID:
        printf("DevUaClient::getNodes(NODEID)\n");
        ret = getAllNodesFromId();
        break;
    case BROWSEPATH:
    case BROWSEPATH_CONCAT:
        printf("DevUaClient::getNodes(BROWSEPATH/BROWSEPATH_CONCAT)\n");
        status = getAllNodesFromBrowsePath();
        if(status.isBad())
            ret=1;
        break;
    case GETNODEMODEMAX: break;
    }
    if(debug>1) {
        printf("DevUaClient::getNodes() Dump nodes and items after init\n");
        for(OpcUa_UInt32 i=0;i<vUaNodeId.size();i++) {
            UaNodeId tempNode;
            tempNode = vUaNodeId.at(i);
            OPCUA_ItemINFO *pOpcItem;
            pOpcItem = vUaItemInfo.at(i);
            printf("    %s\t %s\n",pOpcItem->ItemPath,tempNode.toString().toUtf8());
        }
    }
    return ret;
}

UaStatus DevUaClient::createMonitoredItems()
{
    return m_pDevUaSubscription->createMonitoredItems(vUaNodeId,&vUaItemInfo);
}

UaStatus DevUaClient::writeFunc(ServiceSettings &serviceSettings,UaWriteValues &nodesToWrite,UaStatusCodeArray &results,UaDiagnosticInfos &diagnosticInfos)
{
    // Writes variable value synchronous to OPC server
    return m_pSession->write(serviceSettings,nodesToWrite,results,diagnosticInfos);

/*    // Writes variable values asynchronous to OPC server
    OpcUa_UInt32         transactionId=0;
    m_pSession->beginWrite(serviceSettings,nodesToWrite,transactionId);
*/
}

void DevUaClient::writeComplete( OpcUa_UInt32 transactionId,const UaStatus& result,const UaStatusCodeArray& results,const UaDiagnosticInfos& diagnosticInfos)
{
    if(result.isBad())
        printf("Bad Write Result: ");
        for(unsigned int i=0;i<results.length();i++)
            printf("%s ",result.isBad()? result.toString().toUtf8():"ok");
        printf("\n");
}

UaStatus DevUaClient::readFunc(UaDataValues &values,ServiceSettings &serviceSettings,UaDiagnosticInfos &diagnosticInfos)
{
    UaStatus          result;
    UaReadValueIds nodeToRead;
    OpcUa_UInt32        i,j;

    if(debug) printf("CALL DevUaClient::readFunc()\n");
    nodeToRead.create(pMyClient->vUaNodeId.size());
    for (i=0,j=0; i <pMyClient->vUaNodeId.size(); i++ )
    {
        if ( !vUaNodeId[i].isNull() ) {
            nodeToRead[j].AttributeId = OpcUa_Attributes_Value;
            (pMyClient->vUaNodeId[i]).copyTo(&(nodeToRead[j].NodeId)) ;
            j++;
        }
        else {
            printf("%s DevUaClient::readValues: Skip illegal node: \n",vUaItemInfo[i]->prec->name);
        }
    }
    nodeToRead.resize(j);
    result = m_pSession->read(
        serviceSettings,
        0,
        OpcUa_TimestampsToReturn_Both,
        nodeToRead,
        values,
        diagnosticInfos);
    if(result.isBad() ) {
        printf("FAILED: DevUaClient::readFunc()\n");
        if(diagnosticInfos.noOfStringTable() > 0) {
            for(unsigned int i=0;i<diagnosticInfos.noOfStringTable();i++)
                printf("%s",UaString(diagnosticInfos.stringTableAt(i)).toUtf8());
        }
    }
    return result;
}

void DevUaClient::itemStat(int verb)
{
    printf("OpcUa driver: Connected items: %lu\n",vUaItemInfo.size());
    if(verb>0) {
        printf("idx record Name          NS:PATH                                                       epics Type         opcUa Type        CB Out\n");
        for(unsigned int i=0;i< vUaItemInfo.size();i++) {
            OPCUA_ItemINFO* pOPCUA_ItemINFO = vUaItemInfo[i];
            printf("%3d %-20s %-60s %2d,%-15s %2d:%-15s stat=%d\n",pOPCUA_ItemINFO->itemIdx,pOPCUA_ItemINFO->prec->name,
                   pOPCUA_ItemINFO->ItemPath,
                   pOPCUA_ItemINFO->recDataType,epicsTypeNames[pOPCUA_ItemINFO->recDataType],
                   pOPCUA_ItemINFO->itemDataType,variantTypeStrings(pOPCUA_ItemINFO->itemDataType),
                   pOPCUA_ItemINFO->stat);
        }
    }
}

/***************** just for debug ********************/

void print_OpcUa_DataValue(_OpcUa_DataValue *d)
{
    if (OpcUa_IsGood(d->StatusCode)) {
        printf("Datatype: %d ArrayType:%d SourceTS: H%d,L%d,Pico%d ServerTS: H%d,L%d,Pico%d",
               (d->Value).Datatype,(d->Value).ArrayType,
                (d->SourceTimestamp).dwLowDateTime, (d->SourceTimestamp).dwHighDateTime,d->SourcePicoseconds,
                (d->ServerTimestamp).dwLowDateTime, (d->ServerTimestamp).dwHighDateTime,d->ServerPicoseconds);
//        (d->Value).Value;
    }
    else {
        printf("Statuscode BAD: %d %s",d->StatusCode,UaStatus(d->StatusCode).toString().toUtf8());
    }
    printf("\n");

}

void printVal(UaVariant &val,OpcUa_UInt32 IdxUaItemInfo)
{
    if(val.isArray()) {
        for(int i=0;i<val.arraySize();i++) {
            if(UaVariant(val[i]).type() < OpcUaType_String)
                printf("%s[%d] %s\n",pMyClient->vUaItemInfo[IdxUaItemInfo]->ItemPath,i,UaVariant(val[i]).toString().toUtf8());
            else
                printf("%s[%d] '%s'\n",pMyClient->vUaItemInfo[IdxUaItemInfo]->ItemPath,i,UaVariant(val[i]).toString().toUtf8());
        }
    }
    else {
        if(val.type() < OpcUaType_String)
            printf("%s %s\n",pMyClient->vUaItemInfo[IdxUaItemInfo]->ItemPath, val.toString().toUtf8());
        else
            printf("%s '%s'\n",pMyClient->vUaItemInfo[IdxUaItemInfo]->ItemPath, val.toString().toUtf8());
    }
}

/***************** C Wrapper Functions ********************/

/* Client: Read / setup monitors. First setup items by setOPCUA_Item() function */
long OpcReadValues(int verbose,int monitored)
    {
    UaStatus status;

    ServiceSettings   serviceSettings;
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;

    if(verbose){
        printf("OpcReadValues\n");
        pMyClient->debug = 1;
    }
    if(pMyClient->getNodes() ) {
        printf("\n");
        return 1;
    }
    if(monitored)
        pMyClient->createMonitoredItems();
    else {
        status = pMyClient->readFunc(values,serviceSettings,diagnosticInfos );
        if (status.isGood()) {
            if(verbose) printf("READ VALUES success: %i\n",values.length());
            for(OpcUa_UInt32 j=0;j<values.length();j++) {

                if (OpcUa_IsGood(values[j].StatusCode)) {
                    UaVariant val = values[j].Value;
                    printVal(val,j);
                }
                else {
                    printf("Read item[%i] failed with status %s\n",j,UaStatus(values[j].StatusCode).toString().toUtf8());
                }
            }
        }
        else {
            // Service call failed
            printf("READ VALUES failed with status %s\n", status.toString().toUtf8());
        }
    }
    return 0;
}
/* Client: write one value. First setup items by setOPCUA_Item() function */
long OpcWriteValue(int opcUaItemIndex,double val,int verbose)
    {
    UaStatus            status;
    ServiceSettings     serviceSettings;    // Use default settings
    UaVariant         tempValue;
    UaWriteValues nodesToWrite;       // Array of nodes to write
    UaStatusCodeArray   results;            // Returns an array of status codes
    UaDiagnosticInfos   diagnosticInfos;    // Returns an array of diagnostic info
    OPCUA_ItemINFO* pOPCUA_ItemINFO;
    pOPCUA_ItemINFO = (pMyClient->vUaItemInfo).at(opcUaItemIndex);

    if(verbose){
        printf("OpcWriteValue(%d,%f)\nTRANSLATEBROWSEPATH\n",opcUaItemIndex,val);
        pMyClient->debug = verbose;
    }
    
    nodesToWrite.create(1);
//from OPCUA_ItemINFO:    UaNodeId temp1Node(pOPCUA_ItemINFO->ItemPath,pOPCUA_ItemINFO->NdIdx);
    UaNodeId tempNode(pMyClient->vUaNodeId[pOPCUA_ItemINFO->itemIdx]);
    tempNode.copyTo(&nodesToWrite[0].NodeId);
    nodesToWrite[0].AttributeId = OpcUa_Attributes_Value;
    tempValue.setDouble(val);
    tempValue.copyTo(&nodesToWrite[0].Value.Value);

    // Writes variable value synchronous to OPC server
    status = pMyClient->writeFunc(serviceSettings,nodesToWrite,results,diagnosticInfos);
    if ( status.isBad() )
    {
        printf("** Error: UaSession::write failed [ret=%s] **\n", status.toString().toUtf8());
        return 1;
    }
    return 0;
}
/* iocShell: record write func  */
epicsRegisterFunction(OpcUaWriteItems);
long OpcUaWriteItems(OPCUA_ItemINFO* pOPCUA_ItemINFO)
{
    UaStatus            status;
    ServiceSettings     serviceSettings;    // Use default settings
    UaVariant           tempValue;
    UaWriteValues       nodesToWrite;       // Array of nodes to write
    UaStatusCodeArray   results;            // Returns an array of status codes
    UaDiagnosticInfos   diagnosticInfos;    // Returns an array of diagnostic info

    nodesToWrite.create(1);
//from OPCUA_ItemINFO:    UaNodeId temp1Node(pOPCUA_ItemINFO->ItemPath,pOPCUA_ItemINFO->NdIdx);
    UaNodeId tempNode(pMyClient->vUaNodeId[pOPCUA_ItemINFO->itemIdx]);
    tempNode.copyTo(&nodesToWrite[0].NodeId);
    nodesToWrite[0].AttributeId = OpcUa_Attributes_Value;

    switch((int)pOPCUA_ItemINFO->itemDataType){
    case OpcUaType_SByte:

        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_Byte:
        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_Int16:
        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_UInt16:
        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_Int32:
        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_UInt32:
        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_Float:
        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_Double:
        switch(pOPCUA_ItemINFO->recDataType){//REC_Datatype(EPICS_Datatype)
        case epicsInt32T:   tempValue.setInt32( *((epicsInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsUInt32T:  tempValue.setUInt32(*((epicsUInt32*)(pOPCUA_ItemINFO)->pRecVal));break;
        case epicsFloat64T: tempValue.setDouble(*((epicsFloat64*)(pOPCUA_ItemINFO)->pRecVal));break;
        default: return 1;
        }
        break;
    case OpcUaType_String:
        if(pOPCUA_ItemINFO->recDataType == epicsOldStringT) { /* stringin/outRecord definition of 'char val[40]' */
            tempValue.setString(UaString((char*)pOPCUA_ItemINFO->pRecVal));break;
        }
        break;
    default:
        std::cout <<pOPCUA_ItemINFO->prec->name<< "\tOpcUaWriteItems ERROR: unsupported epics data type: "<< epicsTypeNames[pOPCUA_ItemINFO->recDataType]<< std::endl;
    }

    tempValue.copyTo(&nodesToWrite[0].Value.Value);

    status = pMyClient->writeFunc(serviceSettings,nodesToWrite,results,diagnosticInfos);
    if ( status.isBad() )
    {
        printf("** Error: UaSession::write failed [ret=%s] **\n", status.toString().toUtf8());
        return 1;
    }
    return 0;
}

/* iocShell: Read and setup pOPCUA_ItemINFO Item data type, createMonitoredItems */
epicsRegisterFunction(OpcUaSetupMonitors);
long OpcUaSetupMonitors(void)
{
    UaStatus status;
    UaDataValues values;
    ServiceSettings     serviceSettings;
    UaDiagnosticInfos   diagnosticInfos;

    if(pMyClient->debug) printf("OpcUaSetupMonitors");

    if(pMyClient->debug) printf("OpcUaSetupMonitors Browsepath ok len = %d\n",(int)pMyClient->vUaNodeId.size());

    if(pMyClient->getNodes() )
        return 1;
    status = pMyClient->readFunc(values,serviceSettings,diagnosticInfos );
    if (status.isBad()) {
        printf("OpcUaSetupMonitors: READ VALUES failed with status %s\n", status.toString().toUtf8());
        return -1;
    }
    if(pMyClient->debug) printf("OpcUaSetupMonitors Read values ok nr = %i\n",values.length());
    for(OpcUa_UInt32 j=0;j<values.length();j++) {

        if (OpcUa_IsGood(values[j].StatusCode)) {
            OPCUA_ItemINFO* pOPCUA_ItemINFO = pMyClient->vUaItemInfo[j];
            if(values[j].Value.ArrayType && !pOPCUA_ItemINFO->isArray) {
                 printf("OpcUaSetupMonitors %s: Dont Support Array Data\n",pOPCUA_ItemINFO->prec->name);
            }
            else {

                pOPCUA_ItemINFO->itemDataType = (int) values[j].Value.Datatype;
                epicsMutexLock(pOPCUA_ItemINFO->flagLock);
                pOPCUA_ItemINFO->isArray = 0;
                epicsMutexUnlock(pOPCUA_ItemINFO->flagLock);
                if(pMyClient->debug) printf("%3d %15s: %p noOut: %d\n",pOPCUA_ItemINFO->itemIdx,pOPCUA_ItemINFO->prec->name,pOPCUA_ItemINFO,pOPCUA_ItemINFO->noOut);

            }
        }
        else {
            printf("Read item[%i] failed with status %s\n",j,UaStatus(values[j].StatusCode).toString().toUtf8());
        }
    }
    pMyClient->createMonitoredItems();
    return 0;
}

/* iocShell/Client: unsubscribe, disconnect from server */
long opcUa_close(int verbose)
{
    UaStatus status;
    if(verbose) printf("opcUa_close()\n\tunsubscribe\n");

    status = pMyClient->unsubscribe();
    if(verbose) printf("\tdisconnect\n");
    status = pMyClient->disconnect();

    delete pMyClient;
    pMyClient = NULL;

    if(verbose) printf("\tcleanup\n");
    UaPlatformLayer::cleanup();
    return 0;
}

/* iocShell/Client: Setup an opcUa Item for the driver*/
long setOPCUA_Item(OPCUA_ItemINFO *h)
{
    long ret =  pMyClient->setOPCUA_Item(h);
    return ret;
}

/* iocShell/Client: Setup server url and certificates, connect and subscribe */
long opcUa_init(UaString &g_serverUrl, UaString &g_applicationCertificate, UaString &g_applicationPrivateKey, GetNodeMode mode, int verbose=0)
{
    UaStatus status;
    // Initialize the UA Stack platform layer
    UaPlatformLayer::init();

    // Create instance of DevUaClient
    pMyClient = new DevUaClient();

    pMyClient->applicationCertificate = g_applicationCertificate;
    pMyClient->applicationPrivateKey  = g_applicationPrivateKey;
    pMyClient->mode = mode;
    pMyClient->debug = verbose;
    // Connect to OPC UA Server
    status = pMyClient->connect(g_serverUrl);
    if(status.isBad()) {
        printf("drvOpcUaSetup: Failed to connect to server '%s'' \n",g_serverUrl.toUtf8());
        return 1;
    }
    // Create subscription
    status = pMyClient->subscribe();
    if(status.isBad()) {
        printf("drvOpcUaSetup: Failed to subscribe on server '%s'' \n",g_serverUrl.toUtf8());
        return 1;
    }
    return 0;
}
/* iocShell: shell functions */

static const iocshArg drvOpcUaSetupArg0 = {"[URL]", iocshArgString};
static const iocshArg drvOpcUaSetupArg1 = {"[CERT_PATH]", iocshArgString};
static const iocshArg drvOpcUaSetupArg2 = {"[HOST]", iocshArgString};
static const iocshArg drvOpcUaSetupArg3 = {"[MODE]", iocshArgString};
static const iocshArg drvOpcUaSetupArg4 = {"Debug Level", iocshArgInt};
static const iocshArg *const drvOpcUaSetupArg[5] = {&drvOpcUaSetupArg0,&drvOpcUaSetupArg1,&drvOpcUaSetupArg2,&drvOpcUaSetupArg3,&drvOpcUaSetupArg4};
iocshFuncDef drvOpcUaSetupFuncDef = {"drvOpcUaSetup", 5, drvOpcUaSetupArg};
void drvOpcUaSetup (const iocshArgBuf *args )
{
    UaString g_serverUrl;
    UaString g_certificateStorePath;
    UaString g_defaultHostname;
    UaString g_applicationCertificate;
    UaString g_applicationPrivateKey;;
    int g_mode = 0;

    if(args[0].sval == NULL)
    {
      errlogPrintf("drvOpcUaSetup: ABORT Missing Argument \"url\".\n");
      return;
    }
    g_serverUrl = args[0].sval;
    if(args[1].sval == NULL)
    {
      errlogPrintf("drvOpcUaSetup: ABORT Missing Argument \"cert path\".\n");
      return;
    }
    g_certificateStorePath = args[1].sval;
    if(args[2].sval == NULL)
    {
      errlogPrintf("drvOpcUaSetup: ABORT Missing Argument \"host name\".\n");
      return;
    }
    g_defaultHostname = args[2].sval;

    g_certificateStorePath = args[1].sval;
    g_mode = atoi(args[3].sval);
    if( (g_mode<0)||(g_mode >= GETNODEMODEMAX)) {
        errlogPrintf("drvOpcUaSetup: parameter mode=%d: outside range, set to default: Browsepath or nodeId (BOTH)\n",g_mode);
        g_mode = 0;
    }
    int verbose = args[4].ival;

    signal(SIGINT, signalHandler);

    g_applicationCertificate = g_certificateStorePath + "/certs/cert_client_" + g_defaultHostname + ".der";
    g_applicationPrivateKey	 = g_certificateStorePath + "/private/private_key_client_" + g_defaultHostname + ".pem";
    if(verbose) {
        printf("Host:\t'%s'\n",g_defaultHostname.toUtf8());
        printf("URL:\t'%s'\n",g_serverUrl.toUtf8());
        printf("Set certificate path:\n\t'%s'\n",g_certificateStorePath.toUtf8());
        printf("Client Certificate:\n\t'%s'\n",g_applicationCertificate.toUtf8());
        printf("Client privat key:\n\t'%s'\n",g_applicationPrivateKey.toUtf8());
    }

    if(opcUa_init(g_serverUrl,g_applicationCertificate,g_applicationPrivateKey,(GetNodeMode)g_mode),verbose)
    {
        printf("Error in opcUa_init()");
        return;
    }
}
epicsRegisterFunction(drvOpcUaSetup);

static const iocshArg OpcUaDebugArg0 = {"Debug Level", iocshArgInt};
static const iocshArg *const OpcUaDebugArg[1] = {&OpcUaDebugArg0};
iocshFuncDef OpcUaDebugFuncDef = {"OpcUaDebug", 1, OpcUaDebugArg};
void OpcUaDebug (const iocshArgBuf *args )
{
    if(pMyClient)
        pMyClient->debug = args[0].ival;
    else
        printf("Ignore: OpcUa not initialized\n");
    return;
}
epicsRegisterFunction(OpcUaDebug);

static const iocshArg OpcUaStatArg0 = {"Debug Level", iocshArgInt};
static const iocshArg *const OpcUaStatArg[1] = {&OpcUaStatArg0};
iocshFuncDef OpcUaStatFuncDef = {"OpcUaStat", 1, OpcUaStatArg};
void OpcUaStat (const iocshArgBuf *args )
{
    pMyClient->itemStat(args[0].ival);
    return;
}
epicsRegisterFunction(OpcUaStat);

//create a static object to make shure that opcRegisterToIocShell is called on beginning of
class OpcRegisterToIocShell
{
public :
        OpcRegisterToIocShell(void);
};

OpcRegisterToIocShell::OpcRegisterToIocShell(void)
{
    iocshRegister(&drvOpcUaSetupFuncDef, drvOpcUaSetup);
    iocshRegister(&OpcUaDebugFuncDef, OpcUaDebug);
    iocshRegister(&OpcUaStatFuncDef, OpcUaStat);
      //
}
static OpcRegisterToIocShell opcRegisterToIocShell;

