#include <iostream>
#include <iomanip>
#include <fstream>
#include <ios>
#include <exception>
#include <iterator>
#include <regex>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include "HNScheduleMatrix.h"

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

// The order of this array must match the cooresponding
// enumeration from the header file.
static const char* scDayNames[] =
{
  "Sunday",    // HNS_DINDX_SUNDAY
  "Monday",    // HNS_DINDX_MONDAY
  "Tuesday",   // HNS_DINDX_TUESDAY
  "Wednesday", // HNS_DINDX_WEDNESDAY
  "Thursday",  // HNS_DINDX_THURSDAY
  "Friday",    // HNS_DINDX_FRIDAY
  "Saturday"   // HNS_DINDX_SATURDAY 
};


HNS24HTime::HNS24HTime()
{
    secOfDay = 0;
}

HNS24HTime::~HNS24HTime()
{

}

uint
HNS24HTime::getSeconds() const
{
    return secOfDay;
}

void
HNS24HTime::getHMS( uint &hour, uint &minute, uint &second )
{
    hour    = (secOfDay / (60 * 60));
    minute  = (secOfDay - (hour * 60 * 60))/60;
    second  = secOfDay - ((hour * 60 * 60) + (minute * 60));
}

std::string
HNS24HTime::getHMSStr()
{
    char tmpBuf[128];
    uint hour, minute, second;

    getHMS( hour, minute, second );
    sprintf( tmpBuf, "%2.2d:%2.2d:%2.2d", hour, minute, second );

    std::string result( tmpBuf );
    return result;
}

HNSM_RESULT_T
HNS24HTime::setFromHMS( uint hour, uint min, uint sec )
{
    secOfDay = (hour * 60 * 60) + (min *60) + sec;

    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNS24HTime::setFromSeconds( uint seconds )
{
    // Set to provided value, cieling 24 hours.
    secOfDay = seconds;
    if( secOfDay > (24 * 60 * 60) )
        secOfDay = (24 * 60 * 60);

    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNS24HTime::parseTime( std::string value )
{
    uint hour;
    uint min;
    uint sec;

    sscanf( value.c_str(), "%d:%d:%d", &hour, &min, &sec );

    return setFromHMS( hour, min, sec );
}

void
HNS24HTime::addSeconds( uint seconds )
{
    // Add the two times, cap 24 hours
    secOfDay = (secOfDay + seconds); 
    if( secOfDay > (24 * 60 * 60) )
        secOfDay = (24 * 60 * 60);

}

void
HNS24HTime::addDuration( HNS24HTime &duration )
{
    // Add the two times, cap 24 hours
    secOfDay = (secOfDay + duration.secOfDay); 
    if( secOfDay > (24 * 60 * 60) )
        secOfDay = (24 * 60 * 60);
}

HNSAction::HNSAction()
{

}

HNSAction::HNSAction( HNS_ACT_T actval, HNS24HTime &startTime, HNS24HTime &endTime, std::string swID )
{
    action = actval;
    start  = startTime;
    end    = endTime;
    swid   = swID;
}

HNSAction::~HNSAction()
{

}

HNSM_RESULT_T 
HNSAction::handlePair( std::string key, std::string value )
{
    if( "action" == key )
    {
        if( "swon" == value )
        {
            action = HNS_ACT_SWON;
        }
        else
        {
            return HNSM_RESULT_FAILURE;
        }
    }
    else if( "startTime" == key )
    {
        start.parseTime( value );
    }
    else if( "endTime" == key )
    {
        end.parseTime( value );
    }
    else if( "swid" == key )
    {
        swid = value;
    }

    return HNSM_RESULT_SUCCESS;
}

uint
HNSAction::getStartSeconds() const
{
    return start.getSeconds();
}

uint
HNSAction::getEndSeconds() const
{
    return end.getSeconds();
}

std::string 
HNSAction::getSwID()
{
    return swid;
}

bool 
HNSAction::startIsBefore( HNS24HTime &tgtTime )
{
   if( start.getSeconds() <= tgtTime.getSeconds() )
       return true;
   return false;
}

bool 
HNSAction::startIsAfter( HNS24HTime &tgtTime )
{
   if( start.getSeconds() > tgtTime.getSeconds() )
       return true;
   return false;
}
   
bool 
HNSAction::endIsAfter( HNS24HTime &tgtTime )
{
   if( end.getSeconds() > tgtTime.getSeconds() )
       return true;
   return false;
}

bool 
HNSAction::sortCompare( const HNSAction& first, const HNSAction& second )
{
  return ( first.getStartSeconds() < second.getStartSeconds() );
}

void
HNSAction::debugPrint( uint offset, HNDaemonLogSrc &log )
{
    log.debug( "%*.*caction: %d  start: %s  end: %s  switch: %s", offset, offset, ' ', action, start.getHMSStr().c_str(), end.getHMSStr().c_str(), swid.c_str() );
}

HNSDay::HNSDay()
{
    dindx = HNS_DAY_CNT;
}

HNSDay::~HNSDay()
{

}

void 
HNSDay::setID( HNS_DAY_INDX_T index, std::string name )
{
    dindx = index;
    dayName = name;
}

void
HNSDay::clear()
{
    actionArr.clear();
}

HNSM_RESULT_T 
HNSDay::addAction( HNSAction &action )
{
    actionArr.push_back( action );
    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNSDay::getSwitchOnList( HNS24HTime &tgtTime, std::vector< std::string > &swidList )
{
    // Cycle through actions until start times are after the tgtTime
    for( std::list< HNSAction >::iterator it = actionArr.begin(); it != actionArr.end(); it++ )
    {
        if( it->startIsBefore( tgtTime ) && it->endIsAfter( tgtTime ) )
        {
            swidList.push_back( it->getSwID() );
        }
        else if( it->startIsAfter( tgtTime ) )
        {
            break;
        }
    } 

    return HNSM_RESULT_SUCCESS;
}

void 
HNSDay::sortActions()
{
    actionArr.sort( HNSAction::sortCompare );
}

void
HNSDay::debugPrint( uint offset, HNDaemonLogSrc &log )
{
    log.debug( "%*.*cDay: %s", offset, offset, ' ', dayName.c_str() );

    for( std::list< HNSAction >::iterator it = actionArr.begin(); it != actionArr.end(); it++ )
    {
        it->debugPrint( offset + 2, log );
    }
}

HNScheduleMatrix::HNScheduleMatrix()
: m_health( "Schedule Matrix" )
{
    m_rootDirPath = HNS_SCH_CFG_DEFAULT_PATH;

    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
        m_dayArr[i].setID( (HNS_DAY_INDX_T) i, scDayNames[i] );
    }

    m_state = HNSM_SCHSTATE_ENABLED;

    m_schUpdateIndex = 0;
}

HNScheduleMatrix::~HNScheduleMatrix()
{

}

void 
HNScheduleMatrix::setDstLog( HNDaemonLog *logPtr )
{
    m_log.setDstLog( logPtr );
}

void 
HNScheduleMatrix::setRootDirectory( std::string path )
{
    m_rootDirPath = path; 
}

std::string 
HNScheduleMatrix::getRootDirectory()
{
    return m_rootDirPath;
}

void 
HNScheduleMatrix::setTimezone( std::string tzname )
{
    m_timezone = tzname;
}

void 
HNScheduleMatrix::setScheduleCRC32Str( std::string value )
{
    m_schCRC32 = value;
}

std::string
HNScheduleMatrix::getTimezone()
{
    return m_timezone;
}

uint 
HNScheduleMatrix::getScheduleUpdateIndex()
{
    return m_schUpdateIndex;
}

std::string 
HNScheduleMatrix::getScheduleCRC32Str()
{
    return m_schCRC32;
}

HNSM_RESULT_T 
HNScheduleMatrix::generateScheduleFilePath( std::string &fpath )
{
    char tmpBuf[ 256 ];
 
    fpath.clear();

    if( m_deviceName.empty() == true )
        return HNSM_RESULT_FAILURE;

    if( m_instanceName.empty() == true )
        return HNSM_RESULT_FAILURE;
  
    sprintf( tmpBuf, "%s-%s", m_deviceName.c_str(), m_instanceName.c_str() );

    Poco::Path path( m_rootDirPath );
    path.makeDirectory();
    path.pushDirectory( tmpBuf );
    path.setFileName("scheduleMatrix.json");

    fpath = path.toString();

    return HNSM_RESULT_SUCCESS;
}

void
HNScheduleMatrix::clear()
{
    m_timezone.clear();
    m_schCRC32.clear();

    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
       m_dayArr[i].clear();
       m_dayArr[i].setID( (HNS_DAY_INDX_T) i, scDayNames[i] );
    }
}

void 
HNScheduleMatrix::setInstance( std::string devname, std::string instance )
{
    // Copy over identifying information
    m_deviceName   = devname;
    m_instanceName = instance;
}

HNSM_RESULT_T 
HNScheduleMatrix::loadSchedule()
{
    std::string fpath;

    // Clear any existing matrix information
    clear();

    // Generate and verify filename
    if( generateScheduleFilePath( fpath ) != HNSM_RESULT_SUCCESS )
    {
        m_log.error( "ERROR: Failed to generate path to schedule matrix config for: %s %s", m_deviceName.c_str(), m_instanceName.c_str() );
        m_health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SM_FAILED_PATH_GEN, m_deviceName.c_str(), m_instanceName.c_str() );
        return HNSM_RESULT_FAILURE;
    }    

    // Build target file path and verify existance
    Poco::Path path( fpath );
    Poco::File file( path );

    m_log.info( "Attempting to load schedule matrix config from: %s", path.toString().c_str() );

    if( file.exists() == false || file.isFile() == false )
    {
        m_log.error( "ERROR: Schedule matrix config file does not exist: %s", path.toString().c_str() );
        m_health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SM_CONFIG_MISSING, path.toString().c_str() ); 
        return HNSM_RESULT_FAILURE;
    }

    // Open a stream for reading
    std::ifstream its;
    its.open( path.toString() );

    if( its.is_open() == false )
    {
        m_log.error( "ERROR: Schedule matrix config file open failed: %s", path.toString().c_str() );
        m_health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SM_CONFIG_OPEN, path.toString().c_str() ); 
        return HNSM_RESULT_FAILURE;
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

        // Handle each key of the root objects
        for( pjs::Object::ConstIterator it = jsRoot->begin(); it != jsRoot->end(); it++ )
        {
            if( it->first == "scheduleTimezone" )
            {
                if( it->second.isString() == false )
                {
                    m_log.warn( "Warning: Non-string scheduleTimezone value." );
                    continue;
                }

                setTimezone( it->second );
            }
            else if( it->first == "scheduleCRC32" )
            {
                if( it->second.isString() == false )
                {
                    m_log.warn( "Warning: Non-string scheduleCRC32 value." );
                    continue;
                }

                setScheduleCRC32Str( it->second );
            }
            else if( it->first == "scheduleMatrix" )
            {
                if( jsRoot->isObject( it ) == false )
                {
                    m_log.warn( "Warning: Non-object scheduleMatrix value." );
                    continue;
                }

                pjs::Object::Ptr jsDayObj = jsRoot->getObject( it->first );

                for( pjs::Object::ConstIterator dit = jsDayObj->begin(); dit != jsDayObj->end(); dit++ )
                {

                    for( uint dindx = 0; dindx < HNS_DAY_CNT; dindx++ )
                    {
                        if( scDayNames[dindx] == dit->first )
                        {
                            if( jsDayObj->isArray( dit ) == false )
                            {
                                m_log.warn( "Warning: Non-array day object value: %s", dit->first.c_str() );
                                break;
                            }

                            pjs::Array::Ptr jsActArr = jsDayObj->getArray( dit->first );

                            for( uint index = 0; index < jsActArr->size(); index++ )
                            {
                                if( jsActArr->isObject( index ) == true )
                                {
                                    pjs::Object::Ptr jsAct = jsActArr->getObject( index );

                                    HNSAction tmpAction;
                                    
                                    for( pjs::Object::ConstIterator aoit = jsAct->begin(); aoit != jsAct->end(); aoit++ )
                                    {
                                        if( aoit->second.isString() == false )
                                        {
                                            m_log.warn( "Warning: Non-string action object value" );
                                            break;
                                        }
                                        
                                        tmpAction.handlePair( aoit->first, aoit->second );
                                    }

                                    m_dayArr[ dindx ].addAction( tmpAction );
                                }
                                else
                                {
                                    m_log.warn( "Warning: Non-object type for day action." );
                                }
                            }

                            // Correct day already found,
                            // skip any additional loop iterations.
                            break;
                        } 

                    }
                }

            }
        }
    }
    catch( Poco::Exception ex )
    {
        m_log.error( "ERROR: Schedule matrix config file json parse failure: %s", ex.displayText().c_str() );
        m_health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SM_CONFIG_PARSER, ex.displayText().c_str() ); 
        its.close();
        return HNSM_RESULT_FAILURE;
    }

    // Sort the actions
    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
       m_dayArr[i].sortActions();
    }
    
    // 
    debugPrint();

    m_log.info( "Schedule matrix config successfully loaded." );

    m_health.setOK();

    // Done
    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNScheduleMatrix::replaceSchedule( std::string newSchJSON )
{
    std::string fpath;

    m_log.info( "Replacing schedule matrix configuration." );

    // Create directories if necessary
    if( createDirectories() != HNSM_RESULT_SUCCESS )
    {
        m_log.error( "ERROR: Failed to create directories for schedule matrix config: %s %s", m_deviceName.c_str(), m_instanceName.c_str() );
        return HNSM_RESULT_FAILURE;
    }

    // Generate and verify filename
    if( generateScheduleFilePath( fpath ) != HNSM_RESULT_SUCCESS )
    {
        m_log.error( "ERROR: Failed to generate path to schedule matrix config for: %s %s", m_deviceName.c_str(), m_instanceName.c_str() );
        m_health.setStatusMsg( HN_HEALTH_FAILED, HNSWD_ECODE_SM_FAILED_PATH_GEN, m_deviceName.c_str(), m_instanceName.c_str() );
        return HNSM_RESULT_FAILURE;
    }    

    // Build target file path and verify existance
    Poco::Path path( fpath );
    Poco::File file( path );

    // Open a stream for writinging
    std::ofstream ots;
    ots.open( path.toString() );

    if( ots.is_open() == false )
    {
        return HNSM_RESULT_FAILURE;
    }

    ots.write( newSchJSON.c_str(), newSchJSON.size() );

    ots.close();
    
    // Bump the scheduleUpdateIndex
    m_schUpdateIndex += 1;

    // 
    debugPrint();

    m_log.info( "Schedule matrix config successfully replaced." );

    m_health.setOK();

    // Done
    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T
HNScheduleMatrix::createDirectories()
{
    Poco::Path path( m_rootDirPath );
    path.makeDirectory();

    Poco::File dir( path );

    if( dir.exists() )
    {
        if( dir.isDirectory() )
            return HNSM_RESULT_SUCCESS;
        else
            return HNSM_RESULT_FAILURE;
    }

    // Create any intermediate directories
    try
    {
        dir.createDirectories();
    }
    catch( Poco::Exception ex )
    {
        return HNSM_RESULT_FAILURE;
    }

    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNScheduleMatrix::generateStateFilePath( std::string &fpath )
{
    char tmpBuf[ 256 ];
 
    fpath.clear();

    if( m_deviceName.empty() == true )
        return HNSM_RESULT_FAILURE;

    if( m_instanceName.empty() == true )
        return HNSM_RESULT_FAILURE;
  
    sprintf( tmpBuf, "%s-%s", m_deviceName.c_str(), m_instanceName.c_str() );

    Poco::Path path( m_rootDirPath );
    path.makeDirectory();
    path.pushDirectory( tmpBuf );
    path.setFileName("scheduleState.json");

    fpath = path.toString();

    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNScheduleMatrix::updateStateFile( HNSM_SCHSTATE_T value )
{
    std::string fpath;
    std::string rState;

    // Create directories if necessary
    if( createDirectories() != HNSM_RESULT_SUCCESS )
    {
        return HNSM_RESULT_FAILURE;
    }

    // Generate and verify filename
    if( generateStateFilePath( fpath ) != HNSM_RESULT_SUCCESS )
    {
        return HNSM_RESULT_FAILURE;
    }    

    // Build target file path and verify existance
    Poco::Path path( fpath );
    Poco::File file( path );

    // Open a stream for writinging
    std::ofstream ots;
    ots.open( path.toString() );

    if( ots.is_open() == false )
    {
        return HNSM_RESULT_FAILURE;
    }

    // Create a json root object
    pjs::Object jsRoot;
   
    if( value == HNSM_SCHSTATE_DISABLED )
        jsRoot.set( "state", "disabled" );
    else
        jsRoot.set( "state", "enabled" );

    try
    {
        // Write out the generated json
        pjs::Stringifier::stringify( jsRoot, ots, 1 );
    }
    catch( Poco::Exception ex )
    {
        ots.close();
        return HNSM_RESULT_FAILURE;
    }

    // Close the stream
    ots.close();

    // Done
    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNScheduleMatrix::readStateFile( HNSM_SCHSTATE_T &value )
{
    std::string fpath;
    std::string rState;

    // Default return value;
    value = HNSM_SCHSTATE_ENABLED;

    // Generate and verify filename
    if( generateStateFilePath( fpath ) != HNSM_RESULT_SUCCESS )
    {
        return HNSM_RESULT_FAILURE;
    }    

    // Build target file path and verify existance
    Poco::Path path( fpath );
    Poco::File file( path );

    if( file.exists() == false || file.isFile() == false )
    {
        return HNSM_RESULT_FAILURE;
    }

    // Open a stream for reading
    std::ifstream its;
    its.open( path.toString() );

    if( its.is_open() == false )
    {
        return HNSM_RESULT_FAILURE;
    }

    // Invoke the json parser
    try
    {
        // Attempt to parse the json
        std::string empty;
        pjs::Parser parser;
        pdy::Var varRoot = parser.parse( its );
        its.close();

        // Get a pointer to the root object
        pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();
        rState = jsRoot->optValue( "state", empty );
    }
    catch( Poco::Exception ex )
    {
        its.close();
        return HNSM_RESULT_FAILURE;
    }

    // Set the output value if appropriate
    if( "disabled" == rState )
        value = HNSM_SCHSTATE_DISABLED;

    // Done
    return HNSM_RESULT_SUCCESS;
}

void 
HNScheduleMatrix::initState()
{
    HNSM_SCHSTATE_T pState;

    HNSM_RESULT_T result = readStateFile( pState );

    if( result != HNSM_RESULT_SUCCESS )
    {
        m_state = HNSM_SCHSTATE_ENABLED;
        return;
    }

    m_state = pState;
}

HNSM_SCHSTATE_T 
HNScheduleMatrix::getState()
{
    return m_state;
}

std::string
HNScheduleMatrix::getStateStr()
{
    switch( m_state )
    {
        case HNSM_SCHSTATE_ENABLED:
            return "enabled";
        case HNSM_SCHSTATE_DISABLED:
            return "disabled";
        case HNSM_SCHSTATE_INHIBIT:
            return "inhibit";
    }

    return "notset";
}

void 
HNScheduleMatrix::setStateDisabled()
{
    m_state = HNSM_SCHSTATE_DISABLED;
    updateStateFile( m_state );
}

void 
HNScheduleMatrix::setStateEnabled()
{
    m_state = HNSM_SCHSTATE_ENABLED;
    updateStateFile( m_state );
}

void 
HNScheduleMatrix::setStateInhibited( struct tm *time, std::string durationStr )
{
    HNS24HTime duration;

    m_state = HNSM_SCHSTATE_INHIBIT;

    duration.parseTime( durationStr.c_str() );

    m_inhibitUntil.setFromHMS( time->tm_hour, time->tm_min, time->tm_sec );
    m_inhibitUntil.addDuration( duration );
}

std::string
HNScheduleMatrix::getInhibitUntilStr()
{
    if( m_state != HNSM_SCHSTATE_INHIBIT )
        return "00:00:00";

    return m_inhibitUntil.getHMSStr();
}

bool 
HNScheduleMatrix::checkInhibitExpire( struct tm *time )
{
    HNS24HTime curTime;

    curTime.setFromHMS(  time->tm_hour, time->tm_min, time->tm_sec );

    if( m_inhibitUntil.getSeconds() <= curTime.getSeconds() )
        return true;

    return false;
}

uint 
HNScheduleMatrix::getHealthComponentCount()
{
    return 1;
}

HNDaemonHealth* 
HNScheduleMatrix::getHealthComponent( uint index )
{
    if( index == 0 )
        return &m_health;

    return NULL;
}

HNSM_RESULT_T 
HNScheduleMatrix::getSwitchOnList( struct tm *time, std::vector< std::string > &swidList )
{
    if( time == NULL )
        return HNSM_RESULT_FAILURE;

    swidList.clear();

    HNS24HTime tgtTime;
    tgtTime.setFromHMS( time->tm_hour, time->tm_min, time->tm_sec );

    return m_dayArr[ time->tm_wday ].getSwitchOnList( tgtTime, swidList );
}

void
HNScheduleMatrix::debugPrint()
{
    m_log.debug( "==== Schedule Matrix for %s-%s ====", m_deviceName.c_str(), m_instanceName.c_str() );
    m_log.debug( "  timezone: %s", m_timezone.c_str() );
    m_log.debug( "  === Schedule Array ===" );

    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
       m_dayArr[i].debugPrint( 4, m_log );
    }
}


HNSequenceQueue::HNSequenceQueue()
{

}

HNSequenceQueue::~HNSequenceQueue()
{

}

void 
HNSequenceQueue::setDstLog( HNDaemonLog *logPtr )
{
    m_log.setDstLog( logPtr );
}

void
HNSequenceQueue::clearRequestID()
{
    m_requestID.clear();
}

HNSM_RESULT_T 
HNSequenceQueue::cancelSequences()
{
    m_requestID.clear();
    actionList.clear();
    return HNSM_RESULT_SUCCESS;
}

#if 0
// Example uniform sequence json
{
  "requestID":"sq1"
  "onTime":"00:01:00",
  "offTime":"00:05:00",
  "swidList": "sw1 sw2"
}
#endif

HNSM_RESULT_T 
HNSequenceQueue::addUniformSequence( struct tm *time, std::string seqJSON, std::string &errMsg )
{
    HNSM_RESULT_T result;
    HNS24HTime onPeriod;
    HNS24HTime offPeriod;
    const std::regex ws_re("\\s+"); // whitespace
    pjs::Parser parser;
    std::string empty;

    errMsg.clear();

    // If a sequence is already active then reject this request
    // to start a new sequence
    if( m_requestID.empty() == false )
    {
        m_log.error( "ERROR: Can't start a new sequence while existing sequence (%s) is active.", m_requestID.c_str() );
        errMsg = "ERROR: Can't start a new sequence while an existing sequence is active.";
        return HNSM_RESULT_FAILURE;
    }

    // Parse the json
    try
    {
        // Attempt to parse the json
        pdy::Var varRoot = parser.parse( seqJSON );

        // Get a pointer to the root object
        pjs::Object::Ptr jsRoot = varRoot.extract< pjs::Object::Ptr >();

        std::string reqID = jsRoot->optValue( "requestID", empty );
        if( reqID.empty() )
        {
            // Missing optional field.
            m_log.warn( "Warning: Optional 'requestID' field missing, assigning default ID" );
            m_requestID = "seqid_default";
        }
        else
        {
            m_requestID = reqID;
            m_log.info( "Queueing sequence from requestor with ID: %s", m_requestID.c_str() );
        }

        std::string onDuration = jsRoot->optValue( "onDuration", empty );
        if( onDuration.empty() )
        {
            // Missing required fields.
            m_log.warn( "Warning: Received Add Sequence request is missing the required 'onTime' field." );
            return HNSM_RESULT_FAILURE;
        }

        // Get the on time duration
        result = onPeriod.parseTime( onDuration );
        if( result != HNSM_RESULT_SUCCESS )
        {
            return result;
        }

        std::string offDuration = jsRoot->optValue( "offDuration", empty );
        if( offDuration.empty() )
        {
            // Missing required fields.
            m_log.warn( "Warning: Received Add Sequence request is missing the required 'offTime' field." );
            return HNSM_RESULT_FAILURE;
        }

        // Get the off time duration
        result = offPeriod.parseTime( offDuration );
        if( result != HNSM_RESULT_SUCCESS )
        {
            return result;
        }

        std::string swidList = jsRoot->optValue( "swidList", empty );
        if( swidList.empty() )
        {
            // Missing required fields.
            m_log.warn( "Warning: Received Add Sequence request is missing the required 'swidList' field." );
            return HNSM_RESULT_FAILURE;
        }

        // Walk the swid string
        std::sregex_token_iterator it( swidList.begin(), swidList.end(), ws_re, -1 );
        const std::sregex_token_iterator end;
        while( it != end )
        {
            HNS24HTime startTime;
            HNS24HTime endTime;

            // If the queue is empty then create the first entry
            if( actionList.empty() == true )
            {
                // The start time sould be the current time
                startTime.setFromHMS( time->tm_hour, time->tm_min, time->tm_sec );

                // End time should be start time, plus on period duration
                endTime.setFromHMS( time->tm_hour, time->tm_min, time->tm_sec );
                endTime.addDuration( onPeriod );

                // Add a new switch action to the queue.
                HNSAction swact( HNS_ACT_SWON, startTime, endTime, *it );
                actionList.push_back( swact );
            }
            else
            {
                // Since there is a preceding entry, base all of the time
                // calculation off its end time
                HNSAction& lastAct = actionList.back();

                // Init start time with ending time of previous.
                startTime.setFromSeconds( lastAct.getEndSeconds() );
            
                // Delay offPeriod seconds between switches, 
                // but ensure at least one second of delay.
                if( offPeriod.getSeconds() == 0 )
                    startTime.addSeconds( 1 );
                else
                    startTime.addDuration( offPeriod );
            
                // Based of start time, end after onPeriod seconds.
                endTime.setFromSeconds( startTime.getSeconds() );
                endTime.addDuration( onPeriod );

                // Add a new switch action to the end of the queue.
                HNSAction swact( HNS_ACT_SWON, startTime, endTime, *it );
                actionList.push_back( swact );
            }

            it++;
        }
    }
    catch( Poco::Exception ex )
    {
        m_log.error( "ERROR: Schedule matrix config file json parse failure: %s", ex.displayText().c_str() );
        return HNSM_RESULT_FAILURE;
    }

    // Success
    return HNSM_RESULT_SUCCESS;
}

bool
HNSequenceQueue::hasActions()
{
    return ( actionList.empty() ? false : true );
}

std::string
HNSequenceQueue::getRequestID()
{
    return m_requestID;
}

HNSM_RESULT_T 
HNSequenceQueue::getSwitchOnList( struct tm *time, std::vector< std::string > &swidList )
{
    if( time == NULL )
        return HNSM_RESULT_FAILURE;

    swidList.clear();

    if( actionList.empty() == true )
        return HNSM_RESULT_SUCCESS;

    HNS24HTime tgtTime;
    tgtTime.setFromHMS( time->tm_hour, time->tm_min, time->tm_sec );

    // Loop through the queue until it is empty or
    // and action has started execution.
    while( actionList.empty() == false )
    {
        // Examine the first action on the queue.
        HNSAction& curAct = actionList.front();

        // Figure out whether the action is valid
        if( curAct.startIsBefore( tgtTime ) )
        {
            // Check whether the action is active or expired.
            if( curAct.endIsAfter( tgtTime ) )
            {
                 // The current action is active, return it in the list.
                 swidList.push_back( curAct.getSwID() );
                 debugPrint();
                 return HNSM_RESULT_SUCCESS;
            }
            else
            {
                // The current time is past the end of this action
                // so get rid of the current action and check the
                // next one.
                actionList.pop_front();

                // If the action list is now empty, then 
                // clear the m_requestID since the sequence is complete
                if( actionList.empty() == true )
                    clearRequestID();

                continue;
            }
        }
        else
        {
            // Start time is after current time so nothing to do, exit
            debugPrint();
            return HNSM_RESULT_SUCCESS;
        }
    }

    debugPrint();
    return HNSM_RESULT_SUCCESS;
}

void 
HNSequenceQueue::debugPrint()
{
    m_log.debug( "==== Sequence Queue ====" );

    for( std::list< HNSAction >::iterator it = actionList.begin(); it != actionList.end(); it++ )
    {
       it->debugPrint( 2, m_log );
    }
}

