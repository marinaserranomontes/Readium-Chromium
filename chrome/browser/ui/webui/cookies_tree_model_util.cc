// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/cookies_tree_model_util.h"

#include "base/i18n/time_formatting.h"
#include "base/string_number_conversions.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "base/values.h"
#include "chrome/browser/cookies_tree_model.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"

namespace {

static const char kKeyId[] = "id";
static const char kKeyTitle[] = "title";
static const char kKeyIcon[] = "icon";
static const char kKeyType[] = "type";
static const char kKeyHasChildren[] = "hasChildren";

static const char kKeyName[] = "name";
static const char kKeyContent[] = "content";
static const char kKeyDomain[] = "domain";
static const char kKeyPath[] = "path";
static const char kKeySendFor[] = "sendfor";
static const char kKeyAccessibleToScript[] = "accessibleToScript";
static const char kKeyDesc[] = "desc";
static const char kKeySize[] = "size";
static const char kKeyOrigin[] = "origin";
static const char kKeyManifest[] = "manifest";

static const char kKeyAccessed[] = "accessed";
static const char kKeyCreated[] = "created";
static const char kKeyExpires[] = "expires";
static const char kKeyModified[] = "modified";

static const char kKeyPersistent[] = "persistent";
static const char kKeyTemporary[] = "temporary";

static const char kKeyTotalUsage[] = "totalUsage";
static const char kKeyTemporaryUsage[] = "temporaryUsage";
static const char kKeyPersistentUsage[] = "persistentUsage";
static const char kKeyPersistentQuota[] = "persistentQuota";

static const int64 kNegligibleUsage = 1024;  // 1KiB

// Encodes a pointer value into a hex string.
std::string PointerToHexString(const void* pointer) {
  return base::HexEncode(&pointer, sizeof(pointer));
}

// Decodes a pointer from a hex string.
void* HexStringToPointer(const std::string& str) {
  std::vector<uint8> buffer;
  if (!base::HexStringToBytes(str, &buffer) ||
      buffer.size() != sizeof(void*)) {
    return NULL;
  }

  return *reinterpret_cast<void**>(&buffer[0]);
}

}  // namespace

namespace cookies_tree_model_util {

std::string GetTreeNodeId(CookieTreeNode* node) {
  return PointerToHexString(node);
}

bool GetCookieTreeNodeDictionary(const CookieTreeNode& node,
                                 DictionaryValue* dict) {
  // Use node's address as an id for WebUI to look it up.
  dict->SetString(kKeyId, PointerToHexString(&node));
  dict->SetString(kKeyTitle, node.GetTitle());
  dict->SetBoolean(kKeyHasChildren, !node.empty());

  switch (node.GetDetailedInfo().node_type) {
    case CookieTreeNode::DetailedInfo::TYPE_ORIGIN: {
      dict->SetString(kKeyType, "origin");
#if defined(OS_MACOSX)
      dict->SetString(kKeyIcon, "chrome://theme/IDR_BOOKMARK_BAR_FOLDER");
#endif
      break;
    }
    case CookieTreeNode::DetailedInfo::TYPE_COOKIE: {
      dict->SetString(kKeyType, "cookie");
      dict->SetString(kKeyIcon, "chrome://theme/IDR_COOKIE_ICON");

      const net::CookieMonster::CanonicalCookie& cookie =
          *node.GetDetailedInfo().cookie;

      dict->SetString(kKeyName, cookie.Name());
      dict->SetString(kKeyContent, cookie.Value());
      dict->SetString(kKeyDomain, cookie.Domain());
      dict->SetString(kKeyPath, cookie.Path());
      dict->SetString(kKeySendFor, cookie.IsSecure() ?
          l10n_util::GetStringUTF8(IDS_COOKIES_COOKIE_SENDFOR_SECURE) :
          l10n_util::GetStringUTF8(IDS_COOKIES_COOKIE_SENDFOR_ANY));
      std::string accessible = cookie.IsHttpOnly() ?
          l10n_util::GetStringUTF8(IDS_COOKIES_COOKIE_ACCESSIBLE_TO_SCRIPT_NO) :
          l10n_util::GetStringUTF8(IDS_COOKIES_COOKIE_ACCESSIBLE_TO_SCRIPT_YES);
      dict->SetString(kKeyAccessibleToScript, accessible);
      dict->SetString(kKeyCreated, UTF16ToUTF8(
          base::TimeFormatFriendlyDateAndTime(cookie.CreationDate())));
      dict->SetString(kKeyExpires, cookie.DoesExpire() ? UTF16ToUTF8(
          base::TimeFormatFriendlyDateAndTime(cookie.ExpiryDate())) :
          l10n_util::GetStringUTF8(IDS_COOKIES_COOKIE_EXPIRES_SESSION));

      break;
    }
    case CookieTreeNode::DetailedInfo::TYPE_DATABASE: {
      dict->SetString(kKeyType, "database");
      dict->SetString(kKeyIcon, "chrome://theme/IDR_COOKIE_STORAGE_ICON");

      const BrowsingDataDatabaseHelper::DatabaseInfo& database_info =
          *node.GetDetailedInfo().database_info;

      dict->SetString(kKeyName, database_info.database_name.empty() ?
          l10n_util::GetStringUTF8(IDS_COOKIES_WEB_DATABASE_UNNAMED_NAME) :
          database_info.database_name);
      dict->SetString(kKeyDesc, database_info.description);
      dict->SetString(kKeySize, ui::FormatBytes(database_info.size));
      dict->SetString(kKeyModified, UTF16ToUTF8(
          base::TimeFormatFriendlyDateAndTime(database_info.last_modified)));

      break;
    }
    case CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGE: {
      dict->SetString(kKeyType, "local_storage");
      dict->SetString(kKeyIcon, "chrome://theme/IDR_COOKIE_STORAGE_ICON");

      const BrowsingDataLocalStorageHelper::LocalStorageInfo&
         local_storage_info = *node.GetDetailedInfo().local_storage_info;

      dict->SetString(kKeyOrigin, local_storage_info.origin);
      dict->SetString(kKeySize, ui::FormatBytes(local_storage_info.size));
      dict->SetString(kKeyModified, UTF16ToUTF8(
          base::TimeFormatFriendlyDateAndTime(
              local_storage_info.last_modified)));

      break;
    }
    case CookieTreeNode::DetailedInfo::TYPE_APPCACHE: {
      dict->SetString(kKeyType, "app_cache");
      dict->SetString(kKeyIcon, "chrome://theme/IDR_COOKIE_STORAGE_ICON");

      const appcache::AppCacheInfo& appcache_info =
          *node.GetDetailedInfo().appcache_info;

      dict->SetString(kKeyManifest, appcache_info.manifest_url.spec());
      dict->SetString(kKeySize, ui::FormatBytes(appcache_info.size));
      dict->SetString(kKeyCreated, UTF16ToUTF8(
          base::TimeFormatFriendlyDateAndTime(appcache_info.creation_time)));
      dict->SetString(kKeyAccessed, UTF16ToUTF8(
          base::TimeFormatFriendlyDateAndTime(appcache_info.last_access_time)));

      break;
    }
    case CookieTreeNode::DetailedInfo::TYPE_INDEXED_DB: {
      dict->SetString(kKeyType, "indexed_db");
      dict->SetString(kKeyIcon, "chrome://theme/IDR_COOKIE_STORAGE_ICON");

      const BrowsingDataIndexedDBHelper::IndexedDBInfo& indexed_db_info =
          *node.GetDetailedInfo().indexed_db_info;

      dict->SetString(kKeyOrigin, indexed_db_info.origin.spec());
      dict->SetString(kKeySize, ui::FormatBytes(indexed_db_info.size));
      dict->SetString(kKeyModified, UTF16ToUTF8(
          base::TimeFormatFriendlyDateAndTime(indexed_db_info.last_modified)));

      break;
    }
    case CookieTreeNode::DetailedInfo::TYPE_FILE_SYSTEM: {
      dict->SetString(kKeyType, "file_system");
      dict->SetString(kKeyIcon, "chrome://theme/IDR_COOKIE_STORAGE_ICON");

      const BrowsingDataFileSystemHelper::FileSystemInfo& file_system_info =
          *node.GetDetailedInfo().file_system_info;

      dict->SetString(kKeyOrigin, file_system_info.origin.spec());
      dict->SetString(kKeyPersistent,
                      file_system_info.has_persistent ?
                          UTF16ToUTF8(ui::FormatBytes(
                              file_system_info.usage_persistent)) :
                          l10n_util::GetStringUTF8(
                              IDS_COOKIES_FILE_SYSTEM_USAGE_NONE));
      dict->SetString(kKeyTemporary,
                      file_system_info.has_temporary ?
                          UTF16ToUTF8(ui::FormatBytes(
                              file_system_info.usage_temporary)) :
                          l10n_util::GetStringUTF8(
                              IDS_COOKIES_FILE_SYSTEM_USAGE_NONE));
      break;
    }
    case CookieTreeNode::DetailedInfo::TYPE_QUOTA: {
      dict->SetString(kKeyType, "quota");
      dict->SetString(kKeyIcon, "chrome://theme/IDR_COOKIE_STORAGE_ICON");

      const BrowsingDataQuotaHelper::QuotaInfo& quota_info =
          *node.GetDetailedInfo().quota_info;
      if (quota_info.temporary_usage + quota_info.persistent_usage <=
          kNegligibleUsage)
        return false;

      dict->SetString(kKeyOrigin, quota_info.host);
      dict->SetString(kKeyTotalUsage,
                      UTF16ToUTF8(ui::FormatBytes(
                          quota_info.temporary_usage +
                          quota_info.persistent_usage)));
      dict->SetString(kKeyTemporaryUsage,
                      UTF16ToUTF8(ui::FormatBytes(
                          quota_info.temporary_usage)));
      dict->SetString(kKeyPersistentUsage,
                      UTF16ToUTF8(ui::FormatBytes(
                          quota_info.persistent_usage)));
      break;
    }
    default:
#if defined(OS_MACOSX)
      dict->SetString(kKeyIcon, "chrome://theme/IDR_BOOKMARK_BAR_FOLDER");
#endif
      break;
  }
  return true;
}

void GetChildNodeList(CookieTreeNode* parent, int start, int count,
                      ListValue* nodes) {
  for (int i = 0; i < count; ++i) {
    DictionaryValue* dict = new DictionaryValue;
    CookieTreeNode* child = parent->GetChild(start + i);
    if (GetCookieTreeNodeDictionary(*child, dict))
      nodes->Append(dict);
    else
      delete dict;
  }
}

CookieTreeNode* GetTreeNodeFromPath(CookieTreeNode* root,
                                    const std::string& path) {
  std::vector<std::string> node_ids;
  base::SplitString(path, ',', &node_ids);

  CookieTreeNode* child = NULL;
  CookieTreeNode* parent = root;
  int child_index = -1;

  // Validate the tree path and get the node pointer.
  for (size_t i = 0; i < node_ids.size(); ++i) {
    child = reinterpret_cast<CookieTreeNode*>(
        HexStringToPointer(node_ids[i]));

    child_index = parent->GetIndexOf(child);
    if (child_index == -1)
      break;

    parent = child;
  }

  return child_index >= 0 ? child : NULL;
}

}  // namespace cookies_tree_model_util
