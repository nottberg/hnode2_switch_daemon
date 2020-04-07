#include <stdint.h>
#include <iostream>
#include <cstddef>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include <iostream>
#include <sstream>

#include "HNSWDPacketClient.h"

using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::OptionCallback;

class HNSwitchClient: public Application
{
    private:
        bool _helpRequested    = false;
        bool _resetRequested   = false;
        bool _pingRequested    = false;
        bool _monitorRequested = false;
        bool _statusRequested  = false;

    public:
	    HNSwitchClient()
	    {
        }

    protected:	
	    void initialize( Application& self )
	    {
		    //loadConfiguration(); 
		    Application::initialize( self );

		    // add your own initialization code here
	    }
	
        void uninitialize()
        {
		    // add your own uninitialization code here

            Application::uninitialize();
        }
	
        void reinitialize( Application& self )
        {
            Application::reinitialize( self );
            
            // add your own reinitialization code here
        }
	
        void defineOptions( OptionSet& options )
        {
            Application::defineOptions( options );

		    options.addOption( Option("help", "h", "display help information on command line arguments").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleHelp)));

		    //options.addOption( Option("define", "D", "define a configuration property").required(false).repeatable(true).argument("name=value").callback(OptionCallback<SampleApp>(this, &SampleApp::handleDefine)));
				
		    //options.addOption( Option("config-file", "f", "load configuration data from a file").required(false).repeatable(true).argument("file").callback(OptionCallback<SampleApp>(this, &SampleApp::handleConfig)));

            options.addOption( Option("monitor", "m", "Leave the connection to the daemon open to monitor for asynch events.").required(false).repeatable(false));
        }
	
        void handleHelp(const std::string& name, const std::string& value)
        {
            _helpRequested = true;
            displayHelp();
            stopOptionsProcessing();
        }
			
        void handleOptions( const std::string& name, const std::string& value )
        {
            if( "monitor" == name )
                _monitorRequested = true;
            else if( "ping" == name )
                _pingRequested = true;
            else if( "reset" == name )
                _resetRequested = true;
            else if( "status" == name )
                _statusRequested = true;
        }

        void displayHelp()
        {
            HelpFormatter helpFormatter(options());
            helpFormatter.setCommand(commandName());
            helpFormatter.setUsage("OPTIONS");
            helpFormatter.setHeader("A simple command line client for the hnswitchd daemon.");
            helpFormatter.format(std::cout);
        }

        bool openClientSocket( std::string deviceName, std::string instanceName, uint &sockfd )
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

            // Register the socket
            sockfd = socket( AF_UNIX, SOCK_SEQPACKET, 0 );

            // Establish the connection.
            if( connect( sockfd, (struct sockaddr *) &addr, ( sizeof( sa_family_t ) + strlen( str ) + 1 ) ) == 0 )
            {
                // Success
                return false;
            }

            // Failure
            return true;
        }

        int main( const ArgVec& args )
        {
//            struct sockaddr_un addr;
//            const char *str = "hnswitchd";
            uint sockfd = 0;

            // Bailout if help was requested.
            if( _helpRequested == true )
                return Application::EXIT_OK;

            //logger().information("Command line:");

            // Clear address structure - Unix domain addressing
            // addr.sun_path[0] will be zeroed due to memset 
//            memset( &addr, 0, sizeof(struct sockaddr_un) );  
//            addr.sun_family = AF_UNIX;                     

            // Abstract socket with name @acrt5n1d_readings
//            strncpy( &addr.sun_path[1], str, strlen(str) );

//            int fd = socket( AF_UNIX, SOCK_SEQPACKET, 0 );

            // Establish the connection.
            if( openClientSocket( "switch-daemon", "default", sockfd ) == true )
            {
                std::cerr << "ERROR: Could not establish connection to daemon. Exiting..." << std::endl;
                return Application::EXIT_SOFTWARE;
            }

            //           
            if( _resetRequested == true )
            {
                HNSWDPacketClient packet;
                uint32_t length;

                packet.setType( HNSWD_PTYPE_RESET_REQ );

                std::cout << "Sending a RESET packet...";

                //length = send( sockfd, packet.getPacketPtr(), packet.getPacketLength(), MSG_NOSIGNAL );

                std::cout << length << " bytes sent." << std::endl;
            }
            else if( _pingRequested == true )
            {
                HNSWDPacketClient packet;
                uint32_t length;

                packet.setType( HNSWD_PTYPE_PING_REQ );

                std::cout << "Sending a PING packet...";

                //length = send( sockfd, packet.getPacketPtr(), packet.getPacketLength(), MSG_NOSIGNAL );

                std::cout << length << " bytes sent." << std::endl;
            }

            // Listen for packets
            while( 1 )
            {
                HNSWDPacketClient    packet;
                //HNodeSensorMeasurement reading;
                uint32_t recvd = 0;

                //std::cout << "start recvd..." << std::endl;
                //recvd += recv( sockfd, packet.getPacketPtr(), packet.getMaxPacketLength(), 0 );

#if 0
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
#endif
            }

            close( sockfd );

            return Application::EXIT_OK;
        }
};

POCO_APP_MAIN( HNSwitchClient )

