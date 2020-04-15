#ifndef __HN_SWD_PACKET_DAEMON_H__
#define __HN_SWD_PACKET_DAEMON_H__

#include <stdint.h>

#include <string>

#include "HNSWDPacket.h"

typedef enum HNSWDPacketDaemonResultEnum
{
    HNSWDP_RESULT_SUCCESS,
    HNSWDP_RESULT_FAILURE,
    HNSWDP_RESULT_NOPKT
}HNSWDP_RESULT_T;

class HNSWDMsgBuffer
{
    private:
        uint8_t  *bufPtr;
        uint      allocLen;

    public:
        HNSWDMsgBuffer();
       ~HNSWDMsgBuffer();

        HNSWDP_RESULT_T ensureBufferExists( uint maxlen );

        uint8_t *buffer();
};

class HNSWDPacketDaemon
{
    private:
        HNSWD_PKTHDR_T  pktHdr;
        HNSWDMsgBuffer  msgData;

    public:
        HNSWDPacketDaemon();
        HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result );
        HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result, std::string msg );
       ~HNSWDPacketDaemon();

        void setType( HNSWD_PTYPE_T value );
        HNSWD_PTYPE_T getType();

        void setResult( HNSWD_RCODE_T value );
        HNSWD_RCODE_T getResult();

        uint getMsgLen();

        void setMsg( std::string value );
        uint8_t* getMsg( uint &msgLen );
        void getMsg( std::string &msgStr );

        HNSWDP_RESULT_T rcvHeader( int sockfd );
        HNSWDP_RESULT_T rcvPayload( int sockfd );
        HNSWDP_RESULT_T sendAll( int sockfd );

};

#endif // __HN_SWD_PACKET_DAEMON_H__
