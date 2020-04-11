#include <stdarg.h>

#include "HNDaemonHealth.h"

// Must be kept in sync with the error codes
// in the shared header file.
static const char *sErrorCodeFormatStrings[] =
{
    "",                                                             // HNSWD_ERRCODE_NO_ERROR
    "Internal Software Error",                                      // HNSWD_ERRCODE_INTERNAL_FAILURE

    "Failed to generate path to schedule matrix config for: %s %s", // HNSWD_ECODE_SM_FAILED_PATH_GEN
    "Schedule matrix config file does not exist: %s",               // HNSWD_ECODE_SM_CONFIG_MISSING
    "Schedule matrix config file open failed: %s",                  // HNSWD_ECODE_SM_CONFIG_OPEN
    "Schedule matrix config file json parse failure: %s",           // HNSWD_ECODE_SM_CONFIG_PARSER

    "Failed to generate path to switch manager config for: %s %s",  // HNSWD_ECODE_SWM_FAILED_PATH_GEN
    "Switch manager config file does not exist: %s",                // HNSWD_ECODE_SWM_CONFIG_MISSING
    "Switch manager config file open failed: %s",                   // HNSWD_ECODE_SWM_CONFIG_OPEN
    "Switch manager config file json parse failure: %s",            // HNSWD_ECODE_SWM_CONFIG_PARSER

    "Failure to open i2c bus device %s: %s",                        // HNSWD_ECODE_MCP280XX_I2CBUS_OPEN  
    "Failure to set target device to 0x%x for i2c bus %s: %s",      // HNSWD_ECODE_MCP280XX_SET_TARGET    
    "Failure to zero port state for i2c addr 0x%x",                 // HNSWD_ECODE_MCP280XX_ZERO_STATE    
    "Failure to set io direction to inbound for i2c addr 0x%x",     // HNSWD_ECODE_MCP280XX_SET_INBOUND    
    "Failure to disable pullups for i2c addr 0x%x",                 // HNSWD_ECODE_MCP280XX_SET_PULLUP    
    "Failure to read state for i2c addr 0x%x",                      // HNSWD_ECODE_MCP280XX_READ_STATE   
    "Failure to set io direction to outbound for i2c addr 0x%x",    // HNSWD_ECODE_MCP280XX_SET_OUTBOUND 
    "Failure to write state to i2c addr 0x%x",                      // HNSWD_ECODE_MCP280XX_WRITE_STATE
    "One or more of the daemon subcomponents is in a failing state.", // HNSWD_ECODE_SUBCOMPONENT
};

HNDaemonHealth::HNDaemonHealth()
{
    status    = HN_HEALTH_UNKNOWN;
    errorCode = HNSWD_ERRCODE_NO_ERROR;
}

HNDaemonHealth::HNDaemonHealth( std::string cvalue )
{
    component = cvalue;
    status    = HN_HEALTH_UNKNOWN;
    errorCode = HNSWD_ERRCODE_NO_ERROR;
}

HNDaemonHealth::~HNDaemonHealth()
{

}

void 
HNDaemonHealth::setComponent( std::string value )
{
    component = value;
}

void 
HNDaemonHealth::setOK()
{
    status = HN_HEALTH_OK;
    errorCode = HNSWD_ERRCODE_NO_ERROR;
    msg.clear();
}

void 
HNDaemonHealth::setStatusMsg( HN_HEALTH_T stat, HNSWD_ERRCODE_T err, ... )
{
    char buf[1024];
    va_list args;

    status  = stat;
    errorCode = err;

    va_start( args, err );

    vsnprintf( buf, sizeof( buf ), sErrorCodeFormatStrings[ errorCode ], args );

    msg.assign( buf );

    va_end( args );
}

std::string 
HNDaemonHealth::getComponent()
{
    return component;
}

HN_HEALTH_T 
HNDaemonHealth::getStatus()
{
    return status;
}

static const char *sHStatusStrs[] = {
"UNKNOWN",  // HN_HEALTH_UNKNOWN
"OK",       // HN_HEALTH_OK
"DEGRADED", // HN_HEALTH_DEGRADED
"FAILED"    // HN_HEALTH_FAILED
};

std::string 
HNDaemonHealth::getStatusStr()
{
    return sHStatusStrs[ status ];
}

HNSWD_ERRCODE_T 
HNDaemonHealth::getErrorCode()
{
    return errorCode;
}

std::string 
HNDaemonHealth::getMsg()
{
    return msg;
}

