////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016-2016 ArangoDB GmbH, Cologne, Germany
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
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#include "WorkMonitorHandler.h"

#include "Basics/StringUtils.h"
#include "GeneralServer/RestHandler.h"
#include "Rest/HttpRequest.h"
#include "velocypack/Builder.h"
#include "velocypack/velocypack-aliases.h"

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::rest;

using arangodb::HttpRequest;
using arangodb::rest::RestHandler;

WorkMonitorHandler::WorkMonitorHandler(GeneralRequest* request,
                                       GeneralResponse* response)
    : RestBaseHandler(request, response) {}

bool WorkMonitorHandler::isDirect() const { return true; }

RestStatus WorkMonitorHandler::execute() {
  auto const& suffixes = _request->suffixes();
  size_t const len = suffixes.size();
  auto const type = _request->requestType();

  if (type == rest::RequestType::GET) {
    if (len != 0) {
      generateError(rest::ResponseCode::BAD,
                    TRI_ERROR_HTTP_BAD_PARAMETER,
                    "expecting GET /_admin/work-monitor");
      return RestStatus::DONE;
    }

    std::shared_ptr<RestHandler> self = shared_from_this();

    return RestStatus::WAIT_FOR([self](std::function<void()> next) {
        WorkMonitor::requestWorkOverview(self, next);
      }).done();
  }

  if (type == rest::RequestType::DELETE_REQ) {
    if (len != 1) {
      generateError(rest::ResponseCode::BAD,
                    TRI_ERROR_HTTP_BAD_PARAMETER,
                    "expecting DELETE /_admin/work-monitor/<id>");

      return RestStatus::DONE;
    }

    uint64_t id = StringUtils::uint64(suffixes[0]);
    WorkMonitor::cancelWork(id);

    VPackBuilder b;
    b.add(VPackValue(VPackValueType::Object));
    b.add("canceled", VPackValue(true));
    b.close();

    VPackSlice s(b.start());

    generateResult(rest::ResponseCode::OK, s);
    return RestStatus::DONE;
  }

  generateError(rest::ResponseCode::BAD,
                TRI_ERROR_HTTP_METHOD_NOT_ALLOWED, "expecting GET or DELETE");
  return RestStatus::DONE;
}
