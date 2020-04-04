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
HNSAction::debugPrint( uint offset )
{
    std::cout << std::setw( offset ) << " ";
    std::cout << "action: " << action;
    std::cout << "  start: " << start.getHMSStr();
    std::cout << "  end: " << end.getHMSStr();
    std::cout << "  switch: " << swid;
    std::cout << std::endl;
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
HNSDay::debugPrint( uint offset )
{
    std::cout << std::setw( offset ) << " " << "Day: " << dayName << std::endl;

    for( std::list< HNSAction >::iterator it = actionArr.begin(); it != actionArr.end(); it++ )
    {
        it->debugPrint( offset + 2 );
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
                    continue;

                setTimezone( it->second );
            }
            else if( it->first == "scheduleMatrix" )
            {
                if( jsRoot->isObject( it ) == false )
                    continue;

                pjs::Object::Ptr jsDayObj = jsRoot->getObject( it->first );

                for( pjs::Object::ConstIterator dit = jsDayObj->begin(); dit != jsDayObj->end(); dit++ )
                {

                    for( uint dindx = 0; dindx < HNS_DAY_CNT; dindx++ )
                    {
                        if( scDayNames[dindx] == dit->first )
                        {
                            if( jsDayObj->isArray( dit ) == false )
                                break;

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
                                            break;
                                        
                                        tmpAction.handlePair( aoit->first, aoit->second );
                                    }

                                    dayArr[ dindx ].addAction( tmpAction );
                                }
                                else
                                {
                                    // Unrecognized config element at array layer
                                }
                            }

                            break;
                        } 

                    }
                }

            }
        }
    }
    catch( Poco::Exception ex )
    {
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
    std::cout << "==== Schedule Matrix for " << deviceName << "-" << instanceName << " ====" << std::endl;

    std::cout << "  timezone: " << timezone << std::endl;

    std::cout << "  === Schedule Array ===" << std::endl;
    for( int i = 0; i < HNS_DAY_CNT; i++ )
    {
       dayArr[i].debugPrint( 4 );
    }
}


