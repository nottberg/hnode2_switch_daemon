#ifndef __SWITCHMANAGER_H__
#define __SWITCHMANAGER_H__

#include <string>
#include <list>
#include <vector>
#include <map>

#include <stdio.h>

#include "HNDaemonLog.h"

typedef enum HNSwitchManagerResultEnum
{
    HNSW_RESULT_SUCCESS,
    HNSW_RESULT_FAILURE
}HNSW_RESULT_T;

class HNSWDevice
{
    private:
        std::string id;
        std::string desc;
        std::string model;

    public:
        HNSWDevice();
       ~HNSWDevice();

        void setModel( std::string value );
        void setID( std::string value );
        void setDesc( std::string value );

        std::string getModel();
        std::string getID();
        std::string getDesc();

        virtual void parsePair( std::string, std::string ) = 0;
        virtual void debugPrint( uint offset ) = 0;

        virtual HNSW_RESULT_T initDevice() = 0;
        virtual HNSW_RESULT_T closeDevice() = 0;
        virtual HNSW_RESULT_T restartDevice() = 0;

        virtual void initNextState() = 0;
        virtual HNSW_RESULT_T updateStateToOn( std::string param ) = 0;
        virtual HNSW_RESULT_T applyNextState() = 0;

};

class HNSWI2CExpander : public HNSWDevice
{
    private:
        uint devnum;
        uint busaddr;

        int i2cfd;
 
        uint32_t nextState;

        // MCP23008 interface routines
        HNSW_RESULT_T mcp23008Init();
        HNSW_RESULT_T mcp23008Close();

        HNSW_RESULT_T mcp23008SetPortMode( uint value );
        HNSW_RESULT_T mcp23008SetPortPullup( uint value );
        HNSW_RESULT_T mcp23008SetPortState( uint value );

        HNSW_RESULT_T mcp23008GetPortMode( uint &value );
        HNSW_RESULT_T mcp23008GetPortPullup( uint &value );
        HNSW_RESULT_T mcp23008GetPortState( uint &value );

    public:
        HNSWI2CExpander();
       ~HNSWI2CExpander(); 

        void parsePair( std::string key, std::string value );

        HNSW_RESULT_T initDevice();
        HNSW_RESULT_T closeDevice();
        HNSW_RESULT_T restartDevice();

        void initNextState();
        HNSW_RESULT_T updateStateToOn( std::string param );
        HNSW_RESULT_T applyNextState();

        void debugPrint( uint offset );

        static bool supportedModel( std::string model );
};

class HNSWDeviceFactory
{
    public:
        static HNSW_RESULT_T createDeviceForClass( std::string devClass, std::string devModel, HNSWDevice **devObj );

        static void destroyDevice( HNSWDevice *devObj );
};

class HNSWSwitch
{
    private:
        // the identifier of the the switch itself
        std::string swID;

        // The identifier for the controlling device
        std::string devID;

        // A parameter string to pass to the controlling device.
        // In the case of the i2c-expander this is just a integer
        // indicating the bit position the controls the switch
        std::string devParam; 

        // A user provided descriptive string of switch function.
        std::string desc;     

    public:
        HNSWSwitch();
       ~HNSWSwitch();

        void setSWID( std::string value );
        void setDesc( std::string value );
        void setDevID( std::string value );
        void setDevParam( std::string value );

        std::string getSWID();
        std::string getDesc();
        std::string getDevID();
        std::string getDevParam();

        void debugPrint( uint offset );
};

class HNSwitchManagerNotifications
{
    public:
        virtual void signalError( std::string errMsg ) = 0;
        virtual void signalRunning() = 0;
};

#define HNSW_ROOT_DIRECTORY_DEFAULT "/var/cache/hnode2"

class HNSwitchManager
{
    private:
        HNDaemonLogSrc log;

        std::string rootDirPath;
        HNSwitchManagerNotifications *notifySink;

        std::string deviceName;
        std::string instanceName;

        std::map< std::string, HNSWDevice* > deviceMap;
        std::map< std::string, HNSWSwitch >  switchMap;

        HNSW_RESULT_T generateFilePath( std::string &fpath );

    public:
        HNSwitchManager();
       ~HNSwitchManager();

        void setDstLog( HNDaemonLog *dstPtr );

        void setNotificationSink( HNSwitchManagerNotifications *sink );
        void clearNotificationSink();

        void setRootDirectory( std::string path );
        std::string getRootDirectory();

        void clear();

        HNSW_RESULT_T loadConfiguration( std::string devname, std::string instance );

        HNSW_RESULT_T initDevices();
        HNSW_RESULT_T closeDevices();

        unsigned int getSwitchCount();
        HNSWSwitch* getSwitchByIndex( int index );
        HNSWSwitch* getSwitchByID( std::string swid );

        HNSW_RESULT_T processOnState( std::vector< std::string > &swidOnList );

        void debugPrint();
};

#endif // __SWITCHMANAGER_H__

