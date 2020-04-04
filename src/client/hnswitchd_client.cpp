#include <stdint.h>
#include <iostream>
#include <cstddef>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include <boost/program_options.hpp>

#include "HNodeSWDPacket.h"

namespace po = boost::program_options;

int main( int argc, char *argv[] )
{
    struct sockaddr_un addr;
    const char *str = "hnswitchd";

    // Declare the supported options.
    po::options_description desc( "acrt5n1d client utility" );
    desc.add_options()
        ("help", "produce help message")
        ("ping", "send a ping packet")
        ("reset", "send a reset packet")
    ;

    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );    

    if( vm.count( "help" ) )
    {
        std::cout << desc << "\n";
        return 1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));  // Clear address structure
    addr.sun_family = AF_UNIX;                     // UNIX domain address

    // addr.sun_path[0] has already been set to 0 by memset() 

    // Abstract socket with name @acrt5n1d_readings
    strncpy( &addr.sun_path[1], str, strlen(str) );

    int fd = socket( AF_UNIX, SOCK_SEQPACKET, 0 );

    // Establish the connection.
    if( connect( fd, (struct sockaddr *) &addr, sizeof( sa_family_t ) + strlen( str ) + 1 ) == 0 )
    {

        if( vm.count("reset") )
        {
            HNodeSWDPacket packet;
            uint32_t length;

            packet.setType( HNSWP_TYPE_RESET );

            std::cout << "Sending a RESET packet...";

            length = send( fd, packet.getPacketPtr(), packet.getPacketLength(), MSG_NOSIGNAL );

            std::cout << length << " bytes sent." << std::endl;
        }
        else if( "ping" )
        {
            HNodeSWDPacket packet;
            uint32_t length;

            packet.setType( HNSWP_TYPE_PING );

            std::cout << "Sending a PING packet...";

            length = send( fd, packet.getPacketPtr(), packet.getPacketLength(), MSG_NOSIGNAL );

            std::cout << length << " bytes sent." << std::endl;
        }

        // Listen for packets
        while( 1 )
        {
            HNodeSWDPacket    packet;
            //HNodeSensorMeasurement reading;
            uint32_t recvd = 0;

            //std::cout << "start recvd..." << std::endl;

            recvd += recv( fd, packet.getPacketPtr(), packet.getMaxPacketLength(), 0 );

            switch( packet.getType() )
            {
                case HNSWP_TYPE_STATUS:
                {
                    char timeBuf[64];
                    std::string health;
                    struct timeval statusTime;
                    struct timeval lastMeasurementTime;
                    unsigned long measurementCount;
                    std::string msg;
                  
                    // Decode the packet data.
                    health = ( packet.getActionIndex() == 1 ) ? "OK" : "Degraded";

                    statusTime.tv_sec  = packet.getParam( 0 );
                    statusTime.tv_usec = packet.getParam( 1 );

                    lastMeasurementTime.tv_sec  = packet.getParam( 2 );
                    lastMeasurementTime.tv_usec = packet.getParam( 3 );

                    measurementCount = packet.getParam( 4 );
       
                    msg.assign( (const char *)packet.getPayloadPtr(), packet.getPayloadLength() );

                    // Output the status line
                    std::cout << "Status: " << health;

                    strftime( timeBuf, sizeof timeBuf, "%H:%M:%S", localtime( &(statusTime.tv_sec) ) );
                    std::cout << "  CT: " << timeBuf;

                    strftime( timeBuf, sizeof timeBuf, "%H:%M:%S", localtime( &(lastMeasurementTime.tv_sec) ) );
                    std::cout << "  MT: " << timeBuf;
                    std::cout << "  MC: " << measurementCount;
                    std::cout << "  Msg: " << msg;
                    std::cout << std::endl;
                }
                break;

                default:
                {
                    std::cout << "Unknown Packet Type - len: " << recvd << "  type: " << packet.getType() << std::endl;
                }
                break;
            }
        }
    }
    else
    {
        std::cout << "connection error: " << errno << "  " << strerror(errno) << std::endl;
    }

    close(fd);
}

