#ifndef __HN_SWD_PACKET_CLIENT_H__
#define __HN_SWD_PACKET_CLIENT_H__

#include <stdint.h>

#include <string>

#include "HNSWDPacket.h"

class HNSWDPacketClient
{
    private:
        HNSWD_PKTHDR_T pktHdr;
        std::string    msgData;

    public:
        HNSWDPacketClient();
        HNSWDPacketClient( HNSWD_PTYPE_T type, HNSWD_RCODE_T result );
        HNSWDPacketClient( HNSWD_PTYPE_T type, HNSWD_RCODE_T result, std::string msg );
       ~HNSWDPacketClient();

        void setType( HNSWD_PTYPE_T value );
        HNSWD_PTYPE_T getType();

        void setResult( HNSWD_RCODE_T value );
        HNSWD_RCODE_T getResult();

        void setMsg( std::string value );
        std::string &getMsgRef();
};

#endif // __HN_SWD_PACKET_CLIENT_H__

