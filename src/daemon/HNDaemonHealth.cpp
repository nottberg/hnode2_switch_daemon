#include "HNDaemonHealth.h"

HNDaemonHealth::HNDaemonHealth()
{
    status = HN_HEALTH_UNKNOWN;
}

HNDaemonHealth::HNDaemonHealth( std::string cvalue )
{
    component = cvalue;
    status = HN_HEALTH_UNKNOWN;
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
    setStatus( HN_HEALTH_OK );
}

void 
HNDaemonHealth::setStatus( HN_HEALTH_T value )
{
    if( value == HN_HEALTH_OK )
        msg.clear();

    status = value;
}

void 
HNDaemonHealth::setMsg( std::string value )
{
    msg = value;
}

void 
HNDaemonHealth::setStatusMsg( HN_HEALTH_T stat, std::string msgVal )
{
    status = stat;
    msg = msgVal;
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

std::string 
HNDaemonHealth::getMsg()
{
    return msg;
}

