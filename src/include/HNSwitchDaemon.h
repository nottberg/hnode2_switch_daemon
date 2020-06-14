#ifndef __HN_SWD_PACKET_H__
#define __HN_SWD_PACKET_H__

#include <stdint.h>

#include <string>

#define HN_SWDAEMON_DEVICE_NAME  "switch-daemon"
#define HN_SWDAEMON_DEF_INSTANCE "default"

typedef enum HNodeSWDPacketTypeEnum
{
    HNSWD_PTYPE_NOTSET,
    HNSWD_PTYPE_DAEMON_STATUS,  // Daemon response with status info.
    HNSWD_PTYPE_DAEMON_EVENT,   // Asynch event originating from daemon.
    HNSWD_PTYPE_STATUS_REQ,     // Request Status of daemon
    HNSWD_PTYPE_RESET_REQ,       
    HNSWD_PTYPE_RESET_RSP,
    HNSWD_PTYPE_HEALTH_REQ,     // Request detailed component health
    HNSWD_PTYPE_HEALTH_RSP,     
    HNSWD_PTYPE_USEQ_ADD_REQ,    // Add a one time sequence of switch actions.
    HNSWD_PTYPE_USEQ_ADD_RSP,
    HNSWD_PTYPE_SEQ_CANCEL_REQ, // Cancel all future sequence actions.
    HNSWD_PTYPE_SEQ_CANCEL_RSP,
    HNSWD_PTYPE_SCH_STATE_REQ,  // Change scheduling state. Enable/Disable/Inhibit
    HNSWD_PTYPE_SCH_STATE_RSP,
    HNSWD_PTYPE_SWINFO_REQ,     // Request info about managed switches.
    HNSWD_PTYPE_SWINFO_RSP,
    HNSWD_PTYPE_SCHEDULE_UPDATE_REQ,   // Set a new schedule.
    HNSWD_PTYPE_SCHEDULE_UPDATE_RSP
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

typedef enum HNodeHealthStatusEnum
{
    HN_HEALTH_UNKNOWN,  // "UNKNOWN"
    HN_HEALTH_OK,       // "OK"
    HN_HEALTH_DEGRADED, // "DEGRADED"
    HN_HEALTH_FAILED    // "FAILED"
}HN_HEALTH_T;

// These error codes should be unique to a given
// daemon and error string, never remove/renumber
// only add new ones.
typedef enum HNodeSwitchDeamonErrorCodeEnum
{
    HNSWD_ERRCODE_NO_ERROR           = 0,
    HNSWD_ERRCODE_INTERNAL_FAILURE   = 1,
    HNSWD_ECODE_SM_FAILED_PATH_GEN   = 2,
    HNSWD_ECODE_SM_CONFIG_MISSING    = 3,
    HNSWD_ECODE_SM_CONFIG_OPEN       = 4,
    HNSWD_ECODE_SM_CONFIG_PARSER     = 5,
    HNSWD_ECODE_SWM_FAILED_PATH_GEN  = 6,
    HNSWD_ECODE_SWM_CONFIG_MISSING   = 7,
    HNSWD_ECODE_SWM_CONFIG_OPEN      = 8,
    HNSWD_ECODE_SWM_CONFIG_PARSER    = 9,
    HNSWD_ECODE_MCP280XX_I2CBUS_OPEN = 10,
    HNSWD_ECODE_MCP280XX_SET_TARGET  = 11,
    HNSWD_ECODE_MCP280XX_ZERO_STATE  = 12,
    HNSWD_ECODE_MCP280XX_SET_INBOUND = 13,
    HNSWD_ECODE_MCP280XX_SET_PULLUP  = 14,
    HNSWD_ECODE_MCP280XX_READ_STATE  = 15,
    HNSWD_ECODE_MCP280XX_SET_IODIR   = 16,
    HNSWD_ECODE_MCP280XX_WRITE_STATE = 17,
    HNSWD_ECODE_SUBCOMPONENT         = 18
}HNSWD_ERRCODE_T;

#endif // __HN_SWD_PACKET_H__
