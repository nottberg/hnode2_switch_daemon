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

#include "SwitchManager.h"

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

SWMDevice::SWMDevice()
{

}

SWMDevice::~SWMDevice()
{

}

void 
SWMDevice::setModel( std::string value )
{
    model = value;
}

void 
SWMDevice::setID( std::string value )
{
    id = value;
}

void 
SWMDevice::setDesc( std::string value )
{
    desc = value;
}

std::string 
SWMDevice::getModel()
{
    return model;
}

std::string 
SWMDevice::getID()
{
    return id;
}

std::string 
SWMDevice::getDesc()
{
    return desc;
}

SWMI2CExpander::SWMI2CExpander()
{
    devnum  = 0;
    busaddr = 0;
    i2cfd   = 0;
}

SWMI2CExpander::~SWMI2CExpander()
{
    closeDevice();
}

void 
SWMI2CExpander::parsePair( std::string key, std::string value )
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

SWM_RESULT_T 
SWMI2CExpander::initDevice()
{
    if( getModel() == "mcp23008" )
    {
        return mcp23008Init();
    }

    return SWM_RESULT_FAILURE;
}

SWM_RESULT_T 
SWMI2CExpander::closeDevice()
{
    if( getModel() == "mcp23008" )
    {
        return mcp23008Close();
    }

    return SWM_RESULT_FAILURE;
}

SWM_RESULT_T 
SWMI2CExpander::restartDevice()
{
    SWM_RESULT_T result;

    result = closeDevice();

    if( result != SWM_RESULT_SUCCESS )
        return result;

    return initDevice();
}

void 
SWMI2CExpander::initNextState()
{
    nextState = 0;
}

SWM_RESULT_T 
SWMI2CExpander::updateStateToOn( std::string param )
{
    if( getModel() == "mcp23008" )
    {
        uint bitPos = strtol( param.c_str(), NULL, 0 );

        if( bitPos > 7 )
            return SWM_RESULT_FAILURE;

        nextState |= (1 << bitPos);

        return SWM_RESULT_SUCCESS;
    }

    return SWM_RESULT_FAILURE;
}

SWM_RESULT_T 
SWMI2CExpander::applyNextState()
{
    if( getModel() == "mcp23008" )
    {
        SWM_RESULT_T result;
        uint curState;

        // Read the current state
        result = mcp23008GetPortState( curState );
   
        if( result != SWM_RESULT_SUCCESS )
            return result;

        // If nextState is different
        // then write the nextState value
        if( nextState != curState )
        {
            return mcp23008SetPortState( nextState );
        }

        // No change
        return SWM_RESULT_SUCCESS;
    }

    return SWM_RESULT_FAILURE;
}

void
SWMI2CExpander::debugPrint( uint offset )
{
    std::cout << std::setw( offset ) << " ";
    std::cout << "id: " << getID();
    std::cout << "  class: " << "i2c-gpio-exp";
    std::cout << "  model: " << getModel();
    std::cout << "  devnum: " << devnum;
    std::cout << "  busaddr: " << busaddr;
    std::cout << "  desc: " << getDesc();
    std::cout << std::endl;
}

bool 
SWMI2CExpander::supportedModel( std::string model )
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

SWM_RESULT_T 
SWMI2CExpander::mcp23008Init()
{
    char devfn[256];

    // Clean up if already open
    if( i2cfd != 0 )
        mcp23008Close();

    // Build the device string
    sprintf( devfn, "/dev/i2c-%d", devnum );
  
    printf( "mcp23008 start: %s, 0x%x\n", devfn, busaddr );

    // Attempt to open the i2c device 
    if( ( i2cfd = open( devfn, O_RDWR ) ) < 0 ) 
    {
        printf( "Failed to acquire bus access and/or talk to slave.\n" ); 
        perror("Failed to open the i2c bus");
        return SWM_RESULT_FAILURE;
    }

    // Tell the device which endpoint we want to talk to.
    if( ioctl( i2cfd, I2C_SLAVE, busaddr ) < 0 )
    {
        printf( "Failed to acquire bus access and/or talk to slave.\n" );
        /* ERROR HANDLING; you can check errno to see what went wrong */
        return SWM_RESULT_FAILURE;
    }

    // Clear all of the outputs initially.
    if( mcp23008SetPortState( 0 ) != SWM_RESULT_SUCCESS )
    {
        return SWM_RESULT_FAILURE;
    }

    // Init the IO direction (inbound) and pullup (off) settings
    if( mcp23008SetPortMode( 0xFF ) != SWM_RESULT_SUCCESS )
    {
        return SWM_RESULT_FAILURE;
    }

    if( mcp23008SetPortPullup( 0x00 ) != SWM_RESULT_SUCCESS )
    {
        return SWM_RESULT_FAILURE;
    }

    // Read and intial value from the device.
    uint currentState;
    if( mcp23008GetPortState( currentState ) != SWM_RESULT_SUCCESS )
    {
        return SWM_RESULT_FAILURE;
    }

    printf( "Initial Value Read: %d\n", currentState );

    // Turn all of the pins over to outputs
    if( mcp23008SetPortMode( 0x00 ) != SWM_RESULT_SUCCESS )
    {
        return SWM_RESULT_FAILURE;
    }

    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T 
SWMI2CExpander::mcp23008Close()
{
    if( i2cfd != 0 )
    {
        close( i2cfd );
        i2cfd = 0;
    }

    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T 
SWMI2CExpander::mcp23008GetPortMode( uint &value )
{
    int32_t result;

    result = i2c_smbus_read_byte_data( i2cfd, MCP23008_IODIR );

    if( result < 0 )
        return SWM_RESULT_FAILURE;

    value = ( result & 0xFF );

    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T 
SWMI2CExpander::mcp23008GetPortPullup( uint &value )
{
    int32_t result;

    result = i2c_smbus_read_byte_data( i2cfd, MCP23008_GPPU );

    if( result < 0 )
        return SWM_RESULT_FAILURE;

    value = ( result & 0xFF );

    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T 
SWMI2CExpander::mcp23008GetPortState( uint &value )
{
    int32_t result;

    result = i2c_smbus_read_byte_data( i2cfd, MCP23008_GPIO );

    if( result < 0 )
        return SWM_RESULT_FAILURE;

    value = ( result & 0xFF );

    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T
SWMI2CExpander::mcp23008SetPortMode( uint value )
{
    int32_t result;

    result = i2c_smbus_write_byte_data( i2cfd, MCP23008_IODIR, value );

    if( result < 0 )
        return SWM_RESULT_FAILURE;

    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T 
SWMI2CExpander::mcp23008SetPortPullup( uint value )
{
    int32_t result;

    result = i2c_smbus_write_byte_data( i2cfd, MCP23008_GPPU, value );

    if( result < 0 )
        return SWM_RESULT_FAILURE;

    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T 
SWMI2CExpander::mcp23008SetPortState( uint value )
{
    int32_t result;

    result = i2c_smbus_write_byte_data( i2cfd, MCP23008_OLAT, value );

    if( result < 0 )
        return SWM_RESULT_FAILURE;

    return SWM_RESULT_SUCCESS;
}


SWM_RESULT_T 
SWMDeviceFactory::createDeviceForClass( std::string devClass, std::string devModel, SWMDevice **devObj )
{
    *devObj = NULL;
    if( devClass == "i2c-gpio-exp" )
    {
        *devObj = new SWMI2CExpander;
        (*devObj)->setModel( devModel );
        return SWM_RESULT_SUCCESS;
    }

    return SWM_RESULT_FAILURE;
}

void 
SWMDeviceFactory::destroyDevice( SWMDevice *devObj )
{
    if( devObj == NULL )
        return;

    delete devObj;
}

SWMSwitch::SWMSwitch()
{

}

SWMSwitch::~SWMSwitch()
{

}

void 
SWMSwitch::setSWID( std::string value )
{
    swID = value;
}

void 
SWMSwitch::setDesc( std::string value )
{
    desc = value;
}

void 
SWMSwitch::setDevID( std::string value )
{
    devID = value;
}

void 
SWMSwitch::setDevParam( std::string value )
{
    devParam = value;
}

std::string 
SWMSwitch::getSWID()
{
    return swID;
}

std::string 
SWMSwitch::getDesc()
{
    return desc;
}

std::string 
SWMSwitch::getDevID()
{
    return devID;
}

std::string 
SWMSwitch::getDevParam()
{
    return devParam;
}

void
SWMSwitch::debugPrint( uint offset )
{
    std::cout << std::setw( offset ) << " ";
    std::cout << "swID: " << swID;
    std::cout << "  devID: " << devID;
    std::cout << "  devParam: " << devParam;
    std::cout << "  desc: " << desc;
    std::cout << std::endl;
}

SwitchManager::SwitchManager()
{
    rootDirPath = SWM_ROOT_DIRECTORY_DEFAULT; 
    notifySink = NULL;
}

SwitchManager::~SwitchManager()
{
}

void 
SwitchManager::setNotificationSink( SwitchManagerNotifications *sink )
{
    notifySink = sink;
}

void 
SwitchManager::clearNotificationSink()
{
   notifySink = NULL;
}

void 
SwitchManager::setRootDirectory( std::string path )
{
    rootDirPath = path;
}

std::string 
SwitchManager::getRootDirectory()
{
    return rootDirPath;
}

SWM_RESULT_T 
SwitchManager::generateFilePath( std::string &fpath )
{
    char tmpBuf[ 256 ];
 
    fpath.clear();

    if( deviceName.empty() == true )
        return SWM_RESULT_FAILURE;

    if( instanceName.empty() == true )
        return SWM_RESULT_FAILURE;
  
    sprintf( tmpBuf, "%s-%s", deviceName.c_str(), instanceName.c_str() );

    Poco::Path path( rootDirPath );
    path.makeDirectory();
    path.pushDirectory( tmpBuf );
    path.setFileName("switchConfig.json");

    fpath = path.toString();

    return SWM_RESULT_SUCCESS;
}

void
SwitchManager::clear()
{
    deviceName.clear();
    instanceName.clear();
}

SWM_RESULT_T 
SwitchManager::loadConfiguration( std::string devname, std::string instance )
{
    std::string fpath;

    // Clear any existing configuration
    clear();

    // Copy over identifying information
    deviceName   = devname;
    instanceName = instance;

    // Generate and verify filename
    if( generateFilePath( fpath ) != SWM_RESULT_SUCCESS )
    {
        return SWM_RESULT_FAILURE;
    }    

    // Build target file path and verify existance
    Poco::Path path( fpath );
    Poco::File file( path );

    if( file.exists() == false || file.isFile() == false )
    {
        return SWM_RESULT_FAILURE;
    }

    // Open a stream for reading
    std::ifstream its;
    its.open( path.toString() );

    if( its.is_open() == false )
    {
        return SWM_RESULT_FAILURE;
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
                    continue;

                pjs::Array::Ptr jsArr = jsRoot->getArray( it->first );

                for( uint index = 0; index < jsArr->size(); index++ )
                {
                     std::cout << "devindx: " << index << std::endl;
                     if( jsArr->isObject( index ) == true )
                     {
                         pjs::Object::Ptr jsDev = jsArr->getObject( index );

                         // Get class and model first so correct object can be created.
                         std::string empty;
                         std::string devClass = jsDev->optValue( "class", empty );
                         std::string devModel = jsDev->optValue( "model", empty );;

                         std::cout << "class: " << devClass << "  model: " << devModel << std::endl;
                         if( devClass.empty() || devModel.empty() )
                         {
                             // Missing required fields.
                             continue;
                         }
                         
                         SWMDevice *devObj;
                         if( SWMDeviceFactory::createDeviceForClass( devClass, devModel, &devObj ) != SWM_RESULT_SUCCESS )
                         {
                             // Unsupported control device.
                             continue;
                         }  
                                    
                         for( pjs::Object::ConstIterator dit = jsDev->begin(); dit != jsDev->end(); dit++ )
                         {
                             std::cout << "dev key: " << dit->first << std::endl;

                             if( dit->second.isString() == false )
                                 continue;
                                        
                             if( "id" == dit->first )
                             {
                                 std::cout << "dev id: " << dit->second.toString() << std::endl;
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

                         std::cout << "dev insert: " << devObj->getID() << std::endl;
                         deviceMap.insert( std::pair< std::string, SWMDevice* >( devObj->getID(), devObj ) );
                     }
                     else
                     {
                         // Unrecognized config element at array layer
                     }
                }
            }
            else if( it->first == "switchList" )
            {
                if( jsRoot->isArray( it ) == false )
                    continue;

                pjs::Array::Ptr jsArr = jsRoot->getArray( it->first );

                for( uint index = 0; index < jsArr->size(); index++ )
                {
                     if( jsArr->isObject( index ) == true )
                     {
                         pjs::Object::Ptr jsSwitch = jsArr->getObject( index );

                         SWMSwitch nSW;
                                   
                         for( pjs::Object::ConstIterator dit = jsSwitch->begin(); dit != jsSwitch->end(); dit++ )
                         {
                             if( dit->second.isString() == false )
                                 continue;
                                        
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

                         switchMap.insert( std::pair< std::string, SWMSwitch >( nSW.getSWID(), nSW ) );
                     }
                     else
                     {
                         // Unrecognized config element at array layer
                     }
                }
            }
        }
    }
    catch( Poco::Exception ex )
    {
        its.close();
        return SWM_RESULT_FAILURE;
    }
    
    // 
    debugPrint();

    // Done
    return SWM_RESULT_SUCCESS;
}

SWM_RESULT_T 
SwitchManager::initDevices()
{
    SWM_RESULT_T lastResult = SWM_RESULT_SUCCESS;

    for( std::map< std::string, SWMDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
    {
        SWM_RESULT_T result = it->second->initDevice();
        if( result != SWM_RESULT_SUCCESS )
            lastResult = result;
    }

    return lastResult;
}

SWM_RESULT_T 
SwitchManager::closeDevices()
{
    SWM_RESULT_T lastResult = SWM_RESULT_SUCCESS;

    for( std::map< std::string, SWMDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
    {
        SWM_RESULT_T result = it->second->closeDevice();
        if( result != SWM_RESULT_SUCCESS )
            lastResult = result;
    }

    return lastResult;
}

SWM_RESULT_T 
SwitchManager::processOnState( std::vector< std::string > &swidOnList )
{
    // Begin state track on each device
    for( std::map< std::string, SWMDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
        it->second->initNextState();

    // Walk provided list of ON switches
    for( std::vector< std::string >::iterator it = swidOnList.begin(); it != swidOnList.end(); it++ )
    {
        // Find the referenced switch record
        std::map< std::string, SWMSwitch >::iterator sit = switchMap.find( *it );

        if( sit == switchMap.end() )
        {
            // Unknown switch - skip
            continue;
        }

        // Find the controlling device record
        std::map< std::string, SWMDevice* >::iterator dit = deviceMap.find( sit->second.getDevID() );

        if( dit == deviceMap.end() )
        {
            // Unknown device - skip
            continue;
        }

        // Update the state change in the device to turn on the switch
        dit->second->updateStateToOn( sit->second.getDevParam() );
    } 

    // Apply any generated state changes
    for( std::map< std::string, SWMDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
        it->second->applyNextState();

    return SWM_RESULT_SUCCESS;
}

unsigned int 
SwitchManager::getSwitchCount()
{
    return switchMap.size();
}

SWMSwitch *
SwitchManager::getSwitchByIndex( int index )
{
    uint i = 0;
    for( std::map< std::string, SWMSwitch >::iterator it = switchMap.begin(); it != switchMap.end(); it++ )
    {
        if( i == index )
            return &(it->second);
        i++;
    }

    return NULL;
}

SWMSwitch *
SwitchManager::getSwitchByID( std::string switchID )
{
    std::map< std::string, SWMSwitch >::iterator it = switchMap.find( switchID );

    if( it == switchMap.end() )
        return NULL;

    return &(it->second);
}

void
SwitchManager::debugPrint()
{
    std::cout << "==== Control Device List: " << deviceName << "-" << instanceName << " ====" << std::endl;
    for( std::map< std::string, SWMDevice* >::iterator it = deviceMap.begin(); it != deviceMap.end(); it++ )
    {
        (it->second)->debugPrint( 2 );
    }

    std::cout << "==== Control Switch List: " << deviceName << "-" << instanceName << " ====" << std::endl;
    for( std::map< std::string, SWMSwitch >::iterator it = switchMap.begin(); it != switchMap.end(); it++ )
    {
        it->second.debugPrint( 2 );
    }
}

