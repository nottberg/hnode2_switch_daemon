#ifndef __HNODE_SW_PACKET_H__
#define __HNODE_SW_PACKET_H__

#include <stdint.h>

#include <string>

typedef enum HNodeSWDPacketTypeEnum
{
    HNSWP_TYPE_NOT_SET,
    HNSWP_TYPE_RESET,
    HNSWP_TYPE_PING,
    HNSWP_TYPE_STATUS,
    HNSWP_TYPE_ACT_ON
}HNSWP_TYPE_T;

typedef struct HNSWPacketData
{
    uint32_t type;
    uint32_t actionIndex;
    uint32_t param[5];
    uint32_t payloadLength;
    uint8_t  payload[1024];
}HNSWP_PDATA_T;

class HNodeSWDPacket
{
    private:
        HNSWP_PDATA_T packetData;

    public:
        HNodeSWDPacket();
       ~HNodeSWDPacket();

        void setType( HNSWP_TYPE_T value );
        HNSWP_TYPE_T getType();

        void setActionIndex( uint32_t value );
        uint32_t getActionIndex();

        void setParam( int index, uint32_t value );
        uint32_t getParam( int index );

        // Payload Buffer Handling
        void setPldOffset( uint32_t offset );
        void writePldStr( std::string value );
        void writePldU8( uint8_t value );
        void writePldU16( uint16_t value );
        void writePldU32( uint32_t value );

        std::string getPldStr();
        uint8_t getPldU8();
        uint16_t getPldU16();
        uint32_t getPldU32();

        void setPayloadLength( uint32_t length );
        uint32_t getPayloadLength();
        uint8_t* getPayloadPtr();

        uint32_t getPacketLength();
        uint8_t* getPacketPtr();
        uint32_t getMaxPacketLength();
};

#endif // __HNODE_SW_PACKET_H__
