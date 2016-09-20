#!../../bin/linux-x86_64/OPCUAIOC

cd ../..
epicsEnvSet IOC OPCUAIOC
dbLoadDatabase "dbd/OPCUAIOC.dbd",0,0
${IOC}_registerRecordDeviceDriver pdbbase

# hazel /home/kuner/Dokumente.BESSY/projects/opcProjekt/uasdkDebian7/bin/server_lesson05
#drvOpcUaSetup("opc.tcp://localhost:48010","/home/kuner/Dokumente.BESSY/projects/opcProjekt/work/myCertificates/hazel_store/certs","hazel",0)
#dbLoadRecords "db/OPCUA_RECORD.db"

# hazel /home/kuner/Dokumente.BESSY/projects/opcProjekt/FREEOPCUA/freeOpcUaEpicsServer/bin/linux-x86_64/server
drvOpcUaSetup("opc.tcp://hazel.acc.bessy.de:4841","","hazel",0)
dbLoadRecords "db/freeopcuaTEST.db"


OpcUaDebug(3)
setIocLogDisable 1
iocInit
OpcUaStat(0)
