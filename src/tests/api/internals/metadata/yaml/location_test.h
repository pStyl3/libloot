/*  LOOT

A load order optimisation tool for Oblivion, Skyrim, Fallout 3 and
Fallout: New Vegas.

Copyright (C) 2014-2016    WrinklyNinja

This file is part of LOOT.

LOOT is free software: you can redistribute
it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

LOOT is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LOOT.  If not, see
<https://www.gnu.org/licenses/>.
*/

#ifndef LOOT_TESTS_API_INTERNALS_METADATA_YAML_LOCATION_TEST
#define LOOT_TESTS_API_INTERNALS_METADATA_YAML_LOCATION_TEST

#include <gtest/gtest.h>

#include "api/metadata/yaml/location.h"

namespace loot::test {
TEST(Location, emittingAsYamlShouldOutputAScalarIfTheNameStringIsEmpty) {
  Location location("http://www.example.com");
  YAML::Emitter emitter;
  emitter << location;

  EXPECT_EQ("'" + location.GetURL() + "'", emitter.c_str());
}

TEST(Location, emittingAsYamlShouldOutputAMapIfTheNameStringIsNotEmpty) {
  Location location("http://www.example.com", "example");
  YAML::Emitter emitter;
  emitter << location;

  EXPECT_EQ(
      "link: '" + location.GetURL() + "'\nname: '" + location.GetName() + "'",
      emitter.c_str());
}

TEST(Location, encodingAsYamlShouldStoreDataCorrectly) {
  Location location("http://www.example.com", "example");
  YAML::Node node;
  node = location;

  EXPECT_EQ(location.GetURL(), node["link"].as<std::string>());
  EXPECT_EQ(location.GetName(), node["name"].as<std::string>());
}

TEST(Location, encodingAsYamlShouldOmitEmptyFields) {
  Location location("http://www.example.com");
  YAML::Node node;
  node = location;

  EXPECT_EQ(location.GetURL(), node["link"].as<std::string>());
  EXPECT_FALSE(node["name"]);
}

TEST(Location, decodingFromYamlShouldSetDataCorrectly) {
  YAML::Node node = YAML::Load("{link: http://www.example.com, name: example}");
  Location location = node.as<Location>();

  EXPECT_EQ(node["link"].as<std::string>(), location.GetURL());
  EXPECT_EQ(node["name"].as<std::string>(), location.GetName());
}

TEST(Location,
     decodingFromYamlScalarShouldSetUrlToScalarValueAndLeaveNameEmpty) {
  YAML::Node node = YAML::Load("http://www.example.com");
  Location location = node.as<Location>();

  EXPECT_EQ(node.as<std::string>(), location.GetURL());
  EXPECT_TRUE(location.GetName().empty());
}

TEST(Location, decodingFromYamlShouldThrowIfAListIsGiven) {
  YAML::Node node = YAML::Load("[0, 1, 2]");

  EXPECT_THROW(node.as<Location>(), YAML::RepresentationException);
}
}

#endif
