//
// Created by YTCCC on 2023/10/31.
//
#include "config.h"
#include "log.h"
#include <yaml-cpp/yaml.h>

ytccc::ConfigVar<int>::ptr g_int_value_config =
        ytccc::Config::Lookup("system.port", (int) 8080, "system port");

void print_yaml(const YAML::Node &node, int level) {
    if (node.IsScalar()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                << node.Scalar() << "-" << node.Tag() << level;
    } else if (node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "NULL -" << node.Tag() << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                    << it->first << "-" << it->second.Tag() << "-" << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                    << it->first << "-" << it->second.Tag() << "-" << level;
            print_yaml(it->second, level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    print_yaml(root, 0);
    //    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

int main(int argc, char **argv) {

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    test_yaml();
    return 0;
}
