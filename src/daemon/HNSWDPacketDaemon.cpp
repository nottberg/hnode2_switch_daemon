#include <arpa/inet.h>

#include "HNSWDPacketDaemon.h"

HNSWDPacketDaemon::HNSWDPacketDaemon()
{
    pktHdr.type = HNSWD_PTYPE_NOTSET;
    pktHdr.resultCode = HNSWD_RCODE_NOTSET;
    pktHdr.msgLen = 0;
}

HNSWDPacketDaemon::HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result )
{

}

HNSWDPacketDaemon::HNSWDPacketDaemon( HNSWD_PTYPE_T type, HNSWD_RCODE_T result, std::string msg )
{

}

HNSWDPacketDaemon::~HNSWDPacketDaemon()
{

}

void 
HNSWDPacketDaemon::setType( HNSWD_PTYPE_T value )
{

}

HNSWD_PTYPE_T 
HNSWDPacketDaemon::getType()
{

}

void 
HNSWDPacketDaemon::setResult( HNSWD_RCODE_T value )
{

}

HNSWD_RCODE_T 
HNSWDPacketDaemon::getResult()
{

}

void 
HNSWDPacketDaemon::setMsg( std::string value )
{

}

std::string&
HNSWDPacketDaemon::getMsgRef()
{

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

