////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2017-2018 ArangoDB GmbH, Cologne, Germany
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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#include "Aql/Ast.h"
#include "Aql/AstNode.h"
#include "Aql/Function.h"
#include "Aql/Variable.h"
#include "Geo/GeoJsonParser.h"
#include "Geo/GeoUtils.h"
#include "Geo/GeoParams.h"
#include "Geo/Index.h"
#include "Geo/Shapes.h"

#include <geometry/s2regioncoverer.h>
#include <string>
#include <vector>
#include <velocypack/Slice.h>

using namespace arangodb;
using namespace arangodb::geo;

geo::Index::Index(VPackSlice const& info) : _variant(geo::Index::Variant::NONE) {
  _coverParams.fromVelocyPack(info);
}

void geo::Index::initalize(VPackSlice const& info,
                           std::vector<std::vector<basics::AttributeName>> const& fields) {
  if (fields.size() == 1) {
    bool geoJson =
    basics::VelocyPackHelper::getBooleanValue(info, "geoJson", false);
    // geojson means [<longitude>, <latitude>] or
    // json object {type:"<name>, coordinates:[]}.
    _variant = geoJson ? geo::Index::Variant::COMBINED_GEOJSON
    : geo::Index::Variant::COMBINED_LAT_LON;
    
    auto& loc = fields[0];
    _location.reserve(loc.size());
    for (auto const& it : loc) {
      _location.emplace_back(it.name);
    }
  } else if (fields.size() == 2) {
    _variant = geo::Index::Variant::INDIVIDUAL_LAT_LON;
    auto& lat = fields[0];
    _latitude.reserve(lat.size());
    for (auto const& it : lat) {
      _latitude.emplace_back(it.name);
    }
    auto& lon = fields[1];
    _longitude.reserve(lon.size());
    for (auto const& it : lon) {
      _longitude.emplace_back(it.name);
    }
  } else {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "s2index can only be created with one or two fields.");
  }
}

Result geo::Index::indexCells(VPackSlice const& doc,
                              std::vector<S2CellId>& cells,
                              geo::Coordinate& centroid) const {
  if (_variant == geo::Index::Variant::COMBINED_GEOJSON) {
    VPackSlice loc = doc.get(_location);
    if (loc.isArray()) {
      return GeoUtils::indexCellsLatLng(loc, /*geojson*/true, cells, centroid);
    }
    S2RegionCoverer coverer;
    _coverParams.configureS2RegionCoverer(&coverer);
    return geo::GeoUtils::indexCellsGeoJson(&coverer, loc, cells, centroid);
  } else if (_variant == geo::Index::Variant::COMBINED_LAT_LON) {
    VPackSlice loc = doc.get(_location);
    return GeoUtils::indexCellsLatLng(loc, /*geojson*/false, cells, centroid);
  } else if (_variant == geo::Index::Variant::INDIVIDUAL_LAT_LON) {
    VPackSlice lat = doc.get(_latitude);
    VPackSlice lon = doc.get(_longitude);
    if (!lat.isNumber() || !lon.isNumber()) {
      return TRI_ERROR_BAD_PARAMETER;
    }
    centroid.latitude = lat.getNumericValue<double>();
    centroid.longitude = lon.getNumericValue<double>();
    return geo::GeoUtils::indexCells(centroid, cells);
  }
  return TRI_ERROR_INTERNAL;
}

Result geo::Index::shape(velocypack::Slice const& doc,
                         geo::ShapeContainer& shape) const {
  if (_variant == geo::Index::Variant::COMBINED_GEOJSON) {
    VPackSlice loc = doc.get(_location);
    if (loc.isArray() && loc.length() >= 2) {
      return shape.parseCoordinates(loc, /*geoJson*/true);
    } else if (loc.isObject()) {
      return geo::GeoJsonParser::parseGeoJson(loc, shape);
    }
    return TRI_ERROR_BAD_PARAMETER;
  } else if (_variant == geo::Index::Variant::COMBINED_LAT_LON) {
    VPackSlice loc = doc.get(_location);
    return shape.parseCoordinates(loc, /*geoJson*/false);
  } else if (_variant == geo::Index::Variant::INDIVIDUAL_LAT_LON) {
    VPackSlice lon = doc.get(_longitude);
    VPackSlice lat = doc.get(_latitude);
    if (!lon.isNumber() || !lat.isNumber()) {
      return TRI_ERROR_BAD_PARAMETER;
    }
    shape.resetCoordinates(lat.getNumericValue<double>(),
                           lon.getNumericValue<double>());
    return TRI_ERROR_NO_ERROR;
  }
  return TRI_ERROR_INTERNAL;
}

// Handle GEO_DISTANCE(<something>, doc.field)
geo::Coordinate geo::Index::parseGeoDistance(aql::AstNode const* args,
                                             aql::Variable const* ref) {
  // aql::AstNode* dist = node->getMemberUnchecked(0);
  TRI_ASSERT(args->numMembers() == 2);
  if (args->numMembers() != 2) {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_QUERY_FUNCTION_ARGUMENT_NUMBER_MISMATCH);
  }
  // either doc.geo or [doc.lng, doc.lat]
  aql::AstNode const* var = args->getMember(1);
  TRI_ASSERT(var->isAttributeAccessForVariable(ref, true) ||
             var->isArray() &&
                 var->getMember(0)->isAttributeAccessForVariable(ref, true) &&
                 var->getMember(1)->isAttributeAccessForVariable(ref, true));
  aql::AstNode* cc = args->getMemberUnchecked(0);
  TRI_ASSERT(cc->type != aql::NODE_TYPE_ATTRIBUTE_ACCESS);
  if (cc->type == aql::NODE_TYPE_ATTRIBUTE_ACCESS) {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_QUERY_FUNCTION_ARGUMENT_TYPE_MISMATCH);
  }

  Result res;
  if (cc->type == aql::NODE_TYPE_ARRAY) {  // [lng, lat] is valid input
    TRI_ASSERT(cc->numMembers() == 2);
    return geo::Coordinate(/*lat*/ cc->getMember(1)->getDoubleValue(),
                           /*lon*/ cc->getMember(0)->getDoubleValue());
  } else {
    Result res;
    VPackBuilder jsonB;
    cc->toVelocyPackValue(jsonB);
    VPackSlice json = jsonB.slice();
    geo::ShapeContainer shape;
    if (json.isArray() && json.length() >= 2) {
      res = shape.parseCoordinates(json, /*GeoJson*/ true);
    } else {
      res = geo::GeoJsonParser::parseGeoJson(json, shape);
    }
    if (res.fail()) {
      THROW_ARANGO_EXCEPTION(res);
    }
    return shape.centroid();
  }
}

// either GEO_DISTANCE or DISTANCE
geo::Coordinate geo::Index::parseDistFCall(aql::AstNode const* node,
                                         aql::Variable const* ref) {
  TRI_ASSERT(node->type == aql::NODE_TYPE_FCALL);
  aql::AstNode* args = node->getMemberUnchecked(0);
  aql::Function* func = static_cast<aql::Function*>(node->getData());
  TRI_ASSERT(func != nullptr);
  if (func->name == "GEO_DISTANCE") {
    return geo::Index::parseGeoDistance(args, ref);
  }
  TRI_ASSERT(false);
  THROW_ARANGO_EXCEPTION(TRI_ERROR_QUERY_FUNCTION_ARGUMENT_NUMBER_MISMATCH);
}

void geo::Index::handleNode(aql::AstNode const* node, aql::Variable const* ref,
                          geo::QueryParams& qp) {
  switch (node->type) {
    // Handle GEO_CONTAINS(<geoJson-object>, doc.field)
    // or GEO_INTERSECTS(<geoJson-object>, doc.field)
    case aql::NODE_TYPE_FCALL: {
      // TODO handle GEO_CONTAINS / INTERSECT
      aql::AstNode const* args = node->getMemberUnchecked(0);
      TRI_ASSERT(args->numMembers() == 2);
      if (args->numMembers() != 2) {
        THROW_ARANGO_EXCEPTION(
            TRI_ERROR_QUERY_FUNCTION_ARGUMENT_NUMBER_MISMATCH);
      }

      aql::AstNode const* geoJson = args->getMemberUnchecked(0);
      aql::AstNode const* symbol = args->getMemberUnchecked(1);
      TRI_ASSERT(symbol->isArray() && symbol->numMembers() == 2 &&
                 symbol->getMember(0)->isAttributeAccessForVariable() &&
                 symbol->getMember(1)->isAttributeAccessForVariable()
                 || symbol->isAttributeAccessForVariable(ref, true));
      TRI_ASSERT(!geoJson->isAttributeAccessForVariable());

      // arrays can't occur only handle real GeoJSON
      VPackBuilder bb;
      geoJson->toVelocyPackValue(bb);
      Result res = geo::GeoJsonParser::parseGeoJson(bb.slice(), qp.filterShape);
      if (res.fail()) {
        THROW_ARANGO_EXCEPTION(res);
      }

      aql::Function* func = static_cast<aql::Function*>(node->getData());
      TRI_ASSERT(func != nullptr);
      if (func->name == "GEO_CONTAINS") {
        qp.filterType = geo::FilterType::CONTAINS;
      } else if (func->name == "GEO_INTERSECTS") {
        qp.filterType = geo::FilterType::INTERSECTS;
      } else {
        TRI_ASSERT(false);
      }
      break;
    }
    // Handle GEO_DISTANCE(<something>, doc.field) [<|<=|=>|>] <constant>
    case aql::NODE_TYPE_OPERATOR_BINARY_LE:
      qp.maxInclusive = true;
    case aql::NODE_TYPE_OPERATOR_BINARY_LT: {
      TRI_ASSERT(node->numMembers() == 2);
      geo::Coordinate c =
          geo::Index::parseDistFCall(node->getMemberUnchecked(0), ref);
      if (qp.origin.isValid() && qp.origin != c) {
        THROW_ARANGO_EXCEPTION(TRI_ERROR_INTERNAL);
      }
      // LOG_TOPIC(ERR, Logger::FIXME) << "Found center: " << c.toString();

      qp.origin = std::move(c);
      aql::AstNode const* max = node->getMemberUnchecked(1);
      TRI_ASSERT(max->type == aql::NODE_TYPE_VALUE);
      if (!max->isValueType(aql::VALUE_TYPE_STRING)) {
        qp.maxDistance = max->getDoubleValue();
      }  // else assert(max->getStringValue() == "unlimited")
      break;
    }
    case aql::NODE_TYPE_OPERATOR_BINARY_GE:
      qp.minInclusive = true;
    case aql::NODE_TYPE_OPERATOR_BINARY_GT: {
      TRI_ASSERT(node->numMembers() == 2);
      geo::Coordinate c =
          geo::Index::parseDistFCall(node->getMemberUnchecked(0), ref);
      if (qp.origin.isValid() && qp.origin != c) {
        THROW_ARANGO_EXCEPTION(TRI_ERROR_INTERNAL);
      }
      // LOG_TOPIC(ERR, Logger::FIXME) << "Found center: " << c.toString();

      aql::AstNode const* min = node->getMemberUnchecked(1);
      TRI_ASSERT(min->type == aql::NODE_TYPE_VALUE);
      qp.origin = c;
      qp.minDistance = min->getDoubleValue();
      break;
    }
    default:
      break;
  }
}

void geo::Index::parseCondition(aql::AstNode const* node,
                              aql::Variable const* reference,
                              geo::QueryParams& params) {
  if (aql::Ast::IsAndOperatorType(node->type)) {
    for (size_t i = 0; i < node->numMembers(); i++) {
      handleNode(node->getMemberUnchecked(i), reference, params);
    }
  } else {
    handleNode(node, reference, params);
  }
}
