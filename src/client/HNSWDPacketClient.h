#ifndef __HN_SWD_PACKET_CLIENT_H__
#define __HN_SWD_PACKET_CLIENT_H__

#include <stdint.h>

#include <string>

#include "HNSwitchDaemon.h"

typedef enum HNSWDPacketClientResultEnum
{
    HNSWDP_RESULT_SUCCESS,
    HNSWDP_RESULT_FAILURE
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

class HNSWDPacketClient
{
    private:
        HNSWD_PKTHDR_T  pktHdr;
        HNSWDMsgBuffer  msgData;

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
        uint8_t* getMsg( uint &msgLen );
        void getMsg( std::string &msgStr );

        HNSWDP_RESULT_T rcvHeader( int sockfd );
        HNSWDP_RESULT_T rcvPayload( int sockfd );
        HNSWDP_RESULT_T sendAll( int sockfd );

};

#endif // __HN_SWD_PACKET_CLIENT_H__

