#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <syslog.h>

#include <iostream>
#include <sstream>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Checksum.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include "HNSWDPacketDaemon.h"
#include "HNSwitchDaemon.h"

using namespace Poco::Util;

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

#define MAXEVENTS 8

void 
HNSwitchDaemon::defineOptions( OptionSet& options )
{
    ServerApplication::defineOptions( options );

    options.addOption(
              Option("help", "h", "display help").required(false).repeatable(false));

    options.addOption(
              Option("debug","d", "Enable debug logging").required(false).repeatable(false));
}

void 
HNSwitchDaemon::handleOption( const std::string& name, const std::string& value )
{
    ServerApplication::handleOption( name, value );
    if(name=="help") _helpRequested = true;
    if(name=="debug") _debugLogging = true;
}

void 
HNSwitchDaemon::displayHelp()
{
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("[options]");
    helpFormatter.setHeader("HNode2 Switch Daemon.");
    helpFormatter.format(std::cout);
}

uint32_t
HNSwitchDaemon::logSwitchChanges( struct tm *time, std::vector< std::string > &onList, uint32_t lastCRC )
{
    char buf[50];
    Poco::Checksum csum;

    // Get an ascii representation of the time
    // and clip off the newline
    asctime_r( time, buf );
    std::string tstr( buf, strlen(buf)-1 );

    // Create a string of all the switch ids
    std::string swIDStr;
    for( std::vector< std::string >::iterator it = onList.begin(); it != onList.end(); it++ )
        swIDStr += " " + *it;

    // Always log at the debug level.

    // Selectively log at the info level, always log at the debug level
    csum.update( swIDStr );
    if( lastCRC != csum.checksum() )
    {
        log.info( "Time: %s - switch on: %s", tstr.c_str(), swIDStr.c_str() );
    }
    else
    {
        log.debug( "Time: %s - switch on: %s", tstr.c_str(), swIDStr.c_str() );
    }

    return csum.checksum();   
}

int 
HNSwitchDaemon::main( const std::vector<std::string>& args )
{
    uint32_t lastCRC = 0xFFFF;
        uint prevSec = 0;

    // Move me to before option processing.
    instanceName = HN_SWDAEMON_DEF_INSTANCE;

    // Enable debug logging if requested
    if( _debugLogging == true )
    {
       log.setLevelLimit( HNDL_LOG_LEVEL_ALL );
    }

    // Initialize the overall health
    overallHealth.setComponent( "Overall" );
    overallHealth.setOK();

    // Setup logging for sub objects
    schMat.setDstLog( &log );
    switchMgr.setDstLog( &log );

    if( _helpRequested )
    {
        displayHelp();
        return Application::EXIT_OK;
    }

    log.info( "Starting hnode2 switch daemon init" );

    // Initialize the schedule matrix
    schMat.loadSchedule( HN_SWDAEMON_DEVICE_NAME, instanceName );

    // Initialize/Startup the switch manager
    switchMgr.setNotificationSink( this );
    switchMgr.loadConfiguration( HN_SWDAEMON_DEVICE_NAME, instanceName );

    // Initialize the devices
    switchMgr.initDevices();

    // Initialize for event loop
    epollFD = epoll_create1( 0 );
    if( epollFD == -1 )
    {
        log.error( "ERROR: Failure to create epoll event loop: %s", strerror(errno) );
        return Application::EXIT_SOFTWARE;
    }

    // Buffer where events are returned 
    events = (struct epoll_event *) calloc( MAXEVENTS, sizeof event );

    // Start reading time now.
    //gettimeofday( &lastReadingTS, NULL );

    // Open Unix named socket for requests
    openListenerSocket( HN_SWDAEMON_DEVICE_NAME, instanceName );

    log.info( "Entering hnode2 switch daemon event loop" );

    // The event loop 
    quit = false;
    while( quit == false )
    {
        int n;
        int i;
        struct tm newtime;
        time_t ltime;

        // Check for events
        n = epoll_wait( epollFD, events, MAXEVENTS, 2000 );

        // EPoll error
        if( n < 0 )
        {
            // If we've been interrupted by an incoming signal, continue, wait for socket indication
            if( errno == EINTR )
                continue;

            // Handle error
            log.error( "ERROR: Failure report by epoll event loop: %s", strerror( errno ) );
            return Application::EXIT_SOFTWARE;
        }

        // Check these critical tasks everytime
        // the event loop wakes up.
 
        // Get the local time so schedule matrix 
        // can be queried
        ltime = time( &ltime );
        localtime_r( &ltime, &newtime );

        // Query the schedule matrix for the list of
        // switches that should be active currently.
        std::vector< std::string > swidOnList;
        schMat.getSwitchOnList( &newtime, swidOnList );

        // Do some logging
        lastCRC = logSwitchChanges( &newtime, swidOnList, lastCRC );

        // Run the list of ON switches through the switch manager 
        // to make any necessary control device changes.
        switchMgr.processOnState( swidOnList );
 
        // Send async status packets roughly every 10 seconds.
        int diff = (newtime.tm_sec - prevSec);
        if( (diff < 0) || (diff >= 10) )
        {
            prevSec = newtime.tm_sec;
            sendStatus = true;
        }

        // Check if a status update should
        // be sent
        if( sendStatus )
        {
            sendStatusPacket( &newtime );
            sendStatus = false;
        }

        // If it was a timeout then continue to next loop
        // skip socket related checks.
        if( n == 0 )
            continue;

        // Socket event
        for( i = 0; i < n; i++ )
	    {
            if( acceptFD == events[i].data.fd )
	        {
                // New client connections
	            if( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)) )
	            {
                    /* An error has occured on this fd, or the socket is not ready for reading (why were we notified then?) */
                    syslog( LOG_ERR, "accept socket closed - restarting\n" );
                    close (events[i].data.fd);
	                continue;
	            }

                processNewClientConnections();
                continue;
            }
            else
            {
                // New client connections
	            if( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)) )
	            {
                    // An error has occured on this fd, or the socket is not ready for reading (why were we notified then?)
                    closeClientConnection( events[i].data.fd );

	                continue;
	            }

                // Handle a request from a client.
                processClientRequest( events[i].data.fd );
            }
        }
    }

    // Close the devices
    switchMgr.closeDevices();

    log.info( "Hnode2 switch daemon shutdown" );

    return Application::EXIT_OK;
}



HNSD_RESULT_T
HNSwitchDaemon::addSocketToEPoll( int sfd )
{
    int flags, s;

    flags = fcntl( sfd, F_GETFL, 0 );
    if( flags == -1 )
    {
        syslog( LOG_ERR, "Failed to get socket flags: %s", strerror(errno) );
        return HNSD_RESULT_FAILURE;
    }

    flags |= O_NONBLOCK;
    s = fcntl( sfd, F_SETFL, flags );
    if( s == -1 )
    {
        syslog( LOG_ERR, "Failed to set socket flags: %s", strerror(errno) );
        return HNSD_RESULT_FAILURE; 
    }

    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl( epollFD, EPOLL_CTL_ADD, sfd, &event );
    if( s == -1 )
    {
        return HNSD_RESULT_FAILURE;
    }

    return HNSD_RESULT_SUCCESS;
}

HNSD_RESULT_T
HNSwitchDaemon::removeSocketFromEPoll( int sfd )
{
    int s;

    s = epoll_ctl( epollFD, EPOLL_CTL_DEL, sfd, NULL );
    if( s == -1 )
    {
        return HNSD_RESULT_FAILURE;
    }

    return HNSD_RESULT_SUCCESS;
}

/*
HNSD_RESULT_T
HNSwitchDaemon::init()
{
    // Default to health ok
    healthOK = true;

    epollFD = epoll_create1( 0 );
    if( epollFD == -1 )
    {
        syslog( LOG_ERR, "epoll_create: %s", strerror(errno) );
        return HNSD_RESULT_FAILURE;
    }

    // Buffer where events are returned 
    events = (struct epoll_event *) calloc( MAXEVENTS, sizeof event );

    // Initialize the switch manager
    switchMgr.setNotificationSink( this );
    switchMgr.loadConfiguration();

    // Start reading time now.
    gettimeofday( &lastReadingTS, NULL );

    return HNSD_RESULT_SUCCESS;
}
*/

HNSD_RESULT_T
HNSwitchDaemon::addSignalSocket( int sfd )
{
    signalFD = sfd;
    
    return addSocketToEPoll( signalFD );
}

HNSD_RESULT_T
HNSwitchDaemon::openListenerSocket( std::string deviceName, std::string instanceName )
{
    struct sockaddr_un addr;
    char str[512];

    // Clear address structure - UNIX domain addressing
    // addr.sun_path[0] cleared to 0 by memset() 
    memset( &addr, 0, sizeof(struct sockaddr_un) );  
    addr.sun_family = AF_UNIX;                     

    // Abstract socket with name @<deviceName>-<instanceName>
    sprintf( str, "hnode2-%s-%s", deviceName.c_str(), instanceName.c_str() );
    strncpy( &addr.sun_path[1], str, strlen(str) );

    acceptFD = socket( AF_UNIX, SOCK_SEQPACKET, 0 );
    if( acceptFD == -1 )
    {
        syslog( LOG_ERR, "Opening daemon listening socket failed (%s).", strerror(errno) );
        return HNSD_RESULT_FAILURE;
    }

    if( bind( acceptFD, (struct sockaddr *) &addr, sizeof( sa_family_t ) + strlen( str ) + 1 ) == -1 )
    {
        syslog( LOG_ERR, "Failed to bind socket to @%s (%s).", str, strerror(errno) );
        return HNSD_RESULT_FAILURE;
    }

    if( listen( acceptFD, 4 ) == -1 )
    {
        syslog( LOG_ERR, "Failed to listen on socket for @%s (%s).", str, strerror(errno) );
        return HNSD_RESULT_FAILURE;
    }

    return addSocketToEPoll( acceptFD );
}


HNSD_RESULT_T
HNSwitchDaemon::processNewClientConnections( )
{
    uint8_t buf[16];

    // There are pending connections on the listening socket.
    while( 1 )
    {
        struct sockaddr in_addr;
        socklen_t in_len;
        int infd;

        in_len = sizeof in_addr;
        infd = accept( acceptFD, &in_addr, &in_len );
        if( infd == -1 )
        {
            if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
            {
                // All requests processed
                break;
            }
            else
            {
                // Error while accepting
                syslog( LOG_ERR, "Failed to accept for @acrt5n1d_readings (%s).", strerror(errno) );
                return HNSD_RESULT_FAILURE;
            }
        }

        syslog( LOG_ERR, "Adding client - sfd: %d", infd );

        ClientRecord client;

        client.setFD( infd );

        clientMap.insert( std::pair< int, ClientRecord >( infd, client ) );

        addSocketToEPoll( infd );
    }

    return HNSD_RESULT_SUCCESS;
}

                    
HNSD_RESULT_T
HNSwitchDaemon::closeClientConnection( int clientFD )
{
    clientMap.erase( clientFD );

    removeSocketFromEPoll( clientFD );

    close( clientFD );

    syslog( LOG_ERR, "Closed client - sfd: %d", clientFD );

    return HNSD_RESULT_SUCCESS;
}

HNSD_RESULT_T
HNSwitchDaemon::processClientRequest( int cfd )
{
    // One of the clients has sent us a message.
    HNSWDPacketDaemon packet;
    HNSWDP_RESULT_T   result;

    uint32_t          recvd = 0;

    // Note that a client request is being recieved.
    log.info( "Receiving client request from fd: %d", cfd );

    // Read the header portion of the packet
    result = packet.rcvHeader( cfd );
    if( result == HNSWDP_RESULT_NOPKT )
    {
        return HNSD_RESULT_SUCCESS;
    }
    else if( result != HNSWDP_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failed while receiving packet header." );
        return HNSD_RESULT_FAILURE;
    } 

    // Read any payload portion of the packet
    result = packet.rcvPayload( cfd );
    if( result != HNSWDP_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failed while receiving packet payload." );
        return HNSD_RESULT_FAILURE;
    } 

    // Take any necessary action associated with the packet
    switch( packet.getType() )
    {
        // A request for status from the daemon
        case HNSWD_PTYPE_STATUS_REQ:
        {
            log.info( "Status request from client: %d", cfd );
            sendStatus = true;
        }
        break;

        // Request the daemon to reset itself.
        case HNSWD_PTYPE_RESET_REQ:
        {
            log.info( "Reset request from client: %d", cfd );

            // Start out with good health.
            // healthOK = true;

            // Reinitialize the underlying RTLSDR code.
            // demod.init();

            // Start reading time now.
            // gettimeofday( &lastReadingTS, NULL );

            sendStatus = true;
        }
        break;

        // Request a manual sequence of switch activity.
        case HNSWD_PTYPE_OT_SW_SEQ_REQ:
        break;

        // Unknown packet
        default:
        {
            log.warn( "Warning: RX of unsupported packet, discarding - type: %d", packet.getType() );
        }
        break;
    }

    return HNSD_RESULT_SUCCESS;
}

#if 0
HNSD_RESULT_T
HNSwitchDaemon::run()
{
    // Startup the switch manager
    switchMgr.start();   

    // The event loop
    while( quit == false )
    {
        int n, i;

        // Check for events
        n = epoll_wait( epollFD, events, MAXEVENTS, 0 );

        // EPoll error
        if( n < 0 )
        {
            // If we've been interrupted by an incoming signal, continue, wait for socket indication
            if( errno == EINTR )
                continue;

            // Handle error
        }

        // Timeout
        if( n == 0 )
        {
            struct timeval curTS;

            // Get current time
            gettimeofday( &curTS, NULL );

            // If a reading hasn't been seen for 2 minutes then
            // degrade the health state.
            if( (curTS.tv_sec - lastReadingTS.tv_sec) > 120 )
            {
                signalError( "No measurments within last 2 minutes." );
            }

            // Check if a status update should
            // be sent
            if( sendStatus )
            {
                sendStatusPacket( &curTS );
                sendStatus = false;
            }

            // Run the RTLSDR loop
            if( switchMgr.evaluateActions() )
            {
	            daemon_log( LOG_ERR, "Fatal error while evaluating switch actions, exiting...\n" );
	            break;
            }
            continue;
        }

        // Socket event
        for( i = 0; i < n; i++ )
	    {
            // Dispatch based on file desriptor
            if( signalFD == events[i].data.fd )
            {
                // There was signal activity from libdaemon
	            if( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)) )
	            {
                    // An error has occured on this fd, or the socket is not ready for reading (why were we notified then?) 
	                daemon_log( LOG_ERR, "epoll error on daemon signal socket\n" );
	                close (events[i].data.fd);
	                continue;
	            }

                // Read the signal from the socket
                int sig = daemon_signal_next();
                if( sig <= 0 ) 
                {
                    daemon_log(LOG_ERR, "daemon_signal_next() failed: %s", strerror(errno));
                    break;
                }

                // Act on the signal
                switch (sig)
                {

                    case SIGINT:
                    case SIGQUIT:
                    case SIGTERM:
                    {
                        daemon_log( LOG_WARNING, "Got SIGINT, SIGQUIT or SIGTERM." );
                        quit = true;
                    }
                    break;

                    case SIGHUP:
                    default:
                    {
                        daemon_log( LOG_INFO, "HUP - Ignoring signal" );
                        break;
                    }
                }
            }
            else if( acceptFD == events[i].data.fd )
	        {
                // New client connections
	            if( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)) )
	            {
                    /* An error has occured on this fd, or the socket is not ready for reading (why were we notified then?) */
                    daemon_log( LOG_ERR, "accept socket closed - restarting\n" );
                    close (events[i].data.fd);
	                continue;
	            }

                processNewClientConnections();
                continue;
            }
            else
            {
                // New client connections
	            if( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)) )
	            {
                    // An error has occured on this fd, or the socket is not ready for reading (why were we notified then?)
                    closeClientConnection( events[i].data.fd );

	                continue;
	            }

                // Handle a request from a client.
                processClientRequest( events[i].data.fd );
            }
        }
    }

    // Shutdown the switch manager
    switchMgr.stop();
}
#endif

void 
HNSwitchDaemon::sendStatusPacket( struct tm *curTS )
{
    std::stringstream statusMsg;
    char tmpBuf[128];

    // Create a json root object
    pjs::Object jsRoot;

    // Add the timezone setting
    jsRoot.set( "timezone", schMat.getTimezone() );

    // Add the current date
    sprintf( tmpBuf, "%2.2d/%2.2d/%4.4d", (curTS->tm_mon + 1), curTS->tm_mday, (curTS->tm_year + 1900) );
    jsRoot.set( "date", (const char *)tmpBuf );

    // Add the current time
    sprintf( tmpBuf, "%2.2d:%2.2d:%2.2d", curTS->tm_hour, curTS->tm_min, curTS->tm_sec );
    jsRoot.set( "time", (const char *)tmpBuf );

    // Accumulate system health state
    pjs::Array jsDCHealth;
    pjs::Object jsCHealth;

    overallHealth.setOK();

    uint ccnt = schMat.getHealthComponentCount();
    for( uint cindx = 0; cindx < ccnt; cindx++ )
    {
        HNDaemonHealth *hPtr = schMat.getHealthComponent( cindx );

        if( hPtr->getStatus() != HN_HEALTH_OK )
            overallHealth.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SUBCOMPONENT );

        jsCHealth.set( "component", hPtr->getComponent() );
        jsCHealth.set( "status", hPtr->getStatusStr() );
        sprintf( tmpBuf, "%d", hPtr->getErrorCode() );
        jsCHealth.set( "errCode", (const char *)tmpBuf );
        jsCHealth.set( "msg", hPtr->getMsg() );

        jsDCHealth.add( jsCHealth );
    }

    ccnt = switchMgr.getHealthComponentCount();
    for( uint cindx = 0; cindx < ccnt; cindx++ )
    {
        HNDaemonHealth *hPtr = switchMgr.getHealthComponent( cindx );

        if( hPtr->getStatus() != HN_HEALTH_OK )
            overallHealth.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SUBCOMPONENT );

        jsCHealth.set( "component", hPtr->getComponent() );
        jsCHealth.set( "status", hPtr->getStatusStr() );
        sprintf( tmpBuf, "%d", hPtr->getErrorCode() );
        jsCHealth.set( "errCode", (const char *)tmpBuf );
        jsCHealth.set( "msg", hPtr->getMsg() );

        jsDCHealth.add( jsCHealth );
    }

    jsRoot.set( "componentHealth", jsDCHealth );

    // Add overall health to output.
    pjs::Object jsDHealth;

    jsDHealth.set( "component", overallHealth.getComponent() );
    jsDHealth.set( "status", overallHealth.getStatusStr() );
    jsDHealth.set( "msg", overallHealth.getMsg() );

    jsRoot.set( "overallHealth", jsDHealth );

    // Render into a json string for the status packet.
    try
    {
        // Write out the generated json
        pjs::Stringifier::stringify( jsRoot, statusMsg );
    }
    catch( ... )
    {
        return;
    }

    // Create the packet.
    HNSWDPacketDaemon packet( HNSWD_PTYPE_DAEMON_STATUS, HNSWD_RCODE_SUCCESS, statusMsg.str() );

    // Broadcast packets to listeners
    for( std::map< int, ClientRecord >::iterator it = clientMap.begin(); it != clientMap.end(); it++ )
    {
        packet.sendAll( it->first );         
    }
}

#if 0
void 
HNSwitchDaemon::sendSwitchInfo()
{
    packet.setActionIndex( healthOK );

    packet.setParam( 0, curTS->tv_sec );
    packet.setParam( 1, curTS->tv_usec );

    swCount = switchMgr.getSwitchCount();

    packet.setParam( 2, 0); // switchMgr.getActionCount() );
    packet.setParam( 3, curErrMsg.size() );

    packet.setParam( 4, swCount );

    for( index = 0; index < swCount; index++ )
    {
        HNSWSwitch *swObj = switchMgr.getSwitchByIndex( index );

        //syslog( LOG_INFO, "Switch %d - address: %s\n", index, swObj->getAddress().c_str() );
        //syslog( LOG_INFO, "Switch %d - id: %s\n", index, swObj->getID().c_str() );
        //syslog( LOG_INFO, "Switch %d - name: %s\n", index, swObj->getName().c_str() );
        //syslog( LOG_INFO, "Switch %d - desc: %s\n", index, swObj->getDescription().c_str() );
        //syslog( LOG_INFO, "Switch %d - mapped: %d\n", index, swObj->isMapped() );
        //syslog( LOG_INFO, "Switch %d - controlled: %d\n", index, swObj->isControlled() );
        //syslog( LOG_INFO, "Switch %d - Cap on/off: %d\n", index, swObj->hasCapOnOff() );
        //syslog( LOG_INFO, "Switch %d - State: %d\n", index, swObj->isStateOn() );

        //std::string getID();
        //std::string getName();
        //std::string getDescription();
        //bool isMapped();
        //bool isControlled();
        //bool hasCapOnOff();
        //bool isStateOn();
    }

    packet.setPayloadLength( curErrMsg.size() );
    memcpy( packet.getPayloadPtr(), curErrMsg.c_str(), curErrMsg.size() );

    for( std::map< int, ClientRecord >::iterator it = clientMap.begin(); it != clientMap.end(); it++ )
    {
        length = send( it->first, packet.getPacketPtr(), packet.getPacketLength(), MSG_NOSIGNAL );         
    }
}
#endif

#if 0
void 
DaemonProcess::notifyNewMeasurement( uint32_t sensorIndex, HNodeSensorMeasurement &reading )
{
    HNodeSWDPacket packet;
    uint32_t length;

    gettimeofday( &lastReadingTS, NULL );

    packet.setType( HNSEPP_TYPE_HNS_MEASUREMENT );

    reading.buildPacketData( packet.getPayloadPtr(), length );

    packet.setPayloadLength( length );
    packet.setSensorIndex( sensorIndex );

    for( std::map< int, ClientRecord >::iterator it = clientMap.begin(); it != clientMap.end(); it++ )
    {
        length = send( it->first, packet.getPacketPtr(), packet.getPacketLength(), MSG_NOSIGNAL );         
    }

    // Since we recieved a measurement
    // things must be working.
    signalRunning();
}
#endif

void 
HNSwitchDaemon::signalError( std::string errMsg )
{
#if 0
    if( (healthOK == true) || (curErrMsg != errMsg) )
    {
        syslog( LOG_ERR, "Signal Error: %s\n", errMsg.c_str() );

        healthOK   = false;
        curErrMsg  = errMsg;
        sendStatus = true;
    }
#endif
}

void 
HNSwitchDaemon::signalRunning()
{
#if 0
    if( healthOK == false )
    {
        syslog( LOG_ERR, "Signal Running\n" );

        healthOK  = true;
        curErrMsg.clear();
        sendStatus = true;
    }
#endif
}


#if 0
DaemonProcess::DaemonProcess()
{
    quit = false;

    sendStatus = false;

    healthOK   = false;

    curErrMsg.empty();

    lastReadingTS.tv_sec  = 0;
    lastReadingTS.tv_usec = 0;
}

DaemonProcess::~DaemonProcess()
{

}
#endif

ClientRecord::ClientRecord()
{
    clientFD = 0;
}

ClientRecord::~ClientRecord()
{

}

void 
ClientRecord::setFD( int value )
{
    clientFD = value;
}

int 
ClientRecord::getFD()
{
    return clientFD;
}


