#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "HNDaemonLog.h"

HNDaemonLog::HNDaemonLog()
{
    // Runing as a daemon.
    isDaemon = false;

    // Default log limit
    curLimit = HNDL_LOG_LEVEL_INFO;
}

HNDaemonLog::~HNDaemonLog()
{

}

void 
HNDaemonLog::setLevelLimit( HNDL_LOG_LEVEL_T value )
{
    curLimit = value;
}

void 
HNDaemonLog::setDaemon( bool value )
{
    isDaemon = value;
}

void 
HNDaemonLog::processMsg( HNDL_LOG_LEVEL_T level, const char *format, va_list args )
{
    if( level > curLimit )
        return;

    if( isDaemon == false )
    {
        vprintf( format, args );
        printf( "\n" );
        return;
    }
    
    switch( level )
    {
        case HNDL_LOG_LEVEL_ERROR:
            vsyslog( LOG_ERR, format, args );
        break;

        case HNDL_LOG_LEVEL_WARN:
            vsyslog( LOG_WARNING, format, args );
        break;

        case HNDL_LOG_LEVEL_INFO:
            vsyslog( LOG_INFO, format, args );
        break;

        case HNDL_LOG_LEVEL_DEBUG:
            vsyslog( LOG_DEBUG, format, args );
        break;
    }
}

void
HNDaemonLog::debug( const char *format, ... )
{
    va_list args;
    va_start( args, format );

    processMsg( HNDL_LOG_LEVEL_DEBUG, format, args );

    va_end( args );
}

void
HNDaemonLog::info( const char *format, ... )
{
    va_list args;
    va_start( args, format );

    processMsg( HNDL_LOG_LEVEL_INFO, format, args );

    va_end( args );
}

void
HNDaemonLog::warn( const char *format, ... )
{
    va_list args;
    va_start( args, format );

    processMsg( HNDL_LOG_LEVEL_WARN, format, args );

    va_end( args );
}

void
HNDaemonLog::error( const char *format, ... )
{
    va_list args;
    va_start( args, format );

    processMsg( HNDL_LOG_LEVEL_ERROR, format, args );

    va_end( args );
}


HNDaemonLogSrc::HNDaemonLogSrc()
{
    logPtr = NULL;
}

HNDaemonLogSrc::~HNDaemonLogSrc()
{

}

void 
HNDaemonLogSrc::clearDstLog()
{
    logPtr = NULL;
}

void 
HNDaemonLogSrc::setDstLog( HNDaemonLog *dstLog )
{
    logPtr = dstLog;
}

HNDaemonLog*
HNDaemonLogSrc::getDstLog()
{
    return logPtr;
}

void 
HNDaemonLogSrc::debug( const char *format, ... )
{
    if( logPtr == NULL )
        return;

    va_list args;
    va_start( args, format );

    logPtr->processMsg( HNDL_LOG_LEVEL_DEBUG, format, args );

    va_end( args );
}

void 
HNDaemonLogSrc::info( const char *format, ... )
{
    if( logPtr == NULL )
        return;

    va_list args;
    va_start( args, format );

    logPtr->processMsg( HNDL_LOG_LEVEL_INFO, format, args );

    va_end( args );
}

void 
HNDaemonLogSrc::warn( const char *format, ... )
{
    if( logPtr == NULL )
        return;

    va_list args;
    va_start( args, format );

    logPtr->processMsg( HNDL_LOG_LEVEL_WARN, format, args );

    va_end( args );
}

void 
HNDaemonLogSrc::error( const char *format, ... )
{
    if( logPtr == NULL )
        return;

    va_list args;
    va_start( args, format );

    logPtr->processMsg( HNDL_LOG_LEVEL_ERROR, format, args );

    va_end( args );
}
        
