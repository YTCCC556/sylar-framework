//
// Created by YTCCC on 2023/10/31.
//
#include "config.h"
#include "log.h"
#include <yaml-cpp/yaml.h>

ytccc::ConfigVar<int>::ptr g_int_value_config =
        ytccc::Config::Lookup("system.port", (int) 8080, "system port");
ytccc::ConfigVar<float>::ptr g_float_value_config =
        ytccc::Config::Lookup("system.value", (float) 26.55f, "system values");
ytccc::ConfigVar<std::vector<int>>::ptr g_vec_value_config =
        ytccc::Config::Lookup("system.int_vec", std::vector<int>{1, 2},
                              "system in vec");


void print_yaml(const YAML::Node &node, int level) {
    if (node.IsScalar()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                << std::string(level, ' ') << node.Scalar() << "-"
                << node.Type() << level;
    } else if (node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                                         << "NULL -" << node.Type() << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                    << std::string(level * 4, ' ') << it->first << "-"
                    << it->second.Type() << "-" << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                    << std::string(level * 4, ' ') << i << "-" << node[i].Type()
                    << "-" << level;
            print_yaml(node[i], level);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    print_yaml(root, 0);
    //    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

void test_config() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "before:" << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "before:" << g_float_value_config->toString();
    auto v = g_vec_value_config->getValue();
    for (auto i: v) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before int vec:" << i;
    }

    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    ytccc::Config::LoadFromYaml(root);


    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "after:" << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "after:" << g_float_value_config->toString();

    v = g_vec_value_config->getValue();
    for (auto i: v) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after int vec:" << i;
    }
}

int main(int argc, char **argv) {

    //    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    //    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    //    test_yaml();
    test_config();

    return 0;
}
