#include <stdio.h>
#include <stdarg.h>

#include "HNDaemonLog.h"

HNDaemonLog::HNDaemonLog()
{
    // Default log limit
    curLimit = HNDL_LOG_LEVEL_INFO;
}

HNDaemonLog::~HNDaemonLog()
{

}

void 
HNDaemonLog::processMsg( HNDL_LOG_LEVEL_T level, const char *format, va_list args )
{
    if( level > curLimit )
        return;

    vprintf( format, args );
    printf( "\n" );
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

