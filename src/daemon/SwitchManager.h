#ifndef __SWITCHMANAGER_H__
#define __SWITCHMANAGER_H__

#include <string>
#include <list>
#include <vector>
#include <map>

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef enum SwitchManagerResultEnum
{
    SWM_RESULT_SUCCESS,
    SWM_RESULT_FAILURE
}SWM_RESULT_T;

class SWMDevice
{
    private:
        std::string id;
        std::string desc;
        std::string model;

    public:
        SWMDevice();
       ~SWMDevice();

        void setModel( std::string value );
        void setID( std::string value );
        void setDesc( std::string value );

        std::string getModel();
        std::string getID();
        std::string getDesc();

        virtual void parsePair( std::string, std::string ) = 0;
        virtual void debugPrint( uint offset ) = 0;

        virtual SWM_RESULT_T initDevice() = 0;
        virtual SWM_RESULT_T closeDevice() = 0;
        virtual SWM_RESULT_T restartDevice() = 0;

        virtual void initNextState() = 0;
        virtual SWM_RESULT_T updateStateToOn( std::string param ) = 0;
        virtual SWM_RESULT_T applyNextState() = 0;

};

class SWMI2CExpander : public SWMDevice
{
    private:
        uint devnum;
        uint busaddr;

        int i2cfd;
 
        uint32_t nextState;

        // MCP23008 interface routines
        SWM_RESULT_T mcp23008Init();
        SWM_RESULT_T mcp23008Close();

        SWM_RESULT_T mcp23008SetPortMode( uint value );
        SWM_RESULT_T mcp23008SetPortPullup( uint value );
        SWM_RESULT_T mcp23008SetPortState( uint value );

        SWM_RESULT_T mcp23008GetPortMode( uint &value );
        SWM_RESULT_T mcp23008GetPortPullup( uint &value );
        SWM_RESULT_T mcp23008GetPortState( uint &value );

    public:
        SWMI2CExpander();
       ~SWMI2CExpander(); 

        void parsePair( std::string key, std::string value );

        SWM_RESULT_T initDevice();
        SWM_RESULT_T closeDevice();
        SWM_RESULT_T restartDevice();

        void initNextState();
        SWM_RESULT_T updateStateToOn( std::string param );
        SWM_RESULT_T applyNextState();

        void debugPrint( uint offset );

        static bool supportedModel( std::string model );
};

class SWMDeviceFactory
{
    public:
        static SWM_RESULT_T createDeviceForClass( std::string devClass, std::string devModel, SWMDevice **devObj );

        static void destroyDevice( SWMDevice *devObj );
};

class SWMSwitch
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
        SWMSwitch();
       ~SWMSwitch();

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

class SwitchManagerNotifications
{
    public:
        virtual void signalError( std::string errMsg ) = 0;
        virtual void signalRunning() = 0;
};

#define SWM_ROOT_DIRECTORY_DEFAULT "/var/cache/hnode2"

class SwitchManager
{
    private:
        std::string rootDirPath;
        SwitchManagerNotifications *notifySink;

        std::string deviceName;
        std::string instanceName;

        std::map< std::string, SWMDevice* > deviceMap;
        std::map< std::string, SWMSwitch >  switchMap;

        SWM_RESULT_T generateFilePath( std::string &fpath );

    public:
        SwitchManager();
       ~SwitchManager();

        void setNotificationSink( SwitchManagerNotifications *sink );
        void clearNotificationSink();

        void setRootDirectory( std::string path );
        std::string getRootDirectory();

        void clear();

        SWM_RESULT_T loadConfiguration( std::string devname, std::string instance );

        SWM_RESULT_T initDevices();
        SWM_RESULT_T closeDevices();

        unsigned int getSwitchCount();
        SWMSwitch* getSwitchByIndex( int index );
        SWMSwitch* getSwitchByID( std::string swid );

        SWM_RESULT_T processOnState( std::vector< std::string > &swidOnList );

        void debugPrint();
};

#endif // __SWITCHMANAGER_H__

