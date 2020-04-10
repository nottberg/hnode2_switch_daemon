#ifndef __HN_DAEMON_HEALTH_H__
#define __HN_DAEMON_HEALTH_H__

#include <string>

#include "HNSWDPacket.h"

class HNDaemonHealth
{
    private:
        HN_HEALTH_T status;
        std::string component;
        std::string msg;

    public:
        HNDaemonHealth();
        HNDaemonHealth( std::string component );
       ~HNDaemonHealth();

        void setComponent( std::string value );

        void setOK();
        void setStatus( HN_HEALTH_T value );
        void setStatusMsg( HN_HEALTH_T stat, std::string msgVal );

        void setMsg( std::string value );

        std::string getComponent();
        HN_HEALTH_T getStatus();
        std::string getStatusStr();
        std::string getMsg();
};

#endif // __HN_DAEMON_HEALTH_H__
