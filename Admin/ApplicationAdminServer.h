////////////////////////////////////////////////////////////////////////////////
/// @brief application simple user and session management feature
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2012 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2011-2012, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_ADMIN_APPLICATION_ADMIN_SERVER_H
#define TRIAGENS_ADMIN_APPLICATION_ADMIN_SERVER_H 1

#include "ApplicationServer/ApplicationFeature.h"

#include "Rest/AddressPort.h"

namespace triagens {
  namespace rest {
    class ApplicationServer;
    class HttpHandlerFactory;
    class HttpResponse;
    class HttpRequest;
  }

  namespace admin {

// -----------------------------------------------------------------------------
// --SECTION--                                      class ApplicationAdminServer
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup RestServer
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief application simple user and session management feature
////////////////////////////////////////////////////////////////////////////////

    class ApplicationAdminServer : public rest::ApplicationFeature {
      ApplicationAdminServer (ApplicationAdminServer const&);
      ApplicationAdminServer& operator= (ApplicationAdminServer const&);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                             static public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup RestServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new feature
////////////////////////////////////////////////////////////////////////////////

        static ApplicationAdminServer* create (rest::ApplicationServer*);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup RestServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

        ~ApplicationAdminServer ();

      protected:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

        ApplicationAdminServer ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup RestServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief add a log viewer
////////////////////////////////////////////////////////////////////////////////

        void allowLogViewer ();

////////////////////////////////////////////////////////////////////////////////
/// @brief allows for a webadmin directory
////////////////////////////////////////////////////////////////////////////////

        void allowAdminDirectory ();

////////////////////////////////////////////////////////////////////////////////
/// @brief allows for a webadmin directory
////////////////////////////////////////////////////////////////////////////////

        void allowAdminDirectory (string const& adminDirectory);

////////////////////////////////////////////////////////////////////////////////
/// @brief allows for a front-end configuration
////////////////////////////////////////////////////////////////////////////////

        void allowFeConfiguration ();

////////////////////////////////////////////////////////////////////////////////
/// @brief allows for a version handler using the default version
////////////////////////////////////////////////////////////////////////////////

        void allowVersion ();

////////////////////////////////////////////////////////////////////////////////
/// @brief allows for a version handler
////////////////////////////////////////////////////////////////////////////////

        void allowVersion (string name, string version);

////////////////////////////////////////////////////////////////////////////////
/// @brief adds the http handlers
///
/// Note that the server does not claim ownership of the factory.
////////////////////////////////////////////////////////////////////////////////

        void addBasicHandlers (rest::HttpHandlerFactory*);

////////////////////////////////////////////////////////////////////////////////
/// @brief adds the http handlers for administration
///
/// Note that the server does not claim ownership of the factory.
////////////////////////////////////////////////////////////////////////////////

        void addHandlers (rest::HttpHandlerFactory*, string const& prefix);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                        ApplicationFeature methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup RestServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        void setupOptions (map<string, basics::ProgramOptionsDescription>&);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup RestServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief allow log viewer requests
////////////////////////////////////////////////////////////////////////////////

        bool _allowLogViewer;

////////////////////////////////////////////////////////////////////////////////
/// @brief allow log admin directory
////////////////////////////////////////////////////////////////////////////////

        bool _allowAdminDirectory;

////////////////////////////////////////////////////////////////////////////////
/// @brief allow front-end configuration
////////////////////////////////////////////////////////////////////////////////

        bool _allowFeConfiguration;

////////////////////////////////////////////////////////////////////////////////
/// @brief allow version request
////////////////////////////////////////////////////////////////////////////////

        bool _allowVersion;

////////////////////////////////////////////////////////////////////////////////
/// @brief the directory containing the admin files
////////////////////////////////////////////////////////////////////////////////

        string _adminDirectory;

////////////////////////////////////////////////////////////////////////////////
/// @brief path options for the admin directory
////////////////////////////////////////////////////////////////////////////////

        void* _pathOptions;

////////////////////////////////////////////////////////////////////////////////
/// @brief filename for the front-end configuration file
////////////////////////////////////////////////////////////////////////////////

        string _feConfiguration;

////////////////////////////////////////////////////////////////////////////////
/// @brief version name
////////////////////////////////////////////////////////////////////////////////

        string _name;

////////////////////////////////////////////////////////////////////////////////
/// @brief version number
////////////////////////////////////////////////////////////////////////////////

        string _version;
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

#endif

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:
