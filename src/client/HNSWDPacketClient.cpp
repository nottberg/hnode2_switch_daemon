#include <string.h>
#include <arpa/inet.h>

#include "HNSWDPacketClient.h"

HNSWDMsgBuffer::HNSWDMsgBuffer()
{
    bufPtr = NULL;
    allocLen = 0;
}

HNSWDMsgBuffer::~HNSWDMsgBuffer()
{
    if( bufPtr )
        free( bufPtr );
}

HNSWDP_RESULT_T 
HNSWDMsgBuffer::ensureBufferExists( uint maxlen )
{
    if( allocLen >= maxlen )
        return HNSWDP_RESULT_SUCCESS;

    allocLen = (maxlen & ~0xFFF) + 0x1000;

    void *tmpPtr = realloc( bufPtr, allocLen );

    if( tmpPtr == NULL )
    {
        if( bufPtr != NULL )
            free( bufPtr );
        bufPtr   = NULL;
        allocLen = 0;
        return HNSWDP_RESULT_FAILURE;
    }

    bufPtr = (uint8_t *) tmpPtr;

    return HNSWDP_RESULT_SUCCESS;
}

uint8_t*
HNSWDMsgBuffer::buffer()
{
    return bufPtr;
}


HNSWDPacketClient::HNSWDPacketClient()
{
    pktHdr.type = HNSWD_PTYPE_NOTSET;
    pktHdr.resultCode = HNSWD_RCODE_NOTSET;
    pktHdr.msgLen = 0;
}

HNSWDPacketClient::HNSWDPacketClient( HNSWD_PTYPE_T type, HNSWD_RCODE_T result )
{
    setType( type );
    setResult( result );
}

HNSWDPacketClient::HNSWDPacketClient( HNSWD_PTYPE_T type, HNSWD_RCODE_T result, std::string msg )
{
    setType( type );
    setResult( result );
    setMsg( msg );
}

HNSWDPacketClient::~HNSWDPacketClient()
{

}

void 
HNSWDPacketClient::setType( HNSWD_PTYPE_T value )
{
    pktHdr.type = value;
}

HNSWD_PTYPE_T 
HNSWDPacketClient::getType()
{
    return (HNSWD_PTYPE_T) pktHdr.type;
}

void 
HNSWDPacketClient::setResult( HNSWD_RCODE_T value )
{
    pktHdr.resultCode = value;
}

HNSWD_RCODE_T 
HNSWDPacketClient::getResult()
{
    return (HNSWD_RCODE_T) pktHdr.resultCode;
}

void 
HNSWDPacketClient::setMsg( std::string value )
{
    pktHdr.msgLen = value.size();
    msgData.ensureBufferExists( pktHdr.msgLen );
    memcpy( msgData.buffer(), value.c_str(), pktHdr.msgLen );
}

uint8_t*
HNSWDPacketClient::getMsg( uint &msgLen )
{
    msgLen = pktHdr.msgLen;
    return msgData.buffer();
}

void 
HNSWDPacketClient::getMsg( std::string &msgStr )
{
    msgStr.assign( (const char *)msgData.buffer(), pktHdr.msgLen );
}

HNSWDP_RESULT_T 
HNSWDPacketClient::rcvHeader( int sockfd )
{
    int bytesRX = recv( sockfd, &pktHdr, sizeof( pktHdr ), 0 );

    if( bytesRX != sizeof( pktHdr ) )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    // Allocate space for the message data.
    msgData.ensureBufferExists( pktHdr.msgLen );

    return HNSWDP_RESULT_SUCCESS;
}

HNSWDP_RESULT_T 
HNSWDPacketClient::rcvPayload( int sockfd )
{
    int bytesRX;

    if( pktHdr.msgLen == 0 )
        return HNSWDP_RESULT_SUCCESS;

    while( 1 )
    {
        bytesRX = recv( sockfd, msgData.buffer(), pktHdr.msgLen, 0 );

        if( bytesRX < 0 )
        {
            if( errno == EAGAIN )
                continue;

            return HNSWDP_RESULT_FAILURE;
        }

        break;
    }

    if( bytesRX != pktHdr.msgLen )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    return HNSWDP_RESULT_SUCCESS;
}

HNSWDP_RESULT_T 
HNSWDPacketClient::sendAll( int sockfd )
{
    int length;

    length = send( sockfd, &pktHdr, sizeof( pktHdr ), MSG_NOSIGNAL );

    printf( "Send1: %d\n", length );

    if( length != sizeof( pktHdr ) )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    if( pktHdr.msgLen == 0 )
        return HNSWDP_RESULT_SUCCESS;

    length = send( sockfd, msgData.buffer(), pktHdr.msgLen, MSG_NOSIGNAL );

    printf( "Send2: %d\n", length );

    if( length != pktHdr.msgLen )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    return HNSWDP_RESULT_SUCCESS; 
}


