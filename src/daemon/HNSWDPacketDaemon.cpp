#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "HNSWDPacketDaemon.h"


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


HNSWDPacketDaemon::HNSWDPacketDaemon()
{
    pktHdr.type = HNSWD_PTYPE_NOTSET;
    pktHdr.resultCode = HNSWD_RCODE_NOTSET;
    pktHdr.msgLen = 0;
}

HNSWDPacketDaemon::HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result )
{
    setType( type );
    setResult( result );
}

HNSWDPacketDaemon::HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result, std::string msg )
{
    setType( type );
    setResult( result );
    setMsg( msg );
}

HNSWDPacketDaemon::~HNSWDPacketDaemon()
{

}

void 
HNSWDPacketDaemon::setType( HNSWD_PTYPE_T value )
{
    pktHdr.type = value;
}

HNSWD_PTYPE_T 
HNSWDPacketDaemon::getType()
{
    return (HNSWD_PTYPE_T) pktHdr.type;
}

void 
HNSWDPacketDaemon::setResult( HNSWD_RCODE_T value )
{
    pktHdr.resultCode = value;
}

HNSWD_RCODE_T 
HNSWDPacketDaemon::getResult()
{
    return (HNSWD_RCODE_T) pktHdr.resultCode;
}

void 
HNSWDPacketDaemon::setMsg( std::string value )
{
    pktHdr.msgLen = value.size();
    msgData.ensureBufferExists( pktHdr.msgLen );
    memcpy( msgData.buffer(), value.c_str(), pktHdr.msgLen );
}

uint8_t*
HNSWDPacketDaemon::getMsg( uint &msgLen )
{
    msgLen = pktHdr.msgLen;
    return msgData.buffer();
}

void 
HNSWDPacketDaemon::getMsg( std::string &msgStr )
{
    msgStr.assign( (const char *)msgData.buffer(), pktHdr.msgLen );
}

HNSWDP_RESULT_T 
HNSWDPacketDaemon::rcvHeader( int sockfd )
{
    int bytesRX = recv( sockfd, &pktHdr, sizeof( pktHdr ), 0 );

    if( bytesRX == 0 )
    {
        return HNSWDP_RESULT_NOPKT;
    }

    if( bytesRX != sizeof( pktHdr ) )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    // Allocate space for the message data.
    msgData.ensureBufferExists( pktHdr.msgLen );

    return HNSWDP_RESULT_SUCCESS;
}

HNSWDP_RESULT_T 
HNSWDPacketDaemon::rcvPayload( int sockfd )
{
    if( pktHdr.msgLen == 0 )
        return HNSWDP_RESULT_SUCCESS;

    int bytesRX = recv( sockfd, msgData.buffer(), pktHdr.msgLen, 0 );

    if( bytesRX != pktHdr.msgLen )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    return HNSWDP_RESULT_SUCCESS;
}

HNSWDP_RESULT_T 
HNSWDPacketDaemon::sendAll( int sockfd )
{
    int length;

    length = send( sockfd, &pktHdr, sizeof( pktHdr ), MSG_NOSIGNAL );

    if( length != sizeof( pktHdr ) )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    if( pktHdr.msgLen == 0 )
        return HNSWDP_RESULT_FAILURE;

    length = send( sockfd, msgData.buffer(), pktHdr.msgLen, MSG_NOSIGNAL );

    if( length != pktHdr.msgLen )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    return HNSWDP_RESULT_SUCCESS; 
}


