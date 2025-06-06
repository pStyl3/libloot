/*  LOOT

    A load order optimisation tool for Oblivion, Skyrim, Fallout 3 and
    Fallout: New Vegas.

    Copyright (C) 2013-2016    WrinklyNinja

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

#include "api/api_database.h"

#include <unordered_map>
#include <vector>

#include "api/game/game.h"
#include "api/metadata/condition_evaluator.h"
#include "api/metadata/yaml/plugin_metadata.h"
#include "api/sorting/group_sort.h"
#include "api/sorting/plugin_sort.h"
#include "loot/exception/file_access_error.h"
#include "loot/metadata/group.h"

namespace {
using loot::Group;

std::vector<Group> MergeGroups(const std::vector<Group>& masterlistGroups,
                               const std::vector<Group>& userGroups) {
  auto mergedGroups = masterlistGroups;

  std::vector<Group> newGroups;
  for (const auto& userGroup : userGroups) {
    auto groupIt =
        std::find_if(mergedGroups.begin(),
                     mergedGroups.end(),
                     [&](const Group& existingGroup) {
                       return existingGroup.GetName() == userGroup.GetName();
                     });

    if (groupIt == mergedGroups.end()) {
      newGroups.push_back(userGroup);
    } else {
      // Replace the masterlist group description with the userlist group
      // description if the latter is not empty.
      auto description = userGroup.GetDescription().empty()
                             ? groupIt->GetDescription()
                             : userGroup.GetDescription();

      auto afterGroups = groupIt->GetAfterGroups();
      auto userAfterGroups = userGroup.GetAfterGroups();
      afterGroups.insert(
          afterGroups.end(), userAfterGroups.begin(), userAfterGroups.end());

      *groupIt = Group(userGroup.GetName(), afterGroups, description);
    }
  }

  mergedGroups.insert(mergedGroups.end(), newGroups.cbegin(), newGroups.cend());

  return mergedGroups;
}
}

namespace loot {
ApiDatabase::ApiDatabase(
    std::shared_ptr<ConditionEvaluator> conditionEvaluator) :
    conditionEvaluator_(conditionEvaluator) {}

///////////////////////////////////
// Database Loading Functions
///////////////////////////////////

void ApiDatabase::LoadMasterlist(const std::filesystem::path& masterlistPath) {
  MetadataList temp;

  if (std::filesystem::exists(masterlistPath)) {
    temp.Load(masterlistPath);
  } else {
    throw FileAccessError("The given masterlist path does not exist: " +
                          masterlistPath.u8string());
  }

  masterlist_ = temp;
}

void ApiDatabase::LoadMasterlistWithPrelude(
    const std::filesystem::path& masterlistPath,
    const std::filesystem::path& masterlistPreludePath) {
  MetadataList temp;

  if (std::filesystem::exists(masterlistPath)) {
    if (std::filesystem::exists(masterlistPreludePath)) {
      temp.LoadWithPrelude(masterlistPath, masterlistPreludePath);
    } else {
      throw FileAccessError(
          "The given masterlist prelude path does not exist: " +
          masterlistPreludePath.u8string());
    }
  } else {
    throw FileAccessError("The given masterlist path does not exist: " +
                          masterlistPath.u8string());
  }

  masterlist_ = temp;
}

void ApiDatabase::LoadUserlist(const std::filesystem::path& userlistPath) {
  MetadataList temp;

  if (std::filesystem::exists(userlistPath)) {
    temp.Load(userlistPath);
  } else {
    throw FileAccessError("The given userlist path does not exist: " +
                          userlistPath.u8string());
  }

  userlist_ = temp;
}

void ApiDatabase::WriteUserMetadata(const std::filesystem::path& outputFile,
                                    const bool overwrite) const {
  if (!std::filesystem::exists(outputFile.parent_path()))
    throw std::invalid_argument("Output directory does not exist.");

  if (std::filesystem::exists(outputFile) && !overwrite)
    throw FileAccessError(
        "Output file exists but overwrite is not set to true.");

  userlist_.Save(outputFile);
}

bool ApiDatabase::Evaluate(const std::string& condition) const {
  return conditionEvaluator_->Evaluate(condition);
}

//////////////////////////
// DB Access Functions
//////////////////////////

std::vector<std::string> ApiDatabase::GetKnownBashTags() const {
  auto masterlistTags = masterlist_.BashTags();
  auto userlistTags = userlist_.BashTags();

  if (!userlistTags.empty()) {
    masterlistTags.insert(std::end(masterlistTags),
                          std::begin(userlistTags),
                          std::end(userlistTags));
  }

  return masterlistTags;
}

std::vector<Message> ApiDatabase::GetGeneralMessages(
    bool evaluateConditions) const {
  auto masterlistMessages = masterlist_.Messages();
  auto userlistMessages = userlist_.Messages();

  if (!userlistMessages.empty()) {
    masterlistMessages.insert(std::end(masterlistMessages),
                              std::begin(userlistMessages),
                              std::end(userlistMessages));
  }

  if (evaluateConditions) {
    // Evaluate conditions from scratch.
    conditionEvaluator_->ClearConditionCache();
    for (auto it = std::begin(masterlistMessages);
         it != std::end(masterlistMessages);) {
      if (!conditionEvaluator_->Evaluate(it->GetCondition()))
        it = masterlistMessages.erase(it);
      else
        ++it;
    }
  }

  return masterlistMessages;
}

std::vector<Group> ApiDatabase::GetGroups(bool includeUserMetadata) const {
  if (includeUserMetadata) {
    return MergeGroups(masterlist_.Groups(), userlist_.Groups());
  }

  return masterlist_.Groups();
}

std::vector<Group> ApiDatabase::GetUserGroups() const {
  return userlist_.Groups();
}

void ApiDatabase::SetUserGroups(const std::vector<Group>& groups) {
  userlist_.SetGroups(groups);
}

std::vector<Vertex> ApiDatabase::GetGroupsPath(
    std::string_view fromGroupName,
    std::string_view toGroupName) const {
  auto masterlistGroups = GetGroups(false);
  auto userGroups = GetUserGroups();

  const auto groupGraph = BuildGroupGraph(masterlistGroups, userGroups);

  return loot::GetGroupsPath(groupGraph, fromGroupName, toGroupName);
}

std::optional<PluginMetadata> ApiDatabase::GetPluginMetadata(
    std::string_view plugin,
    bool includeUserMetadata,
    bool evaluateConditions) const {
  auto metadata = masterlist_.FindPlugin(plugin);

  if (includeUserMetadata) {
    auto userMetadata = userlist_.FindPlugin(plugin);
    if (userMetadata.has_value()) {
      if (metadata.has_value()) {
        userMetadata.value().MergeMetadata(metadata.value());
      }
      metadata = userMetadata;
    }
  }

  if (evaluateConditions && metadata.has_value()) {
    return conditionEvaluator_->EvaluateAll(metadata.value());
  }

  return metadata;
}

std::optional<PluginMetadata> ApiDatabase::GetPluginUserMetadata(
    std::string_view plugin,
    bool evaluateConditions) const {
  auto metadata = userlist_.FindPlugin(plugin);

  if (evaluateConditions && metadata) {
    return conditionEvaluator_->EvaluateAll(metadata.value());
  }

  return metadata;
}

void ApiDatabase::SetPluginUserMetadata(const PluginMetadata& pluginMetadata) {
  userlist_.ErasePlugin(pluginMetadata.GetName());
  userlist_.AddPlugin(pluginMetadata);
}

void ApiDatabase::DiscardPluginUserMetadata(std::string_view plugin) {
  userlist_.ErasePlugin(plugin);
}

void ApiDatabase::DiscardAllUserMetadata() { userlist_.Clear(); }

// Writes a minimal masterlist that only contains mods that have Bash Tag
// suggestions, and/or dirty messages, plus the Tag suggestions and/or messages
// themselves and their conditions, in order to create the Wrye Bash taglist.
// outputFile is the path to use for output. If outputFile already exists, it
// will only be overwritten if overwrite is true.
void ApiDatabase::WriteMinimalList(const std::filesystem::path& outputFile,
                                   const bool overwrite) const {
  if (!std::filesystem::exists(outputFile.parent_path()))
    throw std::invalid_argument("Output directory does not exist.");

  if (std::filesystem::exists(outputFile) && !overwrite)
    throw FileAccessError(
        "Output file exists but overwrite is not set to true.");

  MetadataList minimalList;
  for (const auto& plugin : masterlist_.Plugins()) {
    PluginMetadata minimalPlugin(plugin.GetName());
    minimalPlugin.SetTags(plugin.GetTags());
    minimalPlugin.SetDirtyInfo(plugin.GetDirtyInfo());

    minimalList.AddPlugin(minimalPlugin);
  }

  minimalList.Save(outputFile);
}
}
