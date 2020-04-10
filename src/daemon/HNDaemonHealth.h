#ifndef __HN_DAEMON_HEALTH_H__
#define __HN_DAEMON_HEALTH_H__

#include <cstdarg>
#include <string>

#include "HNSWDPacket.h"

class HNDaemonHealth
{
    private:
        HN_HEALTH_T     status;
        std::string     component;
        HNSWD_ERRCODE_T errorCode;
        std::string     msg;

    public:
        HNDaemonHealth();
        HNDaemonHealth( std::string component );
       ~HNDaemonHealth();

        void setComponent( std::string value );

        void setOK();
        void setStatusMsg( HN_HEALTH_T stat, HNSWD_ERRCODE_T errCode, ... );

        std::string     getComponent();
        HN_HEALTH_T     getStatus();
        std::string     getStatusStr();
        HNSWD_ERRCODE_T getErrCode();
        std::string     getMsg();
};

#endif // __HN_DAEMON_HEALTH_H__
