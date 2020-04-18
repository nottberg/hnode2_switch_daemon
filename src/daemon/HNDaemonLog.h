#ifndef __HN_DAEMON_LOG_H__
#define __HN_DAEMON_LOG_H__

#include <cstdarg>

typedef enum HNDaemonLogLevelEnum
{
    HNDL_LOG_LEVEL_ERROR = 0,
    HNDL_LOG_LEVEL_WARN  = 1,
    HNDL_LOG_LEVEL_INFO  = 2,
    HNDL_LOG_LEVEL_DEBUG = 3,
    HNDL_LOG_LEVEL_ALL   = 4,
}HNDL_LOG_LEVEL_T;

class HNDaemonLog
{
    private:
        bool isDaemon;

        HNDL_LOG_LEVEL_T curLimit;

    protected:
        void processMsg( HNDL_LOG_LEVEL_T level, const char *format, va_list args );

    public:
        HNDaemonLog();
       ~HNDaemonLog();

        void setDaemon( bool value );

        void setLevelLimit( HNDL_LOG_LEVEL_T value );

        void debug( const char *format, ... );
        void info( const char *format, ... );
        void warn( const char *format, ... );
        void error( const char *format, ... );
        
    friend class HNDaemonLogSrc;
};

class HNDaemonLogSrc
{
    private:
        HNDaemonLog *logPtr;

    public:
        HNDaemonLogSrc();
       ~HNDaemonLogSrc();

        void clearDstLog();
        void setDstLog( HNDaemonLog *dstLog );
        HNDaemonLog *getDstLog();

        void debug( const char *format, ... );
        void info( const char *format, ... );
        void warn( const char *format, ... );
        void error( const char *format, ... );
        
};

#endif // __HN_DAEMON_LOG_H__
