#ifndef __HN_SWD_PACKET_H__
#define __HN_SWD_PACKET_H__

#include <stdint.h>

#include <string>

typedef enum HNodeSWDPacketTypeEnum
{
    HNSWD_PTYPE_NOTSET,
    HNSWD_PTYPE_DAEMON_STATUS,    // Daemon response with status info.
    HNSWD_PTYPE_DAEMON_EVENT,  // Asynch event originating from daemon.
    HNSWD_PTYPE_STATUS_REQ,    // Request Status of daemon
    HNSWD_PTYPE_RESET_REQ,       
    HNSWD_PTYPE_RESET_RSP,
    HNSWD_PTYPE_OT_SW_SEQ_REQ, // One time sequence of switches to on.
    HNSWD_PTYPE_OT_SW_SEQ_RSP
}HNSWD_PTYPE_T;

typedef enum HNodeSWDPacketResultCodeEnum
{
    HNSWD_RCODE_NOTSET,
    HNSWD_RCODE_SUCCESS,
    HNSWD_RCODE_FAILURE       
}HNSWD_RCODE_T;

typedef struct HNSWDPacketHeader
{
    uint32_t type;
    uint32_t resultCode;
    uint32_t msgLen;
}HNSWD_PKTHDR_T;

#endif // __HN_SWD_PACKET_H__
