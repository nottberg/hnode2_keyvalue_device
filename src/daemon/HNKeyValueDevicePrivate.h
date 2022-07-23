#ifndef __HN_TEST_DEVICE_PRIVATE_H__
#define __HN_TEST_DEVICE_PRIVATE_H__

#include <string>
#include <vector>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"

#include <hnode2/HNodeDevice.h>
#include <hnode2/HNodeConfig.h>
#include <hnode2/HNEPLoop.h>
#include <hnode2/HNReqWaitQueue.h>

#define HNODE_TEST_DEVTYPE   "hnode2-keyvalue-device"

typedef enum HNKeyValueDeviceResultEnum
{
  HNKVD_RESULT_SUCCESS,
  HNKVD_RESULT_FAILURE,
  HNKVD_RESULT_BAD_REQUEST,
  HNKVD_RESULT_SERVER_ERROR
}HNKVD_RESULT_T;

class HNKeyValueDevice : public Poco::Util::ServerApplication, public HNDEPDispatchInf //, public HNEPLoopCallbacks
{
    private:
        bool _helpRequested   = false;
        bool _debugLogging    = false;
        bool _instancePresent = false;

        std::string _instance; 
        std::string m_instanceName;

        HNodeDevice m_hnodeDev;

        void displayHelp();

        bool configExists();
        HNKVD_RESULT_T initConfig();
        HNKVD_RESULT_T readConfig();
        HNKVD_RESULT_T updateConfig();

    protected:
        // HNDevice REST callback
        virtual void dispatchEP( HNodeDevice *parent, HNOperationData *opData );

        // Poco funcions
        void defineOptions( Poco::Util::OptionSet& options );
        void handleOption( const std::string& name, const std::string& value );
        int main( const std::vector<std::string>& args );

};

#endif // __HN_KEYVALUE_DEVICE_PRIVATE_H__
