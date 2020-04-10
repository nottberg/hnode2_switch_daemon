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

    printf("AllocLen: %d\n", allocLen);

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

    printf( "RX Hdr bytes: %d\n", bytesRX );

    if( bytesRX == 0 )
    {
        return HNSWDP_RESULT_NOPKT;
    }

    if( bytesRX != sizeof( pktHdr ) )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    printf( "PktHdr - t: %d  s: %d  ml: %d\n", pktHdr.type, pktHdr.resultCode, pktHdr.msgLen );

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

    printf( "RX Payload bytes: %d\n", bytesRX );

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

    printf( "TX sent: %d\n", length );

    if( length != sizeof( pktHdr ) )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    length = send( sockfd, msgData.buffer(), pktHdr.msgLen, MSG_NOSIGNAL );

    if( length != pktHdr.msgLen )
    {
        return HNSWDP_RESULT_FAILURE;
    }

    return HNSWDP_RESULT_SUCCESS; 
}

#if 0

HNodeSWDPacket::HNodeSWDPacket()
{
    packetData.payloadLength = htonl( 0 );
}

HNodeSWDPacket::~HNodeSWDPacket()
{

}

void 
HNodeSWDPacket::setType( HNSWP_TYPE_T value )
{
    packetData.type = (HNSWP_TYPE_T) htonl(  value );
}

HNSWP_TYPE_T 
HNodeSWDPacket::getType()
{
    return (HNSWP_TYPE_T) ntohl( packetData.type );
}

void 
HNodeSWDPacket::setActionIndex( uint32_t value )
{
    packetData.actionIndex = htonl( value );
}

uint32_t
HNodeSWDPacket::getActionIndex()
{
    return ntohl( packetData.actionIndex );
}

void 
HNodeSWDPacket::setParam( int index, uint32_t value )
{
    if( index >= 5 )
        return;

    packetData.param[ index ] = htonl( value );
}

uint32_t
HNodeSWDPacket::getParam( int index )
{
    if( index >= 5 )
        return -1;

    return ntohl( packetData.param[ index ] );
}

void 
HNodeSWDPacket::setPldOffset( uint32_t offset )
{

}

void 
HNodeSWDPacket:: writePldStr( std::string value )
{

}

void 
HNodeSWDPacket:: writePldU8( uint8_t value )
{

}

void 
HNodeSWDPacket:: writePldU16( uint16_t value )
{

}

void 
HNodeSWDPacket:: writePldU32( uint32_t value )
{

}

std::string 
HNodeSWDPacket:: getPldStr()
{

}

uint8_t 
HNodeSWDPacket:: getPldU8()
{

}

uint16_t 
HNodeSWDPacket:: getPldU16()
{

}

uint32_t 
HNodeSWDPacket:: getPldU32()
{

}

void 
HNodeSWDPacket::setPayloadLength( uint32_t length )
{
    if( length > sizeof( packetData.payload ) )
        length = sizeof( packetData.payload );

    packetData.payloadLength = htonl( length );
}

uint32_t 
HNodeSWDPacket::getPayloadLength()
{
    return ntohl( packetData.payloadLength );
}

uint8_t* 
HNodeSWDPacket::getPayloadPtr()
{
    return packetData.payload;
}

uint32_t 
HNodeSWDPacket::getPacketLength()
{
    return getPayloadLength() + ( sizeof( uint32_t ) * 8 );
}

uint8_t* 
HNodeSWDPacket::getPacketPtr()
{
    return (uint8_t*) &packetData;
}

uint32_t 
HNodeSWDPacket::getMaxPacketLength()
{
    return (sizeof packetData);
}

#endif

