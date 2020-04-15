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
        bool _monitorRequested = false;
        bool _statusRequested  = false;
        bool _healthRequested  = false;
        bool _seqaddRequested  = false;

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

            options.addOption( Option("status", "s", "Make an explicit request for a status packet from the deamon.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("reset", "r", "Reset the daemon, including a re-read of its configuration.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("monitor", "m", "Leave the connection to the daemon open to monitor for asynch events.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("health", "c", "Request a component health report.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("seqadd", "q", "Schedule a uniform manual switch sequence.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));
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
            else if( "status" == name )
                _statusRequested = true;
            else if( "reset" == name )
                _resetRequested = true;
            else if( "health" == name )
                _healthRequested = true;
            else if( "seqadd" == name )
                _seqaddRequested = true;

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
                printf( "openClientSocket success - fd: %d\n", sockfd );
                return false;
            }

            // Failure
            return true;
        }

        int main( const ArgVec& args )
        {
            uint sockfd = 0;

            // Bailout if help was requested.
            if( _helpRequested == true )
                return Application::EXIT_OK;

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

                std::cout << "Sending a RESET request..." << std::endl;

                packet.sendAll( sockfd );

            }
            else if( _statusRequested == true )
            {
                HNSWDPacketClient packet;
                uint32_t length;

                packet.setType( HNSWD_PTYPE_STATUS_REQ );

                std::cout << "Sending a STATUS request..." << std::endl;

                packet.sendAll( sockfd );

            }
            else if( _healthRequested == true )
            {
                HNSWDPacketClient packet;
                uint32_t length;

                packet.setType( HNSWD_PTYPE_HEALTH_REQ );

                std::cout << "Sending a HEALTH request..." << std::endl;

                packet.sendAll( sockfd );
            }
            else if( _seqaddRequested == true )
            {
                HNSWDPacketClient packet;
                uint32_t length;

                packet.setType( HNSWD_PTYPE_SEQ_ADD_REQ );

                std::cout << "Sending a Uniform Sequence Add request..." << std::endl;

                packet.sendAll( sockfd );
            }

            // Listen for packets
            while( 1 )
            {
                HNSWDPacketClient    packet;
                //HNodeSensorMeasurement reading;
                HNSWDP_RESULT_T result;

                printf( "Waiting for packet reception...\n" );

                // Read the header portion of the packet
                result = packet.rcvHeader( sockfd );
                if( result != HNSWDP_RESULT_SUCCESS )
                {
                    printf( "ERROR: Failed while receiving packet header." );
                    return Application::EXIT_SOFTWARE;
                } 

                // Read any payload portion of the packet
                result = packet.rcvPayload( sockfd );
                if( result != HNSWDP_RESULT_SUCCESS )
                {
                    printf( "ERROR: Failed while receiving packet payload." );
                    return Application::EXIT_SOFTWARE;
                } 

                switch( packet.getType() )
                {
                    case HNSWD_PTYPE_DAEMON_STATUS:
                    {
                        std::string msg;
                        packet.getMsg( msg );
                        std::cout << "Daemon Status Recieved: " << msg << std::endl;
#if 0
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
#endif
                    }
                    break;

                    case HNSWD_PTYPE_HEALTH_RSP:
                    {
                        std::string msg;
                        packet.getMsg( msg );
                        std::cout << "Component Health Response Recieved: " << msg << std::endl;
                    }
                    break;

                    case HNSWD_PTYPE_SEQ_RSP:
                    {
                        std::string msg;
                        packet.getMsg( msg );
                        std::cout << "Uniform Sequence Response Recieved: " << msg << std::endl;
                    }
                    break;

                    default:
                    {
                        std::cout << "Unknown Packet Type -  type: " << packet.getType() << std::endl;
                    }
                    break;
                }

            }

            close( sockfd );

            return Application::EXIT_OK;
        }
};

POCO_APP_MAIN( HNSwitchClient )

