#ifndef __HN_SWD_PACKET_DAEMON_H__
#define __HN_SWD_PACKET_DAEMON_H__

#include <stdint.h>

#include <string>

#include "HNSWDPacket.h"

class HNSWDPacketDaemon
{
    private:
        HNSWD_PKTHDR_T pktHdr;
        std::string    msgData;

    public:
        HNSWDPacketDaemon();
        HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result );
        HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result, std::string msg );
       ~HNSWDPacketDaemon();

        void setType( HNSWD_PTYPE_T value );
        HNSWD_PTYPE_T getType();

        void setResult( HNSWD_RCODE_T value );
        HNSWD_RCODE_T getResult();

        void setMsg( std::string value );
        std::string &getMsgRef();
};

#endif // __HN_SWD_PACKET_DAEMON_H__
