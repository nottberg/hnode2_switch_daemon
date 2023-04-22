#ifndef __HN_SCHEDULE_MATRIX_H__
#define __HN_SCHEDULE_MATRIX_H__

#include <unistd.h>
#include <time.h>

#include <string>
#include <list>

#include "HNDaemonLog.h"
#include "HNDaemonHealth.h"

typedef enum HNSScheduleMatrixResult
{
    HNSM_RESULT_SUCCESS,
    HNSM_RESULT_FAILURE
}HNSM_RESULT_T;

typedef enum HNScheduleMatrixDayIndexEnum
{
    HNS_DINDX_SUNDAY    = 0,
    HNS_DINDX_MONDAY    = 1,
    HNS_DINDX_TUESDAY   = 2,
    HNS_DINDX_WEDNESDAY = 3,
    HNS_DINDX_THURSDAY  = 4,
    HNS_DINDX_FRIDAY    = 5,
    HNS_DINDX_SATURDAY  = 6,
    HNS_DAY_CNT         = 7
}HNS_DAY_INDX_T;

class HNS24HTime
{
    private:
        uint secOfDay;

    public:
        HNS24HTime();
       ~HNS24HTime();

        HNSM_RESULT_T setFromHMS( uint hour, uint minute, uint second );
        HNSM_RESULT_T setFromSeconds( uint seconds );
        HNSM_RESULT_T parseTime( std::string value );

        void addSeconds( uint seconds );
        void addDuration( HNS24HTime &duration );

        uint getSeconds() const;
        void getHMS( uint &hour, uint &minute, uint &second );
        std::string getHMSStr();
};

typedef enum HNScheduleMatrixDayActionsEnum
{
    HNS_ACT_NOT_SET,
    HNS_ACT_SWON
}HNS_ACT_T;

class HNSAction
{
    private:
        HNS_ACT_T   action;
        HNS24HTime  start;
        HNS24HTime  end;
        std::string swid;

    public:
        HNSAction();
        HNSAction( HNS_ACT_T action, HNS24HTime &startTime, HNS24HTime &endTime, std::string swID );
       ~HNSAction();

        HNSM_RESULT_T handlePair( std::string key, std::string value );

        uint getStartSeconds() const;
        uint getEndSeconds() const;

        std::string getSwID();

        bool startIsBefore( HNS24HTime &tgtTime );
        bool startIsAfter( HNS24HTime &tgtTime );
   
        bool endIsAfter( HNS24HTime &tgtTime );

        static bool sortCompare(const HNSAction& first, const HNSAction& second);

        void debugPrint( uint offset, HNDaemonLogSrc &log );
};

class HNSDay
{
    private:
        HNS_DAY_INDX_T dindx;
        std::string    dayName;
  
        std::list< HNSAction > actionArr;
 
    public:
        HNSDay();
       ~HNSDay();

        void clear();

        HNSM_RESULT_T addAction( HNSAction &action );

        void setID( HNS_DAY_INDX_T index, std::string name );

        void sortActions();

        void debugPrint( uint offset, HNDaemonLogSrc &log );

        HNSM_RESULT_T getSwitchOnList( HNS24HTime &tgtTime, std::vector< std::string > &swidList );
};

typedef enum HNScheduleMatrixState
{
  HNSM_SCHSTATE_ENABLED,   // Scheduling is enabled
  HNSM_SCHSTATE_DISABLED,  // Scheduling is disabled
  HNSM_SCHSTATE_INHIBIT    // Scheduling is inhibited for a duration.
}HNSM_SCHSTATE_T;

#define HNS_SCH_CFG_DEFAULT_PATH  "/var/cache/hnode2/"

class HNScheduleMatrix : public HNDHConsumerInterface
{
    private:
        HNDaemonLogSrc m_log;

        HNDaemonHealth m_health;

        std::string m_rootDirPath;

        std::string m_deviceName;
        std::string m_instanceName;

        std::string m_timezone;

        HNSDay  m_dayArr[ HNS_DAY_CNT ];

        HNSM_SCHSTATE_T m_state;
        HNS24HTime      m_inhibitUntil;
  
        uint        m_schUpdateIndex; 
        std::string m_schCRC32;

        HNSM_RESULT_T generateScheduleFilePath( std::string &fpath );
        HNSM_RESULT_T generateStateFilePath( std::string &fpath );

        HNSM_RESULT_T createDirectories();

        HNSM_RESULT_T updateStateFile( HNSM_SCHSTATE_T value );
        HNSM_RESULT_T readStateFile( HNSM_SCHSTATE_T &value );

    public:
        HNScheduleMatrix();
       ~HNScheduleMatrix();

        void setDstLog( HNDaemonLog *logPtr );

        void setRootDirectory( std::string path );
        std::string getRootDirectory();

        void setInstance( std::string devname, std::string instance );
        void setTimezone( std::string tzname );
        void setScheduleCRC32Str( std::string value );


        HNSM_SCHSTATE_T getState();
        std::string getStateStr();

        std::string getTimezone();
        uint getScheduleUpdateIndex();
        std::string getScheduleCRC32Str();

        void clear();

        HNSM_RESULT_T loadSchedule();
        HNSM_RESULT_T replaceSchedule( std::string newSchJSON );

        void initState();
        void setStateDisabled();
        void setStateEnabled();
        void setStateInhibited( struct tm *time, std::string duration );
        std::string getInhibitUntilStr();
        bool checkInhibitExpire( struct tm *time );

        HNSM_RESULT_T getSwitchOnList( struct tm *time, std::vector< std::string > &swidList );

        virtual uint getHealthComponentCount();
        virtual HNDaemonHealth* getHealthComponent( uint index );

        void debugPrint();
};


class HNSequenceQueue
{
    private:
        HNDaemonLogSrc m_log;

        // The ID from the requestor for the current sequence
        std::string m_requestID;

        std::list< HNSAction > actionList;

    public:
        HNSequenceQueue();
       ~HNSequenceQueue();

        void setDstLog( HNDaemonLog *logPtr );

        bool hasActions();

        void clearRequestID();
        
        HNSM_RESULT_T cancelSequences();

        HNSM_RESULT_T addUniformSequence( struct tm *time, std::string seqJSON, std::string &errMsg );

        std::string getRequestID();
        
        HNSM_RESULT_T getSwitchOnList( struct tm *time, std::vector< std::string > &swidList );

        void debugPrint();
};

#endif // __HN_SCHEDULE_MATRIX_H__

