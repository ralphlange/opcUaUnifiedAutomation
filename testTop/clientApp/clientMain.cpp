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
/*  now with translate path, 
    Start with server: elbe:/home/pc/kuner/ctl/OPC_UA/uasdkDebian7/bin/server_lesson03
**************************************************/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
// toolbox header
#include "uaplatformlayer.h"
#include "uabase.h"
#include "uaclientsdk.h"
#include "uasession.h"

#include"dbCommon.h" // need dummy prec for print in Subscription onDataChange callback
#include"epicsMutex.h"
#include "drvOpcUa.h"

#ifdef _WIN32
    #include <windows.h>
    #include "getopt.h"
#endif


UaString g_startNode;
UaString g_serverUrl;
UaString g_certificateStorePath;
UaString g_defaultHostname;
UaString g_applicationCertificate;
UaString g_applicationPrivateKey;;
UaString optionUsage = "client [OPTIONS] PATH1 PATH...\nclient -w [OPTIONS] PATH [VALUE]\n"
        "PATH: scalar: NS:path.items\n"
        "      array: SIZE/NS:path.items\n\n"
        "OPTIONS:\n\n"
        "  -h : This help\n"
        "  -H HOSTNAME: own hostname. optional for certpath, if $HOST is not defined\n"
        "  -c CERTPATH: Path to certivicate store full path is: <CERTPATH>/certs/cert_client_<HOSTNAME>.der\n"
        "  -M Mode, default = BOTH\n"
        "           0=BOTH:NODEID or BROWSEPATH\n"
        "           1=NODEID: 'NSINDEX,IDENTIFIER' string or numeric id\n"
        "           2=BROWSEPATH: 'NSINDEX:BROWSE.PATH'\n"
        "           4=BROWSEPATH_CONCAT:Concatenate: browsepath parts for each node\n"
        "  -n Read arguments are not a path but a NodeId 'NS:identifier'\n"
        "  -m monitor\n"
        "  -u URL: Server URL\n"
        "  -w : Write scalar, arg VALUE required\n"
        "  -v : Verbose level 1\n"
        "  -V n: Verbose level n\n"
        "\n";

static int verbose   = 0;
static int monitored = 0;
static int writeOpt  = 0;
static GetNodeMode modeOpt = BOTH;

int getOptions(int argc, char *argv[]) {
    char c;

    int hasCertOption = 0;
    while((c =  getopt(argc, argv, "wmhvM:V:c:u:H:s:")) != EOF)
	{
        switch (c)
        {
        case 'h':
            printf("%s",optionUsage.toUtf8());
            exit(0);
        case 'M':
            modeOpt = (GetNodeMode) atoi(optarg);
            if((modeOpt<0)||(modeOpt>=GETNODEMODEMAX)) {
                printf("Parameter 'M %d': outside range, set to default: Browsepath or nodeId (BOTH)\n",modeOpt);
                modeOpt=BOTH;
            }
            break;
        case 'c':
            g_certificateStorePath = optarg;
            hasCertOption = 1;
            break;
        case 'm':
           monitored = 1;
            break;
        case 'u':
            g_serverUrl = optarg;
            break;
        case 'w':
            writeOpt = 1;
            break;
        case 'H':
           g_defaultHostname = optarg;
           hasCertOption = 1;
           break;
        case 'v':
            verbose = 1;
            break;
        case 'V':
            verbose = atoi(optarg);
            break;
        }
    }
    if(hasCertOption) {
        g_applicationCertificate = g_certificateStorePath + "/certs/cert_client_" + g_defaultHostname + ".der";
        g_applicationPrivateKey	 = g_certificateStorePath + "/private/private_key_client_" + g_defaultHostname + ".pem";
    }
    return 0;
}

OPCUA_ItemINFO * newOpcItem(char *path,int verb)
{
    OPCUA_ItemINFO *pOPCUA_ItemINFO = NULL;
    dbCommon *prec = NULL;
    char *src = path;

    pOPCUA_ItemINFO =  (OPCUA_ItemINFO *) calloc(1,sizeof(OPCUA_ItemINFO));
    prec = (dbCommon*) calloc(1,sizeof(dbCommon));
    int len = strlen(path);
    if(len>60)
        src += len-61;
    strncpy(prec->name,src,61); //set dummy record name to path print in Subscription onDataChange callback
    prec->tpro = verb;
    pOPCUA_ItemINFO->prec = prec;
    pOPCUA_ItemINFO->recDataType = epicsOldStringT;
    pOPCUA_ItemINFO->inpDataType=epicsOldStringT;
    pOPCUA_ItemINFO->isArray=0;
    pOPCUA_ItemINFO->ioscanpvt  = 0;
    pOPCUA_ItemINFO->pInpVal = NULL;
    pOPCUA_ItemINFO->pRecVal = (char*) prec->desc;
    pOPCUA_ItemINFO->debug = verbose;
    pOPCUA_ItemINFO->flagLock = epicsMutexMustCreate();
    if(strlen(path) < ITEMPATHLEN) {
        strcpy(pOPCUA_ItemINFO->ItemPath,path);
        if(! setOPCUA_Item(pOPCUA_ItemINFO) )
            return pOPCUA_ItemINFO;
    }
    free(pOPCUA_ItemINFO);
    printf("Skip Argument '%s'\n",path);
    return NULL;
}

int main(int argc, char* argv[], char *envp[])
{
    long result;
    OPCUA_ItemINFO *pOPCUA_ItemINFO = NULL;

    // init the toolbox to gain access to the toolbox functions

    signal(SIGINT, signalHandler);
    g_defaultHostname = getenv("HOST");
    if(g_defaultHostname.isNull())
        g_defaultHostname="hostDefault";

    getOptions(argc, argv);

    if(writeOpt && monitored) {
        printf("mutual exclusive arguments -m, -w\n");
        exit(1);
    }
    if(verbose) {
        printf("Host:\t'%s'\n",g_defaultHostname.toUtf8());
        printf("URL:\t'%s'\n",g_serverUrl.toUtf8());
        printf("Set certificate path:\n\t'%s'\n",g_certificateStorePath.toUtf8());
        printf("Client Certificate:\n\t'%s'\n",g_applicationCertificate.toUtf8());
        printf("Client privat key:\n\t'%s'\n",g_applicationPrivateKey.toUtf8());
    }

    result = opcUa_init(g_serverUrl,g_applicationCertificate,g_applicationPrivateKey,g_defaultHostname,(GetNodeMode)modeOpt,verbose);
    if(result)
    {
        printf("Error in opcUa_init()");
        exit(1);
    }
    if( writeOpt ) {
        if(verbose) printf("Write Arguments: \n");
        if( ((argc-optind)%2) ) {
            printf("illegal write args, not of: PATH VALUE PATH VALUE..\n");
            exit(1);
        }
        for(int idx=optind;idx<argc;idx += 2) {
            if(verbose) printf("\t'%s'\t'%s'\n",argv[idx],argv[idx+1]);
            OPCUA_ItemINFO *pOPCUA_ItemINFO;
            char *valStr = argv[idx+1];
            double doubleVal = atof(valStr);
            pOPCUA_ItemINFO = newOpcItem(argv[idx],verbose);
            if( pOPCUA_ItemINFO == NULL )
                exit(1);
            result = OpcWriteValue(0,doubleVal,verbose);
            if(result)
                printf("OpcWriteValue failed: %ld\n",result);
            else
                printf("OpcWriteValue success\n");
        }
        
    }
    else if( (argc-optind) > 0) {
        if(verbose) printf("Read Arguments: \n");
        for(int idx=optind;idx<argc;idx++) {
            if(verbose) printf("\t%s\n",argv[idx]);
            pOPCUA_ItemINFO = newOpcItem(argv[idx],((monitored &&(verbose<2))?2:verbose) );
            if( pOPCUA_ItemINFO == NULL ) {// monitores need verb>=2 to print value in sampleSubscription::dataChange
                printf("EXIT\n");
                exit(1);
            }
        }

        if( OpcReadValues(verbose,monitored))
            printf("Error in OpcReadValues\n");
        while(monitored){
#ifdef _WIN32
            Sleep(1);
#else
            sleep(1);
#endif
        }
    }

    result = opcUa_close(verbose);
    return 0;
}
