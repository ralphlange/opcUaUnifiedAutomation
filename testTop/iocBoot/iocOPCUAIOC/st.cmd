#!../../bin/linux-x86_64/OPCUAIOC

cd ../..
epicsEnvSet IOC OPCUAIOC
dbLoadDatabase "dbd/OPCUAIOC.dbd",0,0
${IOC}_registerRecordDeviceDriver pdbbase

# Server oriole - Softing Server to PLC
#drvOpcUaSetup("opc.tcp://193.149.12.196:4840","/home/kuner/Dokumente.BESSY/projects/opcProjekt/work/myCertificates/hazel_store/certs","hazel",3)
#dbLoadRecords "db/PLC_RECORD.db"

# hazel /home/kuner/Dokumente.BESSY/projects/opcProjekt/uasdkDebian7/bin/server_lesson05
#drvOpcUaSetup("opc.tcp://localhost:48010","/home/kuner/Dokumente.BESSY/projects/opcProjekt/work/myCertificates/hazel_store/certs","hazel",0)
#dbLoadRecords "db/OPCUA_RECORD.db"

# hazel /home/kuner/Dokumente.BESSY/projects/opcProjekt/FREEOPCUA/freeOpcUaEpicsServer/bin/linux-x86_64/server
drvOpcUaSetup("opc.tcp://localhost:4841","/home/kuner/Dokumente.BESSY/projects/opcProjekt/work/myCertificates/hazel_store/certs","hazel",0)
dbLoadRecords "db/freeopcuaTEST.db"

# Server FUG PLC
#drvOpcUaSetup("opc.tcp://172.18.16.208:4840","/home/kuner/Dokumente.BESSY/projects/opcProjekt/work/myCertificates/hazel_store/certs","hazel",1)
#dbLoadRecords "db/HPFR1H1RF.db"


OpcUaDebug(3)
setIocLogDisable 1
iocInit
OpcUaStat(0)
