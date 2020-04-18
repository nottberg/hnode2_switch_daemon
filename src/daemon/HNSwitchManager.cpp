#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

extern "C"{
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
}

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include "HNSwitchManager.h"
#include "HNSwitchDaemon.h"

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

HNSWDevice::HNSWDevice()
{

}

HNSWDevice::~HNSWDevice()
{

}

void 
HNSWDevice::setDstLog( HNDaemonLog *dstLog )
{
    log.setDstLog( dstLog );
}

void 
HNSWDevice::setModel( std::string value )
{
    model = value;
}

void 
HNSWDevice::setID( std::string value )
{
    id = value;
}

void 
HNSWDevice::setDesc( std::string value )
{
    desc = value;
}

std::string 
HNSWDevice::getModel()
{
    return model;
}

std::string 
HNSWDevice::getID()
{
    return id;
}

std::string 
HNSWDevice::getDesc()
{
    return desc;
}

uint 
HNSWDevice::getHealthComponentCount()
{
    return 1;
}

HNDaemonHealth* 
HNSWDevice::getHealthComponent( uint index )
{
    if( index == 0 )
        return &health;

    return NULL;
}

HNSWI2CExpander::HNSWI2CExpander()
{
    devnum  = 0;
    busaddr = 0;
    i2cfd   = 0;
}

HNSWI2CExpander::~HNSWI2CExpander()
{
    closeDevice();
}

void 
HNSWI2CExpander::parsePair( std::string key, std::string value )
{
    if( "devnum" == key )
    {
        devnum = strtol( value.c_str(), NULL, 0 );
    }
    else if( "busaddr" == key )
    {
        busaddr = strtol( value.c_str(), NULL, 0 );
    }
}

HNSW_RESULT_T 
HNSWI2CExpander::initDevice()
{
    char buf[ 512 ];
    if( getModel() == "mcp23008" )
    {
        sprintf( buf, "i2-gpio-exp.mcp23008.%s.%x", getID().c_str(), busaddr );
        health.setComponent( buf );
        return mcp23008Init();
    }

    log.error( "The i2c device type is not supported: %d", getModel().c_str() ); 
    return HNSW_RESULT_FAILURE;
}

HNSW_RESULT_T 
HNSWI2CExpander::closeDevice()
{
    if( getModel() == "mcp23008" )
    {
        return mcp23008Close();
    }

    log.error( "The i2c device type is not supported: %d", getModel().c_str() ); 
    return HNSW_RESULT_FAILURE;
}

HNSW_RESULT_T 
HNSWI2CExpander::restartDevice()
{
    HNSW_RESULT_T result;

    result = closeDevice();

    if( result != HNSW_RESULT_SUCCESS )
        return result;

    return initDevice();
}

void 
HNSWI2CExpander::initNextState()
{
    nextState = 0;
}

HNSW_RESULT_T 
HNSWI2CExpander::updateStateToOn( std::string param )
{
    if( getModel() == "mcp23008" )
    {
        uint bitPos = strtol( param.c_str(), NULL, 0 );

        if( bitPos > 7 )
            return HNSW_RESULT_FAILURE;

        nextState |= (1 << bitPos);

        return HNSW_RESULT_SUCCESS;
    }

    log.error( "The i2c device type is not supported: %d", getModel().c_str() ); 
    return HNSW_RESULT_FAILURE;
}

HNSW_RESULT_T 
HNSWI2CExpander::applyNextState()
{
    if( getModel() == "mcp23008" )
    {
        HNSW_RESULT_T result;
        uint curState;

        // Clean up if already open
        if( i2cfd == 0 )
            return HNSW_RESULT_FAILURE;

        // Read the current state
        result = mcp23008GetPortState( curState );
   
        if( result != HNSW_RESULT_SUCCESS )
        {
            health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_READ_STATE, busaddr );
            return result;
        }

        // If nextState is different
        // then write the nextState value
        if( nextState != curState )
        {
            log.debug( "Applying new state to i2c exp %d:0x%x - %d", devnum, busaddr, nextState );
            result = mcp23008SetPortState( nextState );

            if( result != HNSW_RESULT_SUCCESS )
            {
                health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_WRITE_STATE, busaddr ); 
                return result;
            }
        }

        health.setOK();

        // No change
        return HNSW_RESULT_SUCCESS;
    }

    log.error( "The i2c device type is not supported: %d", getModel().c_str() ); 
    return HNSW_RESULT_FAILURE;
}

void
HNSWI2CExpander::debugPrint( uint offset )
{
    log.debug( "%*.*cid: %s  class: i2c-gpio-exp  model: %s  devnum: %d  busaddr: 0x%x  desc: %s", 
                     offset, offset, ' ', getID().c_str(), getModel().c_str(), devnum, busaddr, getDesc().c_str() );
}

bool 
HNSWI2CExpander::supportedModel( std::string model )
{
    if( model == "mcp23008" )
        return true;

    return false;
}

typedef enum MCP230xxRegisterAddresses
{
    MCP23008_IODIR  = 0x00,
    MCP23008_GPIO   = 0x09,
    MCP23008_GPPU   = 0x06,
    MCP23008_OLAT   = 0x0A,
}MCP_REG_ADDR;

HNSW_RESULT_T 
HNSWI2CExpander::mcp23008Init()
{
    char devfn[256];

    // Clean up if already open
    if( i2cfd != 0 )
        mcp23008Close();

    // Build the device string
    sprintf( devfn, "/dev/i2c-%d", devnum );
  
    log.info( "Initializing switch control device - model: %s  busaddr:  0x%x  devpath: %s", getModel().c_str(), busaddr, devfn );
 
    // Attempt to open the i2c device 
    if( ( i2cfd = open( devfn, O_RDWR ) ) < 0 ) 
    {
        log.error( "ERROR: Failure to open i2c bus device %s: %s", devfn, strerror( errno ) );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_I2CBUS_OPEN, devfn, strerror( errno ) );
        i2cfd = 0;
        return HNSW_RESULT_FAILURE;
    }

    // Tell the device which endpoint we want to talk to.
    if( ioctl( i2cfd, I2C_SLAVE, busaddr ) < 0 )
    {
        log.error( "ERROR: Failure to set target device to 0x%x for i2c bus %s: %s", busaddr, devfn, strerror( errno ) );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_SET_TARGET, busaddr, devfn, strerror( errno ) );       
        return HNSW_RESULT_FAILURE;
    }

    // Clear all of the outputs initially.
    if( mcp23008SetPortState( 0 ) != HNSW_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failure to zero port state for i2c addr 0x%x", busaddr );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_ZERO_STATE, busaddr ); 
        return HNSW_RESULT_FAILURE;
    }

    // Init the IO direction (inbound) and pullup (off) settings
    if( mcp23008SetPortMode( 0xFF ) != HNSW_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failure to set io direction to inbound for i2c addr 0x%x", busaddr );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_SET_INBOUND, busaddr );  
        return HNSW_RESULT_FAILURE;
    }

    if( mcp23008SetPortPullup( 0x00 ) != HNSW_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failure to disable pullups for i2c addr 0x%x", busaddr );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_SET_PULLUP, busaddr );   
        return HNSW_RESULT_FAILURE;
    }

    // Read and intial value from the device.
    uint currentState;
    if( mcp23008GetPortState( currentState ) != HNSW_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failure to read current state for i2c addr 0x%x", busaddr );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_READ_STATE, busaddr );   
        return HNSW_RESULT_FAILURE;
    }

    log.debug( "Initial value from 0x%x: %d\n", busaddr, currentState );

    // Turn all of the pins over to outputs
    if( mcp23008SetPortMode( 0x00 ) != HNSW_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failure to set io direction to outbound for i2c addr 0x%x", busaddr );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_MCP280XX_SET_IODIR, busaddr );  
        return HNSW_RESULT_FAILURE;
    }

    log.info( "Control device initialized - model: %s  busaddr:  0x%x  devpath: %s", getModel().c_str(), busaddr, devfn );

    health.setOK();

    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSWI2CExpander::mcp23008Close()
{
    if( i2cfd != 0 )
    {
        close( i2cfd );
        i2cfd = 0;
        log.info( "Control device close complete - model: %s  busaddr:  0x%x devnum: %d", getModel().c_str(), busaddr, devnum );
    }

    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSWI2CExpander::mcp23008GetPortMode( uint &value )
{
    int32_t result;

    if( i2cfd == 0 )
        return HNSW_RESULT_FAILURE;

    result = i2c_smbus_read_byte_data( i2cfd, MCP23008_IODIR );

    if( result < 0 )
        return HNSW_RESULT_FAILURE;

    value = ( result & 0xFF );

    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSWI2CExpander::mcp23008GetPortPullup( uint &value )
{
    int32_t result;

    if( i2cfd == 0 )
        return HNSW_RESULT_FAILURE;

    result = i2c_smbus_read_byte_data( i2cfd, MCP23008_GPPU );

    if( result < 0 )
        return HNSW_RESULT_FAILURE;

    value = ( result & 0xFF );

    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSWI2CExpander::mcp23008GetPortState( uint &value )
{
    int32_t result;

    if( i2cfd == 0 )
        return HNSW_RESULT_FAILURE;

    result = i2c_smbus_read_byte_data( i2cfd, MCP23008_GPIO );

    if( result < 0 )
        return HNSW_RESULT_FAILURE;

    value = ( result & 0xFF );

    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T
HNSWI2CExpander::mcp23008SetPortMode( uint value )
{
    int32_t result;

    if( i2cfd == 0 )
        return HNSW_RESULT_FAILURE;

    result = i2c_smbus_write_byte_data( i2cfd, MCP23008_IODIR, value );

    if( result < 0 )
    {
        log.debug( "Failure control device port mode update - busaddr: 0x%x, errno: %d", busaddr, abs(result) );
        return HNSW_RESULT_FAILURE;
    }

    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSWI2CExpander::mcp23008SetPortPullup( uint value )
{
    int32_t result;

    if( i2cfd == 0 )
        return HNSW_RESULT_FAILURE;

    result = i2c_smbus_write_byte_data( i2cfd, MCP23008_GPPU, value );

    if( result < 0 )
    {
        log.debug( "Failure control device port pullup update - busaddr: 0x%x, errno: %d", busaddr, abs(result) );
        return HNSW_RESULT_FAILURE;
    }

    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSWI2CExpander::mcp23008SetPortState( uint value )
{
    int32_t result;

    if( i2cfd == 0 )
        return HNSW_RESULT_FAILURE;

    result = i2c_smbus_write_byte_data( i2cfd, MCP23008_OLAT, value );

    if( result < 0 )
    {
        log.debug( "Failure control device state update - busaddr: 0x%x, errno: %d", busaddr, abs(result) );
        return HNSW_RESULT_FAILURE;
    }

    return HNSW_RESULT_SUCCESS;
}


HNSW_RESULT_T 
HNSWDeviceFactory::createDeviceForClass( std::string devClass, std::string devModel, HNSWDevice **devObj )
{
    *devObj = NULL;
    if( devClass == "i2c-gpio-exp" )
    {
        *devObj = new HNSWI2CExpander;
        (*devObj)->setModel( devModel );
        return HNSW_RESULT_SUCCESS;
    }

    return HNSW_RESULT_FAILURE;
}

void 
HNSWDeviceFactory::destroyDevice( HNSWDevice *devObj )
{
    if( devObj == NULL )
        return;

    delete devObj;
}

HNSWSwitch::HNSWSwitch()
{

}

HNSWSwitch::~HNSWSwitch()
{

}

void 
HNSWSwitch::setSWID( std::string value )
{
    swID = value;
}

void 
HNSWSwitch::setDesc( std::string value )
{
    desc = value;
}

void 
HNSWSwitch::setDevID( std::string value )
{
    devID = value;
}

void 
HNSWSwitch::setDevParam( std::string value )
{
    devParam = value;
}

std::string 
HNSWSwitch::getSWID()
{
    return swID;
}

std::string 
HNSWSwitch::getDesc()
{
    return desc;
}

std::string 
HNSWSwitch::getDevID()
{
    return devID;
}

std::string 
HNSWSwitch::getDevParam()
{
    return devParam;
}

void
HNSWSwitch::debugPrint( uint offset, HNDaemonLogSrc &log )
{
    log.debug( "%*.*cswID: %s  devID: %s  devParam: %s  desc: %s", 
                     offset, offset, ' ', swID.c_str(), devID.c_str(), devParam.c_str(), desc.c_str() );
}

HNSwitchManager::HNSwitchManager()
: health( "Switch Manager" )
{
    rootDirPath = HNSW_ROOT_DIRECTORY_DEFAULT; 
    notifySink = NULL;
}

HNSwitchManager::~HNSwitchManager()
{
}

void 
HNSwitchManager::setDstLog( HNDaemonLog *logPtr )
{
    log.setDstLog( logPtr );
}

void 
HNSwitchManager::setNotificationSink( HNSwitchManagerNotifications *sink )
{
    notifySink = sink;
}

void 
HNSwitchManager::clearNotificationSink()
{
   notifySink = NULL;
}

void 
HNSwitchManager::setRootDirectory( std::string path )
{
    rootDirPath = path;
}

std::string 
HNSwitchManager::getRootDirectory()
{
    return rootDirPath;
}

HNSW_RESULT_T 
HNSwitchManager::generateFilePath( std::string &fpath )
{
    char tmpBuf[ 256 ];
 
    fpath.clear();

    if( deviceName.empty() == true )
        return HNSW_RESULT_FAILURE;

    if( instanceName.empty() == true )
        return HNSW_RESULT_FAILURE;
  
    sprintf( tmpBuf, "%s-%s", deviceName.c_str(), instanceName.c_str() );

    Poco::Path path( rootDirPath );
    path.makeDirectory();
    path.pushDirectory( tmpBuf );
    path.setFileName("switchConfig.json");

    fpath = path.toString();

    return HNSW_RESULT_SUCCESS;
}

void
HNSwitchManager::clear()
{
    deviceName.clear();
    instanceName.clear();
}

HNSW_RESULT_T 
HNSwitchManager::loadConfiguration( std::string devname, std::string instance )
{
    std::string fpath;

    // Clear any existing configuration
    clear();

    // Copy over identifying information
    deviceName   = devname;
    instanceName = instance;

    // Generate and verify filename
    if( generateFilePath( fpath ) != HNSW_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failed to generate path to switch config for: %s %s", devname, instance );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SWM_FAILED_PATH_GEN, devname.c_str(), instance.c_str() ); 
        return HNSW_RESULT_FAILURE;
    }    

    // Build target file path and verify existance
    Poco::Path path( fpath );
    Poco::File file( path );

    log.info( "Attempting to load switch config from: %s", path.toString().c_str() );

    if( file.exists() == false || file.isFile() == false )
    {
        log.error( "ERROR: Switch config file does not exist: %s", path.toString().c_str() );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SWM_CONFIG_MISSING, path.toString().c_str() ); 
        return HNSW_RESULT_FAILURE;
    }

    // Open a stream for reading
    std::ifstream its;
    its.open( path.toString() );

    if( its.is_open() == false )
    {
        log.error( "ERROR: Switch config file open failed: %s", path.toString().c_str() );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SWM_CONFIG_OPEN, path.toString().c_str() ); 
        return HNSW_RESULT_FAILURE;
    }

    // Invoke the json parser
    try
    {
        // Attempt to parse the json    
        pjs::Parser parser;
        pdy::Var varRoot = parser.parse( its );
        its.close();

        // Get a pointer to the root object
        pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();

        // Handle each key of the root objects
        for( pjs::Object::ConstIterator it = jsRoot->begin(); it != jsRoot->end(); it++ )
        {
            if( it->first == "devList" )
            {
                if( jsRoot->isArray( it ) == false )
                {
                    log.warn( "Warning: Switch config unexpected non-array value for devList" );
                    continue;
                }

                pjs::Array::Ptr jsArr = jsRoot->getArray( it->first );

                for( uint index = 0; index < jsArr->size(); index++ )
                {
                     if( jsArr->isObject( index ) == true )
                     {
                         pjs::Object::Ptr jsDev = jsArr->getObject( index );

                         // Get class and model first so correct object can be created.
                         std::string empty;
                         std::string devClass = jsDev->optValue( "class", empty );
                         std::string devModel = jsDev->optValue( "model", empty );;

                         if( devClass.empty() || devModel.empty() )
                         {
                             // Missing required fields.
                             log.warn( "Warning: Switch config control device is missing required model and/or class fields." );
                             continue;
                         }
                         
                         HNSWDevice *devObj;
                         if( HNSWDeviceFactory::createDeviceForClass( devClass, devModel, &devObj ) != HNSW_RESULT_SUCCESS )
                         {
                             // Unsupported control device.
                             log.warn( "Warning: Switch config requested control device is not supported: %s, %s", devClass.c_str(), devModel.c_str() );
                             continue;
                         }  

                         // Pass through the log dst pointer                                    
                         devObj->setDstLog( log.getDstLog() );

                         for( pjs::Object::ConstIterator dit = jsDev->begin(); dit != jsDev->end(); dit++ )
                         {
                             if( dit->second.isString() == false )
                             {
                                 log.warn( "Warning: Switch config device object contains unexpected non-string value" );                            
                                 continue;
                             }
                                        
                             if( "id" == dit->first )
                             {
                                 devObj->setID( dit->second );
                             }
                             else if( "desc" == dit->first )
                             {
                                 devObj->setDesc( dit->second );
                             }
                             else
                             {
                                 devObj->parsePair( dit->first, dit->second );
                             }
                         }

                         deviceMap.insert( std::pair< std::string, HNSWDevice* >( devObj->getID(), devObj ) );
                     }
                     else
                     {
                         log.warn( "Warning: Switch config devList contains an unexpected non-object value" );                            
                     }
                }
            }
            else if( it->first == "switchList" )
            {
                if( jsRoot->isArray( it ) == false )
                {
                    log.warn( "Warning: Switch config unexpected non-array value for switchList" );
                    continue;
                }

                pjs::Array::Ptr jsArr = jsRoot->getArray( it->first );

                for( uint index = 0; index < jsArr->size(); index++ )
                {
                     if( jsArr->isObject( index ) == true )
                     {
                         pjs::Object::Ptr jsSwitch = jsArr->getObject( index );

                         HNSWSwitch nSW;
                                   
                         for( pjs::Object::ConstIterator dit = jsSwitch->begin(); dit != jsSwitch->end(); dit++ )
                         {
                             if( dit->second.isString() == false )
                             {
                                 log.warn( "Warning: Switch config switch definition contains unexpected non-string value" );
                                 continue;
                             }
                                        
                             if( "swid" == dit->first )
                             {
                                 nSW.setSWID( dit->second );
                             }
                             else if( "desc" == dit->first )
                             {
                                 nSW.setDesc( dit->second );
                             }
                             else if( "dev-id" == dit->first )
                             {
                                 nSW.setDevID( dit->second );
                             }
                             else if( "dev-param" == dit->first )
                             {
                                 nSW.setDevParam( dit->second );
                             }
                         }

                         switchMap.insert( std::pair< std::string, HNSWSwitch >( nSW.getSWID(), nSW ) );
                     }
                     else
                     {
                         log.warn( "Warning: Switch config switchList contains an unexpected non-object value" );                            
                     }
                }
            }
        }
    }
    catch( Poco::Exception ex )
    {
        log.error( "ERROR: Switch config file json parse failure: %s", ex.displayText().c_str() );
        health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SWM_CONFIG_PARSER, ex.displayText().c_str() ); 
        its.close();
        return HNSW_RESULT_FAILURE;
    }
    
    // 
    debugPrint();

    log.info( "Switch config successfully loaded." );

    // Set health to ok
    health.setOK();

    // Done
    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSwitchManager::getSwitchInfo( std::string &jsonMsg )
{
    std::stringstream msgStream;

    jsonMsg.clear();

    char tmpBuf[128];

    // Create a json root object
    pjs::Object jsRoot;

    // Create an array of switch info
    pjs::Array jsSWList;

    // Add a switch info object to the array for each switch
    for( std::map< std::string, HNSWSwitch >::iterator it = switchMap.begin(); it != switchMap.end(); it++ )
    {
        // Add overall health to output.
        pjs::Object jsSWInfo;

        jsSWInfo.set( "swid", it->second.getSWID() );
        jsSWInfo.set( "description", it->second.getDesc() );

        jsSWList.add( jsSWInfo );
    }

    // Add switch info array to the root object
    jsRoot.set( "swList", jsSWList );

    // Render into a json string.
    try
    {
        // Write out the generated json
        pjs::Stringifier::stringify( jsRoot, msgStream );
    }
    catch( ... )
    {
        // Something went wrong
        return HNSW_RESULT_FAILURE;
    }

    // Return to the caller.
    jsonMsg = msgStream.str();

    // Success
    return HNSW_RESULT_SUCCESS;
}

HNSW_RESULT_T 
HNSwitchManager::initDevices()
{
    HNSW_RESULT_T lastResult = HNSW_RESULT_SUCCESS;

    for( std::map< std::string, HNSWDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
    {
        HNSW_RESULT_T result = it->second->initDevice();
        if( result != HNSW_RESULT_SUCCESS )
            lastResult = result;
    }

    return lastResult;
}

HNSW_RESULT_T 
HNSwitchManager::closeDevices()
{
    HNSW_RESULT_T lastResult = HNSW_RESULT_SUCCESS;

    for( std::map< std::string, HNSWDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
    {
        HNSW_RESULT_T result = it->second->closeDevice();
        if( result != HNSW_RESULT_SUCCESS )
            lastResult = result;
    }

    return lastResult;
}

HNSW_RESULT_T 
HNSwitchManager::processOnState( std::vector< std::string > &swidOnList )
{
    // Begin state track on each device
    for( std::map< std::string, HNSWDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
        it->second->initNextState();

    // Walk provided list of ON switches
    for( std::vector< std::string >::iterator it = swidOnList.begin(); it != swidOnList.end(); it++ )
    {
        // Find the referenced switch record
        std::map< std::string, HNSWSwitch >::iterator sit = switchMap.find( *it );

        if( sit == switchMap.end() )
        {
            // Unknown switch - skip
            continue;
        }

        // Find the controlling device record
        std::map< std::string, HNSWDevice* >::iterator dit = deviceMap.find( sit->second.getDevID() );

        if( dit == deviceMap.end() )
        {
            // Unknown device - skip
            continue;
        }

        // Update the state change in the device to turn on the switch
        dit->second->updateStateToOn( sit->second.getDevParam() );
    } 

    // Apply any generated state changes
    for( std::map< std::string, HNSWDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
    {
        HNSW_RESULT_T result = it->second->applyNextState();
        if( result != HNSW_RESULT_SUCCESS )
        {
            log.error( "ERROR: Unable to update the control device state for device: %s", it->second->getID().c_str() );
        }
    }

    return HNSW_RESULT_SUCCESS;
}

uint 
HNSwitchManager::getHealthComponentCount()
{
    return ( 1 + deviceMap.size() );
}

HNDaemonHealth* 
HNSwitchManager::getHealthComponent( uint index )
{
    if( index == 0 )
        return &health;
    else if( index < getHealthComponentCount() )
    {
        index -= 1;
        for( std::map< std::string, HNSWDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
        {
            if( index == 0 )
                return it->second->getHealthComponent( 0 );

            index -= 1;
        }
    }

    return NULL;
}

void
HNSwitchManager::debugPrint()
{
    log.debug( "==== Control Device List: %s-%s ====", deviceName.c_str(), instanceName.c_str() );
 
    for( std::map< std::string, HNSWDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
    {
        (it->second)->debugPrint( 2 );
    }

    log.debug( "==== Control Switch List: %s-%s ====", deviceName.c_str(), instanceName.c_str() );
    for( std::map< std::string, HNSWSwitch >::iterator it = switchMap.begin(); it != switchMap.end(); it++ )
    {
        it->second.debugPrint( 2, log );
    }
}

