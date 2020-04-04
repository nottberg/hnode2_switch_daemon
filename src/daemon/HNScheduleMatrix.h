#ifndef __HN_SCHEDULE_MATRIX_H__
#define __HN_SCHEDULE_MATRIX_H__

#include <unistd.h>
#include <time.h>

#include <string>
#include <list>

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
        HNSM_RESULT_T parseTime( std::string value );

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
       ~HNSAction();

        HNSM_RESULT_T handlePair( std::string key, std::string value );

        uint getStartSeconds() const;
        uint getEndSeconds() const;

        std::string getSwID();

        bool startIsBefore( HNS24HTime &tgtTime );
        bool startIsAfter( HNS24HTime &tgtTime );
   
        bool endIsAfter( HNS24HTime &tgtTime );

        static bool sortCompare(const HNSAction& first, const HNSAction& second);

        void debugPrint( uint offset );
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

        void debugPrint( uint offset );

        HNSM_RESULT_T getSwitchOnList( HNS24HTime &tgtTime, std::vector< std::string > &swidList );
};

#define HNS_SCH_CFG_DEFAULT_PATH  "/var/cache/hnode2/"

class HNScheduleMatrix
{
    private:
        std::string rootDirPath;

        std::string deviceName;
        std::string instanceName;

        std::string timezone;

        HNSDay  dayArr[ HNS_DAY_CNT ];
  
        HNSM_RESULT_T generateFilePath( std::string &fpath );

    public:
        HNScheduleMatrix();
       ~HNScheduleMatrix();

        void setRootDirectory( std::string path );
        std::string getRootDirectory();

        void setTimezone( std::string tzname );

        void clear();

        HNSM_RESULT_T loadSchedule( std::string devname, std::string instance );

        HNSM_RESULT_T getSwitchOnList( struct tm *time, std::vector< std::string > &swidList );

        void debugPrint();
};

#endif // __HN_SCHEDULE_MATRIX_H__

