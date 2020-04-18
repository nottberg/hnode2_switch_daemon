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
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "HNSWDPacketClient.h"

using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::OptionCallback;

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

class HNSwitchClient: public Application
{
    private:
        bool _helpRequested      = false;
        bool _resetRequested     = false;
        bool _monitorRequested   = false;
        bool _statusRequested    = false;
        bool _healthRequested    = false;
        bool _seqaddRequested    = false;
        bool _swinfoRequested    = false;
        bool _seqcancelRequested = false;
        bool _schstateRequested  = false;
        bool _durationPresent    = false;
        bool _instancePresent    = false;
        
        std::string _seqaddFilePath;
        std::string _schstateNewState;
        std::string _durationStr;
        std::string _instanceStr;

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

            options.addOption( Option("instance", "", "The instance of the daemon to connect to.").required(false).repeatable(false).argument("00:00:00").callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("status", "s", "Make an explicit request for a status packet from the deamon.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("reset", "r", "Reset the daemon, including a re-read of its configuration.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("monitor", "m", "Leave the connection to the daemon open to monitor for asynch events.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("health", "c", "Request a component health report.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("switch", "i", "Request information about managed switches.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("seqadd", "q", "Add a uniform manual switch sequence.  Sequence is defined in provided json file.").required(false).repeatable(false).argument("seqfile").callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("seqcancel", "x", "Cancel any previously added switch sequences.").required(false).repeatable(false).callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("schstate", "e", "Change the scheduler state. Possible states: enabled|disabled|inhibit. For inhibit the duration parameter is also required.").required(false).repeatable(false).argument("newstate").callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

            options.addOption( Option("duration", "d", "Duration in HH:MM:SS format.").required(false).repeatable(false).argument("00:00:00").callback(OptionCallback<HNSwitchClient>(this, &HNSwitchClient::handleOptions)));

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
            {
                _seqaddRequested = true;
                _seqaddFilePath  = value;
            }
            else if( "switch" == name )
                _swinfoRequested = true;
            else if( "seqcancel" == name )
                _seqcancelRequested = true;
            else if( "schstate" == name )
            {
                _schstateRequested = true;
                _schstateNewState  = value;
            }
            else if( "duration" == name )
            {
                _durationPresent = true;
                _durationStr     = value;
            }
            else if( "instance" == name )
            {
                _instancePresent = true;
                _instanceStr     = value;
            }

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
                printf( "Successfully opened client socket on file descriptor: %d\n", sockfd );
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
 
            if( _instancePresent == false )
                _instanceStr = HN_SWDAEMON_DEF_INSTANCE;

            // Establish the connection.
            if( openClientSocket( HN_SWDAEMON_DEVICE_NAME, _instanceStr, sockfd ) == true )
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
            else if( _swinfoRequested == true )
            {
                HNSWDPacketClient packet;

                packet.setType( HNSWD_PTYPE_SWINFO_REQ );

                std::cout << "Sending a SWITCH INFO request..." << std::endl;

                packet.sendAll( sockfd );
            }
            else if( _seqaddRequested == true )
            {
                std::stringstream msg;

                Poco::Path path( _seqaddFilePath );
                Poco::File file( path );

                if( file.exists() == false || file.isFile() == false )
                {
                    std::cerr << "ERROR: Sequence definition file does not exist: " << path.toString() << std::endl;
                    return Application::EXIT_SOFTWARE;
                }
            
                // Open a stream for reading
                std::ifstream its;
                its.open( path.toString() );

                if( its.is_open() == false )
                {
                    std::cerr << "ERROR: Sequence definition file could not be opened: " << path.toString() << std::endl;
                    return Application::EXIT_SOFTWARE;
                }

                // Invoke the json parser
                try
                {
                    // Attempt to parse the json    
                    pjs::Parser parser;
                    pdy::Var varRoot = parser.parse( its );
                    its.close();

                    // Get a pointer to the root object
                    pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();

                    // Write out the generated json
                    pjs::Stringifier::stringify( jsRoot, msg );
                }
                catch( Poco::Exception ex )
                {
                    its.close();
                    std::cerr << "ERROR: Sequence definition file json parsing failure: " << ex.displayText().c_str() << std::endl;
                    return Application::EXIT_SOFTWARE;
                }

                HNSWDPacketClient packet( HNSWD_PTYPE_USEQ_ADD_REQ, HNSWD_RCODE_NOTSET, msg.str() );

                std::cout << "Sending a Uniform Sequence Add request..." << std::endl;

                packet.sendAll( sockfd );
            }
            else if( _seqcancelRequested == true )
            {
                HNSWDPacketClient packet;

                packet.setType( HNSWD_PTYPE_SEQ_CANCEL_REQ );

                std::cout << "Sending a SEQUENCE CANCEL request..." << std::endl;

                packet.sendAll( sockfd );
            }
            else if( _schstateRequested == true )
            {
                std::stringstream msg;

                // Error check the provided parameters
                if(   ( _schstateNewState != "enable" )
                   && ( _schstateNewState != "disable" )
                   && ( _schstateNewState != "inhibit" ) )
                {
                    std::cout << "ERROR: Request scheduling state is not supported: " << _schstateNewState << std::endl;
                    return Application::EXIT_SOFTWARE;
                }

                if( ( _schstateNewState == "inhibit" ) && ( _durationPresent == false ) )
                {
                    std::cout << "ERROR: When requesting the inhibit state a duration must be provided: " << _durationStr << std::endl;
                    return Application::EXIT_SOFTWARE;
                }
          
                // Build the payload message
                // Create a json root object
                pjs::Object jsRoot;

                // Add the timezone setting
                jsRoot.set( "state", _schstateNewState );

                // Add the current date
                if( _durationPresent )
                    jsRoot.set( "inhibitDuration", _durationStr );
                else
                    jsRoot.set( "inhibitDuration", "00:00:00" );

                // Render into a json string.
                try
                {
                    pjs::Stringifier::stringify( jsRoot, msg );
                }
                catch( ... )
                {
                    return Application::EXIT_SOFTWARE;
                }

                // Build the request packet.
                HNSWDPacketClient packet( HNSWD_PTYPE_SCH_STATE_REQ, HNSWD_RCODE_NOTSET, msg.str() );

                std::cout << "Sending a SCHEDULING STATE request..." << std::endl;

                packet.sendAll( sockfd );
            }

            // Listen for packets
            bool quit = false;
            while( quit == false )
            {
                HNSWDPacketClient packet;
                HNSWDP_RESULT_T   result;

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
                        std::cout << "=== Daemon Status Recieved - result code: " << packet.getResult() << " ===" << std::endl;

                        // Parse the response
                        try
                        {
                            std::string empty;
                            pjs::Parser parser;

                            // Attempt to parse the json
                            pdy::Var varRoot = parser.parse( msg );

                            // Get a pointer to the root object
                            pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();

                            std::string date = jsRoot->optValue( "date", empty );
                            std::string time = jsRoot->optValue( "time", empty );
                            std::string tz   = jsRoot->optValue( "timezone", empty );
                            std::string swON = jsRoot->optValue( "swOnList", empty );

                            std::string schState = jsRoot->optValue( "schedulerState", empty );
                            std::string inhUntil = jsRoot->optValue( "inhibitUntil", empty );

                            pjs::Object::Ptr jsOHealth = jsRoot->getObject( "overallHealth" );
                            
                            std::string ohstat = jsOHealth->optValue( "status", empty );
                            std::string ohmsg = jsOHealth->optValue( "msg", empty ); 

                            printf( "       Date: %s\n", date.c_str() );
                            printf( "       Time: %s\n", time.c_str() );
                            printf( "   Timezone: %s\n\n", tz.c_str() );
                            printf( "   Schduler State: %s\n", schState.c_str() );
                            printf( "    Inhibit Until: %s\n\n", inhUntil.c_str() );
                            printf( "  Switch On: %s\n", swON.c_str() );
                            printf( "     Health: %s (%s)\n", ohstat.c_str(), ohmsg.c_str() );
                        }
                        catch( Poco::Exception ex )
                        {
                            std::cout << "  ERROR: Response message not parsable: " << msg << std::endl;
                        }

                        // Exit if we received the expected response and monitoring wasn't requested.
                        if( _monitorRequested == false )
                            quit = true;
                    }
                    break;

                    case HNSWD_PTYPE_HEALTH_RSP:
                    {
                        std::string msg;

                        // Get the json response string.
                        packet.getMsg( msg );
                        std::cout << "=== Component Health Response Recieved - result code: " << packet.getResult() << " ===" << std::endl;

                        // Parse and format the response
                        try
                        {
                            std::string empty;
                            pjs::Parser parser;

                            // Attempt to parse the json
                            pdy::Var varRoot = parser.parse( msg );

                            // Get a pointer to the root object
                            pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();

                            pjs::Object::Ptr jsOHealth = jsRoot->getObject( "overallHealth" );
                            
                            std::string ohstat = jsOHealth->optValue( "status", empty );
                            std::string ohmsg = jsOHealth->optValue( "msg", empty ); 

                            printf( "  Component                    |   Status   |   (Error Code) Message\n" );
                            printf( "  ------------------------------------------------------------------\n" );
                            printf( "  %-29.29s %-12.12s (%s) %s\n", "overall", ohstat.c_str(), "0", ohmsg.c_str() );

                            pjs::Array::Ptr jsCHealth = jsRoot->getArray( "componentHealth" );

                            for( uint index = 0; index < jsCHealth->size(); index++ )
                            {
                                if( jsCHealth->isObject( index ) == false )
                                    continue;
                                
                                pjs::Object::Ptr jsCHObject = jsCHealth->getObject( index );

                                std::string component = jsCHObject->optValue( "component", empty );
                                std::string status    = jsCHObject->optValue( "status", empty );
                                std::string errCode   = jsCHObject->optValue( "errCode", empty );
                                std::string msg       = jsCHObject->optValue( "msg", empty );

                                printf( "  %-29.29s %-12.12s (%s) %s\n", component.c_str(), status.c_str(), errCode.c_str(), msg.c_str() );
                            }
                        }
                        catch( Poco::Exception ex )
                        {
                            std::cout << "  ERROR: Response message not parsable: " << msg << std::endl;
                        }

                        // Exit if we received the expected response and monitoring wasn't requested.
                        if( (_healthRequested == true) && (_monitorRequested == false ) )
                            quit = true;

                    }
                    break;

                    case HNSWD_PTYPE_SWINFO_RSP:
                    {
                        std::string msg;

                        // Get the json response string.
                        packet.getMsg( msg );
                        std::cout << "=== Switch Info Response Recieved - result code: " << packet.getResult() << " ===" << std::endl;

                        // Parse and format the response
                        try
                        {
                            std::string empty;
                            pjs::Parser parser;

                            // Attempt to parse the json
                            pdy::Var varRoot = parser.parse( msg );

                            // Get a pointer to the root object
                            pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();

                            pjs::Array::Ptr jsSWList = jsRoot->getArray( "swList" );

                            printf( "  swid                |   Description  \n" );
                            printf( "  -------------------------------------\n" );

                            for( uint index = 0; index < jsSWList->size(); index++ )
                            {
                                if( jsSWList->isObject( index ) == false )
                                    continue;
                                
                                pjs::Object::Ptr jsSWInfo = jsSWList->getObject( index );

                                std::string swid = jsSWInfo->optValue( "swid", empty );
                                std::string desc = jsSWInfo->optValue( "description", empty );

                                printf( "  %-20.20s %s\n", swid.c_str(), desc.c_str() );
                            }
                        }
                        catch( Poco::Exception ex )
                        {
                            std::cout << "  ERROR: Response message not parsable: " << msg << std::endl;
                        }

                        // Exit if we received the expected response and monitoring wasn't requested.
                        if( (_swinfoRequested == true) && (_monitorRequested == false ) )
                            quit = true;

                    }
                    break;

                    case HNSWD_PTYPE_USEQ_ADD_RSP:
                    {
                        std::string msg;
                        packet.getMsg( msg );
                        std::cout << "=== Uniform Sequence Add Response Recieved - result code: " << packet.getResult() << " ===" << std::endl;

                        // Exit if we received the expected response and monitoring wasn't requested.
                        if( (_seqaddRequested == true) && (_monitorRequested == false ) )
                            quit = true;

                    }
                    break;

                    case HNSWD_PTYPE_SEQ_CANCEL_RSP:
                    {
                        std::string msg;
                        packet.getMsg( msg );
                        std::cout << "=== Sequence Cancel Response Recieved - result code: " << packet.getResult() << " ===" << std::endl;

                        // Exit if we received the expected response and monitoring wasn't requested.
                        if( (_seqcancelRequested == true) && (_monitorRequested == false ) )
                            quit = true;

                    }
                    break;

                    case HNSWD_PTYPE_SCH_STATE_RSP:
                    {
                        std::string msg;
                        packet.getMsg( msg );
                        std::cout << "=== Schedule State Response Recieved - result code: " << packet.getResult() << " ===" << std::endl;

                        // Exit if we received the expected response and monitoring wasn't requested.
                        if( (_schstateRequested == true) && (_monitorRequested == false ) )
                            quit = true;

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

