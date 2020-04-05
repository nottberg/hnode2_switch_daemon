#ifndef __HN_SWITCH_DAEMON_H__
#define __HN_SWITCH_DAEMON_H__

#include <sys/epoll.h>

#include <string>
#include <vector>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"

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

#define HN_SWDAEMON_DEVICE_NAME  "switch-daemon"
#define HN_SWDAEMON_DEF_INSTANCE "default"

typedef enum DaemonProcessResultEnum
{
  DP_RESULT_SUCCESS,
  DP_RESULT_FAILURE
}DP_RESULT_T;

class HNSwitchDaemon : public Poco::Util::ServerApplication, public SwitchManagerNotifications
{
    private:
        bool helpRequested=false;

        bool quit;

        int epollFD;
        int signalFD; 
        int acceptFD;

        std::string instanceName;
       
        struct epoll_event event;
        struct epoll_event *events;

        std::map< int, ClientRecord > clientMap;

        SwitchManager  switchMgr;

        bool           sendStatus;

        bool           healthOK;
        std::string    curErrMsg;
        struct timeval lastReadingTS;

        HNScheduleMatrix schMat;

        DP_RESULT_T addSocketToEPoll( int sfd );
        DP_RESULT_T removeSocketFromEPoll( int sfd );        
      
        DP_RESULT_T processNewClientConnections();
        DP_RESULT_T processClientRequest( int efd );
        DP_RESULT_T closeClientConnection( int cfd );

        void sendStatusPacket( struct timeval *curTS );

        void displayHelp();

    protected:
        void defineOptions( Poco::Util::OptionSet& options );
        void handleOption( const std::string& name, const std::string& value );
        int main( const std::vector<std::string>& args );

        //DP_RESULT_T init();

        DP_RESULT_T addSignalSocket( int sfd );

        DP_RESULT_T openListenerSocket();

        //DP_RESULT_T run();

        //virtual void notifyNewMeasurement( uint32_t sensorIndex, HNodeSensorMeasurement &reading );
        virtual void signalError( std::string errMsg );
        virtual void signalRunning();
};

#endif // __HN_SWITCH_DAEMON_H__
