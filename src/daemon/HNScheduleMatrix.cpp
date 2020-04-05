#include <iostream>
#include <iomanip>
#include <fstream>
#include <ios>
#include <exception>

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
HNS24HTime::parseTime( std::string value )
{
    uint hour;
    uint min;
    uint sec;

    sscanf( value.c_str(), "%d:%d:%d", &hour, &min, &sec );

    return setFromHMS( hour, min, sec );
}


HNSAction::HNSAction()
{

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
{
    rootDirPath = HNS_SCH_CFG_DEFAULT_PATH;

    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
        dayArr[i].setID( (HNS_DAY_INDX_T) i, scDayNames[i] );
    }
}

HNScheduleMatrix::~HNScheduleMatrix()
{

}

void 
HNScheduleMatrix::setDstLog( HNDaemonLog *logPtr )
{
    log.setDstLog( logPtr );
}

void 
HNScheduleMatrix::setRootDirectory( std::string path )
{
    rootDirPath = path; 
}

std::string 
HNScheduleMatrix::getRootDirectory()
{
    return rootDirPath;
}

void 
HNScheduleMatrix::setTimezone( std::string tzname )
{
    timezone = tzname;
}

HNSM_RESULT_T 
HNScheduleMatrix::generateFilePath( std::string &fpath )
{
    char tmpBuf[ 256 ];
 
    fpath.clear();

    if( deviceName.empty() == true )
        return HNSM_RESULT_FAILURE;

    if( instanceName.empty() == true )
        return HNSM_RESULT_FAILURE;
  
    sprintf( tmpBuf, "%s-%s", deviceName.c_str(), instanceName.c_str() );

    Poco::Path path( rootDirPath );
    path.makeDirectory();
    path.pushDirectory( tmpBuf );
    path.setFileName("scheduleMatrix.json");

    fpath = path.toString();

    return HNSM_RESULT_SUCCESS;
}

void
HNScheduleMatrix::clear()
{
    deviceName.clear();
    instanceName.clear();
    timezone.clear();

    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
       dayArr[i].clear();
       dayArr[i].setID( (HNS_DAY_INDX_T) i, scDayNames[i] );
    }
}

HNSM_RESULT_T 
HNScheduleMatrix::loadSchedule( std::string devname, std::string instance )
{
    std::string fpath;

    // Clear any existing matrix information
    clear();

    // Copy over identifying information
    deviceName   = devname;
    instanceName = instance;

    // Generate and verify filename
    if( generateFilePath( fpath ) != HNSM_RESULT_SUCCESS )
    {
        log.error( "ERROR: Failed to schedule matrix file path for: %s %s", devname, instance );
        return HNSM_RESULT_FAILURE;
    }    

    // Build target file path and verify existance
    Poco::Path path( fpath );
    Poco::File file( path );

    log.info( "Attempting to load schedule matrix config from: %s", path.toString().c_str() );

    if( file.exists() == false || file.isFile() == false )
    {
        log.error( "ERROR: Schedule matrix config file does not exist: %s", path.toString().c_str() );
        return HNSM_RESULT_FAILURE;
    }

    // Open a stream for reading
    std::ifstream its;
    its.open( path.toString() );

    if( its.is_open() == false )
    {
        log.error( "ERROR: Schedule matrix config file open failed: %s", path.toString().c_str() );
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
                    log.warn( "Warning: Non-string scheduleTimezone value." );
                    continue;
                }

                setTimezone( it->second );
            }
            else if( it->first == "scheduleMatrix" )
            {
                if( jsRoot->isObject( it ) == false )
                {
                    log.warn( "Warning: Non-object scheduleMatrix value." );
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
                                log.warn( "Warning: Non-array day object value: %s", dit->first.c_str() );
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
                                            log.warn( "Warning: Non-string action object value" );
                                            break;
                                        }
                                        
                                        tmpAction.handlePair( aoit->first, aoit->second );
                                    }

                                    dayArr[ dindx ].addAction( tmpAction );
                                }
                                else
                                {
                                    log.warn( "Warning: Non-object type for day action." );
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
        log.error( "ERROR: Schedule matrix config file json parse failure: %s", ex.displayText().c_str() );
        its.close();
        return HNSM_RESULT_FAILURE;
    }

    // Sort the actions
    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
       dayArr[i].sortActions();
    }
    
    // 
    debugPrint();

    log.info( "Schedule matrix config successfully loaded." );

    // Done
    return HNSM_RESULT_SUCCESS;
}

HNSM_RESULT_T 
HNScheduleMatrix::getSwitchOnList( struct tm *time, std::vector< std::string > &swidList )
{
    if( time == NULL )
        return HNSM_RESULT_FAILURE;

    swidList.clear();

    HNS24HTime tgtTime;
    tgtTime.setFromHMS( time->tm_hour, time->tm_min, time->tm_sec );

    return dayArr[ time->tm_wday ].getSwitchOnList( tgtTime, swidList );
}

void
HNScheduleMatrix::debugPrint()
{
    log.debug( "==== Schedule Matrix for %s-%s ====", deviceName.c_str(), instanceName.c_str() );
    log.debug( "  timezone: %s", timezone.c_str() );
    log.debug( "  === Schedule Array ===" );

    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
       dayArr[i].debugPrint( 4, log );
    }
}


