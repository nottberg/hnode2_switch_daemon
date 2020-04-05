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

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"

#include "HNodeSWDPacket.h"
#include "HNSwitchDaemon.h"

using namespace Poco::Util;

#define MAXEVENTS 8

void 
HNSwitchDaemon::defineOptions( OptionSet& options )
{
    ServerApplication::defineOptions( options );

    options.addOption(
              Option("help", "h", "display help").required(false).repeatable(false));

    options.addOption(
            Option("port","p", "Listening port (default 9980)").required(false).repeatable(true).argument("<port>"));
}

void 
HNSwitchDaemon::handleOption( const std::string& name, const std::string& value )
{
    ServerApplication::handleOption(name,value);
    if(name=="help") helpRequested=true;
    //if(name=="port") listenPort=stoi(value);
}

void 
HNSwitchDaemon::displayHelp()
{
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("[options]");
    helpFormatter.setHeader("C++ server pages application server.");
    helpFormatter.format(std::cout);
}

int 
HNSwitchDaemon::main( const std::vector<std::string>& args )
{
    // Move me to before option processing.
    instanceName = HN_SWDAEMON_DEF_INSTANCE;

    if(helpRequested)
    {
        displayHelp();
        return Application::EXIT_OK;
    }

    // Initialize the schedule matrix
    schMat.loadSchedule( HN_SWDAEMON_DEVICE_NAME, instanceName );

    // Initialize/Startup the switch manager
    switchMgr.setNotificationSink( this );
    switchMgr.loadConfiguration( HN_SWDAEMON_DEVICE_NAME, instanceName );

    // Initialize the devices
    switchMgr.initDevices();

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

    // Start reading time now.
    gettimeofday( &lastReadingTS, NULL );

    // Open Unix named socket for requests
    openListenerSocket( HN_SWDAEMON_DEVICE_NAME, instanceName );

    // The event loop 
    quit = false;
    while( quit == false )
    {
        int n, i;

        // Check for events
        n = epoll_wait( epollFD, events, MAXEVENTS, 2000 );

        // EPoll error
        if( n < 0 )
        {
            // If we've been interrupted by an incoming signal, continue, wait for socket indication
            if( errno == EINTR )
                continue;

            // Handle error
            std::cout << "EPoll Error" << std::endl;
        }

        // Timeout
        if( n == 0 )
        {
            struct tm newtime;
            time_t ltime;
            char buf[50];
 
            ltime=time( &ltime );
            localtime_r( &ltime, &newtime );
            asctime_r( &newtime, buf );
            std::string tstr( buf, strlen(buf)-1 );

            std::vector< std::string > swidOnList;
            schMat.getSwitchOnList( &newtime, swidOnList );

            std::cout << "Time: " << tstr << "  swids: "; 
            for( std::vector< std::string >::iterator it = swidOnList.begin(); it != swidOnList.end(); it++ )
            {
                std::cout << *it << " ";
            }
            std::cout << std::endl;

            switchMgr.processOnState( swidOnList );
 
/* 
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
	            syslog( LOG_ERR, "Fatal error while evaluating switch actions, exiting...\n" );
	            break;
            }
            continue;
*/
        }

        // Socket event
        for( i = 0; i < n; i++ )
	    {
#if 0
            // Dispatch based on file desriptor
            if( signalFD == events[i].data.fd )
            {
                // There was signal activity from libdaemon
	            if( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)) )
	            {
                    // An error has occured on this fd, or the socket is not ready for reading (why were we notified then?) 
	                syslog( LOG_ERR, "epoll error on daemon signal socket\n" );
	                close (events[i].data.fd);
	                continue;
	            }

                // Read the signal from the socket
                int sig = daemon_signal_next();
                if( sig <= 0 ) 
                {
                    syslog(LOG_ERR, "daemon_signal_next() failed: %s", strerror(errno));
                    break;
                }

                // Act on the signal
                switch (sig)
                {

                    case SIGINT:
                    case SIGQUIT:
                    case SIGTERM:
                    {
                        syslog( LOG_WARNING, "Got SIGINT, SIGQUIT or SIGTERM." );
                        quit = true;
                    }
                    break;

                    case SIGHUP:
                    default:
                    {
                        syslog( LOG_INFO, "HUP - Ignoring signal" );
                        break;
                    }
                }
            }
            else 
#endif
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

    // Shutdown the switch manager
    //switchMgr.stop();

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
    sprintf( str, "%s-%s", deviceName.c_str(), instanceName.c_str() );
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
HNSwitchDaemon::processClientRequest( int efd )
{
    // One of the clients has sent us a message.
    HNodeSWDPacket  packet;
    uint32_t        recvd = 0;

    //std::cout << "start recvd..." << std::endl;

    recvd += recv( efd, packet.getPacketPtr(), packet.getMaxPacketLength(), 0 );

    switch( packet.getType() )
    {
        case HNSWP_TYPE_PING:
        {
            syslog( LOG_ERR, "Ping request from client: %d", efd );
            sendStatus = true;
        }
        break;

        case HNSWP_TYPE_RESET:
        {
            syslog( LOG_ERR, "Reset request from client: %d", efd );

            // Start out with good health.
            healthOK = true;

            // Reinitialize the underlying RTLSDR code.
            //demod.init();

            // Start reading time now.
            gettimeofday( &lastReadingTS, NULL );

            sendStatus = true;
        }
        break;

        default:
        {
            syslog( LOG_ERR, "RX of unknown packet - len: %d  type: %d", recvd, packet.getType() );
        }
        break;
    }

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
HNSwitchDaemon::sendStatusPacket( struct timeval *curTS )
{
    HNodeSWDPacket packet;
    uint32_t length;
    uint32_t swCount;
    uint32_t index;

    packet.setType( HNSWP_TYPE_STATUS );

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
    if( (healthOK == true) || (curErrMsg != errMsg) )
    {
        syslog( LOG_ERR, "Signal Error: %s\n", errMsg.c_str() );

        healthOK   = false;
        curErrMsg  = errMsg;
        sendStatus = true;
    }
}

void 
HNSwitchDaemon::signalRunning()
{
    if( healthOK == false )
    {
        syslog( LOG_ERR, "Signal Running\n" );

        healthOK  = true;
        curErrMsg.clear();
        sendStatus = true;
    }
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


