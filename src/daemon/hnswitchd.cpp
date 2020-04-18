#include "HNSwitchDaemonPrivate.h"

int 
main( int argc, char* argv[] )
{
    HNSwitchDaemon swd;    
    return swd.run( argc, argv );
}

