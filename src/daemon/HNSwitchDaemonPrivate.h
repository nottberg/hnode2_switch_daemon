#ifndef __HN_SWITCH_DAEMON_H__
#define __HN_SWITCH_DAEMON_H__

#include <sys/epoll.h>

#include <string>
#include <vector>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"

#include "HNDaemonLog.h"
#include "HNDaemonHealth.h"
#include "HNSwitchManager.h"
#include "HNScheduleMatrix.h"

class ClientRecord
{
    private:
        int clientFD;

    public:
        ClientRecord();
       ~ClientRecord();

        void setFD( int value );
        int getFD();
};

typedef enum HNSwitchDaemonResultEnum
{
  HNSD_RESULT_SUCCESS,
  HNSD_RESULT_FAILURE
}HNSD_RESULT_T;

class HNSwitchDaemon : public Poco::Util::ServerApplication, public HNSwitchManagerNotifications
{
    private:
        HNDaemonLog log;

        bool _helpRequested   = false;
        bool _debugLogging    = false;
        bool _instancePresent = false;

        std::string _instance; 

        bool quit;

        int epollFD;
        //int signalFD; 
        int acceptFD;

        std::string instanceName;
       
        struct epoll_event event;
        struct epoll_event *events;

        std::map< int, ClientRecord > clientMap;

        HNScheduleMatrix schMat;
        HNSequenceQueue  seqQueue;
        HNSwitchManager  switchMgr;

        uint   m_sendStatusFD;

        HNDaemonHealth overallHealth;

        void updateOverallHealth();

        HNSD_RESULT_T createStatusEventFD();

        HNSD_RESULT_T addSocketToEPoll( int sfd );
        HNSD_RESULT_T removeSocketFromEPoll( int sfd );        
      
        HNSD_RESULT_T processNewClientConnections();
        HNSD_RESULT_T processClientRequest( int efd );
        HNSD_RESULT_T closeClientConnection( int cfd );

        void sendStatusPacket( struct tm *curTS, std::vector< std::string > &swOnList );

        void sendComponentHealthPacket( int clientFD );
        void sendSwitchInfoPacket( int clientFD );

        uint32_t logSwitchChanges( struct tm *time, std::vector< std::string > &onList, uint32_t lastCRC );

        void displayHelp();

    protected:
        void defineOptions( Poco::Util::OptionSet& options );
        void handleOption( const std::string& name, const std::string& value );
        int main( const std::vector<std::string>& args );

        //HNSD_RESULT_T init();

        //HNSD_RESULT_T addSignalSocket( int sfd );

        HNSD_RESULT_T openListenerSocket( std::string deviceName, std::string instanceName );

        //HNSD_RESULT_T run();

        void triggerStatusSend();

        //virtual void notifyNewMeasurement( uint32_t sensorIndex, HNodeSensorMeasurement &reading );
        virtual void signalError( std::string errMsg );
        virtual void signalRunning();
};

#endif // __HN_SWITCH_DAEMON_H__
