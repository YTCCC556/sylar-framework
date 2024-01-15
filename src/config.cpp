//
// Created by YTCCC on 2023/10/31.
//

#include "config.h"
#include "yaml-cpp/yaml.h"
#include <string>

namespace ytccc {

static void
ListAllMember(const std::string &prefix, const YAML::Node &node,
              std::list<std::pair<std::string, const YAML::Node>> &output) {
    if (prefix.find_first_not_of("qwertyuiopasdfghjklzxcvbnm._0123456789") !=
        std::string::npos) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "Config invaild name:" << prefix << ":" << node;
        return;
    }
    output.emplace_back(prefix, node);
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar()
                                         : prefix + "." + it->first.Scalar(),
                          it->second, output);
        }
    }
}

//查找当前项
ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    MutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

void Config::LoadFromYaml(const YAML::Node &root) {
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);//将root中所有键值对存入output

    for (auto &i: all_nodes) {
        std::string key = i.first;
        if (key.empty()) { continue; }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);//查找当前项

        if (var) {//如果找到
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

void Config::Visit(const std::function<void(ConfigVarBase::ptr)> &cb) {
    MutexType::ReadLock lock(GetMutex());
    ConfigVarMap &m = GetDatas();
    for (auto &it: m) { cb(it.second); }
}

}// namespace ytccc
