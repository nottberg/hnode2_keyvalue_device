#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <iostream>
#include <sstream>
#include <thread>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Checksum.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include <hnode2/HNodeDevice.h>

#include "HNKeyValueDevicePrivate.h"

using namespace Poco::Util;

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

// Forward declaration
extern const std::string g_HNode2TestRest;

void 
HNKeyValueDevice::defineOptions( OptionSet& options )
{
    ServerApplication::defineOptions( options );

    options.addOption(
              Option("help", "h", "display help").required(false).repeatable(false));

    options.addOption(
              Option("debug","d", "Enable debug logging").required(false).repeatable(false));

    options.addOption(
              Option("instance", "", "Specify the instance name of this daemon.").required(false).repeatable(false).argument("name"));

}

void 
HNKeyValueDevice::handleOption( const std::string& name, const std::string& value )
{
    ServerApplication::handleOption( name, value );
    if( "help" == name )
        _helpRequested = true;
    else if( "debug" == name )
        _debugLogging = true;
    else if( "instance" == name )
    {
         _instancePresent = true;
         _instance = value;
    }
}

void 
HNKeyValueDevice::displayHelp()
{
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("[options]");
    helpFormatter.setHeader("HNode2 Switch Daemon.");
    helpFormatter.format(std::cout);
}

int 
HNKeyValueDevice::main( const std::vector<std::string>& args )
{
    m_instanceName = "default";
    if( _instancePresent == true )
        m_instanceName = _instance;

    m_hnodeDev.setDeviceType( HNODE_TEST_DEVTYPE );
    m_hnodeDev.setInstance( m_instanceName );

    HNDEndpoint hndEP;

    hndEP.setDispatch( "hnode2Test", this );
    hndEP.setOpenAPIJson( g_HNode2TestRest ); 

    m_hnodeDev.addEndpoint( hndEP );

    std::cout << "Looking for config file" << std::endl;
    
    if( configExists() == false )
    {
        initConfig();
    }

    readConfig();

    // Start up the hnode device
    m_hnodeDev.start();

    waitForTerminationRequest();

    return Application::EXIT_OK;
}

bool 
HNKeyValueDevice::configExists()
{
    HNodeConfigFile cfgFile;

    return cfgFile.configExists( HNODE_TEST_DEVTYPE, m_instanceName );
}

HNKVD_RESULT_T
HNKeyValueDevice::initConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    m_hnodeDev.initConfigSections( cfg );

    cfg.debugPrint(2);
    
    std::cout << "Saving config..." << std::endl;
    if( cfgFile.saveConfig( HNODE_TEST_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not save initial configuration." << std::endl;
        return HNKVD_RESULT_FAILURE;
    }

    return HNKVD_RESULT_SUCCESS;
}

HNKVD_RESULT_T
HNKeyValueDevice::readConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    if( configExists() == false )
        return HNKVD_RESULT_FAILURE;

    std::cout << "Loading config..." << std::endl;

    if( cfgFile.loadConfig( HNODE_TEST_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not load saved configuration." << std::endl;
        return HNKVD_RESULT_FAILURE;
    }
  
    std::cout << "cl1" << std::endl;
    m_hnodeDev.readConfigSections( cfg );

    std::cout << "Config loaded" << std::endl;

    return HNKVD_RESULT_SUCCESS;
}

HNKVD_RESULT_T
HNKeyValueDevice::updateConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    cfg.debugPrint(2);
    
    std::cout << "Saving config..." << std::endl;
    if( cfgFile.saveConfig( HNODE_TEST_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not save configuration." << std::endl;
        return HNKVD_RESULT_FAILURE;
    }
    std::cout << "Config saved" << std::endl;

    return HNKVD_RESULT_SUCCESS;
}

void 
HNKeyValueDevice::dispatchEP( HNodeDevice *parent, HNOperationData *opData )
{
    std::cout << "HNKeyValueDevice::dispatchEP() - entry" << std::endl;
    std::cout << "  dispatchID: " << opData->getDispatchID() << std::endl;
    std::cout << "  opID: " << opData->getOpID() << std::endl;
    std::cout << "  thread: " << std::this_thread::get_id() << std::endl;

    std::string opID = opData->getOpID();
          

    // GET "/hnode2/keyvalue/status"
    if( "getStatus" == opID )
    {
        std::cout << "=== Get Status Request ===" << std::endl;
    
        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json status object
        pjs::Object jsRoot;
        jsRoot.set( "overallStatus", "OK" );

        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }
    // GET "/hnode2/keyvalue/settings"
    else if( "getSettings" == opID )
    {
        std::cout << "=== Get Settings Request ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json root object
        pjs::Array jsRoot;
/*
        pjs::Object w1Obj;
        w1Obj.set( "id", "w1" );
        w1Obj.set( "color", "red" );
        jsRoot.add( w1Obj );

        pjs::Object w2Obj;
        w2Obj.set( "id", "w2" );
        w2Obj.set( "color", "green" );
        jsRoot.add( w2Obj );

        pjs::Object w3Obj;
        w3Obj.set( "id", "w3" );
        w3Obj.set( "color", "blue" );
        jsRoot.add( w3Obj );
*/          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }    
    // PUT "/hnode2/keyvalue/settings"
    else if( "setSettings" == opID )
    {
        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        std::cout << "=== Set Settings Data ===" << std::endl;
        std::cout << body << std::endl;

        // Object was updated
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }
    // GET "/hnode2/keyvalue/pairs"
    else if( "getPairsList" == opID )
    {
        std::cout << "=== Get Pairs List ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json root object
        pjs::Array jsRoot;
/*
        pjs::Object w1Obj;
        w1Obj.set( "id", "w1" );
        w1Obj.set( "color", "red" );
        jsRoot.add( w1Obj );

        pjs::Object w2Obj;
        w2Obj.set( "id", "w2" );
        w2Obj.set( "color", "green" );
        jsRoot.add( w2Obj );

        pjs::Object w3Obj;
        w3Obj.set( "id", "w3" );
        w3Obj.set( "color", "blue" );
        jsRoot.add( w3Obj );
*/          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }
    // GET "/hnode2/keyvalue/pairs/{key}"
    else if( "getPair" == opID )
    {
        std::string key;

        if( opData->getParam( "key", key ) == true )
        {
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Get Pair Request (id: " << key << ") ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );
        
        // Create a json root object
        pjs::Array jsRoot;
/*
        pjs::Object w1Obj;
        w1Obj.set( "id", widgetID );
        w1Obj.set( "color", "black" );
        jsRoot.add( w1Obj );
*/          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );

    }
    // POST "/hnode2/keyvalue/pairs"
    else if( "createPair" == opID )
    {
        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        std::cout << "=== Create New Pair Post Data ===" << std::endl;
        std::cout << body << std::endl;

        // Object was created return info
        opData->responseSetCreated( "w1" );
        opData->responseSetStatusAndReason( HNR_HTTP_CREATED );
    }
    // PUT "/hnode2/keyvalue/pairs/{key}"
    else if( "updatePair" == opID )
    {
        std::string key;

        // Make sure zoneid was provided
        if( opData->getParam( "key", key ) == true )
        {
            // widgetid parameter is required
            opData->responseSetStatusAndReason( HNR_HTTP_BAD_REQUEST );
            opData->responseSend();
            return; 
        }
        
        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        std::cout << "=== Update Pair Data (id: " << key << ") ===" << std::endl;
        std::cout << body << std::endl;

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }    
    // DELETE "/hnode2/keyvalue/pairs/{key}"
    else if( "deletePair" == opID )
    {
        std::string key;

        // Make sure key was provided
        if( opData->getParam( "key", key ) == true )
        {
            // widgetid parameter is required
            opData->responseSetStatusAndReason( HNR_HTTP_BAD_REQUEST );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Delete Pair Request (id: " << key << ") ===" << std::endl;

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }
    // GET "/hnode2/keyvalue/watchers"
    else if( "getWatchersList" == opID )
    {
        std::cout << "=== Get Watchers List ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json root object
        pjs::Array jsRoot;
/*
        pjs::Object w1Obj;
        w1Obj.set( "id", "w1" );
        w1Obj.set( "color", "red" );
        jsRoot.add( w1Obj );

        pjs::Object w2Obj;
        w2Obj.set( "id", "w2" );
        w2Obj.set( "color", "green" );
        jsRoot.add( w2Obj );

        pjs::Object w3Obj;
        w3Obj.set( "id", "w3" );
        w3Obj.set( "color", "blue" );
        jsRoot.add( w3Obj );
*/          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }
    // GET "/hnode2/keyvalue/watchers/{watchid}"
    else if( "getWatcher" == opID )
    {
        std::string watchid;

        if( opData->getParam( "watchid", watchid ) == true )
        {
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Get Watcher Request (id: " << watchid << ") ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );
        
        // Create a json root object
        pjs::Array jsRoot;
/*
        pjs::Object w1Obj;
        w1Obj.set( "id", widgetID );
        w1Obj.set( "color", "black" );
        jsRoot.add( w1Obj );
*/          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );

    }
    // POST "/hnode2/keyvalue/watchers"
    else if( "createWatchers" == opID )
    {
        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        std::cout << "=== Create New Watcher Post Data ===" << std::endl;
        std::cout << body << std::endl;

        // Object was created return info
        opData->responseSetCreated( "w1" );
        opData->responseSetStatusAndReason( HNR_HTTP_CREATED );
    }
    // PUT "/hnode2/keyvalue/watchers/{watchid}"
    else if( "updateWatchers" == opID )
    {
        std::string watchid;

        // Make sure zoneid was provided
        if( opData->getParam( "watchid", watchid ) == true )
        {
            // widgetid parameter is required
            opData->responseSetStatusAndReason( HNR_HTTP_BAD_REQUEST );
            opData->responseSend();
            return; 
        }
        
        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        std::cout << "=== Update Watchers Data (id: " << watchid << ") ===" << std::endl;
        std::cout << body << std::endl;

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }    
    // DELETE "/hnode2/keyvalue/watchers/{watchid}"
    else if( "deleteWatcher" == opID )
    {
        std::string watchid;

        // Make sure key was provided
        if( opData->getParam( "watchid", watchid ) == true )
        {
            // widgetid parameter is required
            opData->responseSetStatusAndReason( HNR_HTTP_BAD_REQUEST );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Delete Watcher Request (id: " << watchid << ") ===" << std::endl;

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
    }    
    else
    {
        // Send back not implemented
        opData->responseSetStatusAndReason( HNR_HTTP_NOT_IMPLEMENTED );
        opData->responseSend();
        return;
    }

    // Return to caller
    opData->responseSend();
}

const std::string g_HNode2TestRest = R"(
{
  "openapi": "3.0.0",
  "info": {
    "description": "",
    "version": "1.0.0",
    "title": ""
  },
  "paths": {
      "/hnode2/keyvalue/status": {
        "get": {
          "summary": "Get test device status.",
          "operationId": "getStatus",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "array"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/keyvalue/settings": {
        "get": {
          "summary": "Return key-value device settings.",
          "operationId": "getSettings",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },

        "put": {
          "summary": "Change key-value device settings.",
          "operationId": "setSettings",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/keyvalue/pairs": {
        "get": {
          "summary": "Return a list of key-value pairs.",
          "operationId": "getPairsList",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },

        "post": {
          "summary": "Create a new key value pair.",
          "operationId": "createPair",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/keyvalue/pairs/{key}": {
        "get": {
          "summary": "Get the pair associated with a specific key.",
          "operationId": "getPair",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },
        "put": {
          "summary": "Update the value for a spcific key.",
          "operationId": "updatePair",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },
        "delete": {
          "summary": "Delete the pair associated with a specific key",
          "operationId": "deletePair",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/keyvalue/watchers": {
        "get": {
          "summary": "Return a list of pair watchers",
          "operationId": "getWatchersList",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },

        "post": {
          "summary": "Create a pair watcher.",
          "operationId": "createWatcher",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/keyvalue/watchers/{watchid}": {
        "get": {
          "summary": "Get the watcher associated with watchid.",
          "operationId": "getWatcher",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },
        "put": {
          "summary": "Update the watcher associated with watchid.",
          "operationId": "updateWatcher",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },
        "delete": {
          "summary": "Delete the watcher associated with watchid",
          "operationId": "deleteWatcher",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      }      
    }
}
)";

