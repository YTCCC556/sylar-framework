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
                              "system in vector");
ytccc::ConfigVar<std::list<int>>::ptr g_list_value_config =
        ytccc::Config::Lookup("system.int_list", std::list<int>{1, 2},
                              "system in list");
ytccc::ConfigVar<std::set<int>>::ptr g_set_value_config = ytccc::Config::Lookup(
        "system.int_set", std::set<int>{1, 2}, "system in set");
ytccc::ConfigVar<std::unordered_set<int>>::ptr g_unorderset_value_config =
        ytccc::Config::Lookup("system.int_unorderset",
                              std::unordered_set<int>{1, 2}, "system in set");
ytccc::ConfigVar<std::map<std::string, int>>::ptr g_map_value_config =
        ytccc::Config::Lookup("system.int_map",
                              std::map<std::string, int>{{"k1", 22}},
                              "system in map");
ytccc::ConfigVar<std::unordered_map<std::string, int>>::ptr
        g_unordermap_value_config = ytccc::Config::Lookup(
                "system.int_unordermap",
                std::unordered_map<std::string, int>{{"k", 2}},
                "system in unordermap");

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

#define XX(g_var, name, prefix)                                                \
    {                                                                          \
        auto v = g_var->getValue();                                            \
        for (auto &i: v) {                                                     \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ":" << i;    \
        }                                                                      \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                                       \
                << #prefix " " #name " yaml:" << g_var->toString();            \
    }

#define XX_M(g_var, name, prefix)                                              \
    {                                                                          \
        auto v = g_var->getValue();                                            \
        for (auto &i: v) {                                                     \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                                   \
                    << #prefix " " #name ":{" << i.first << "-" << i.second    \
                    << "}";                                                    \
        }                                                                      \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                                       \
                << #prefix " " #name " yaml:" << g_var->toString();            \
    }

    XX(g_vec_value_config, vec, before);
    XX(g_list_value_config, list, before);
    XX(g_set_value_config, set, before);
    XX(g_unorderset_value_config, unorderset, before);
    XX_M(g_map_value_config, map, before);
    XX_M(g_unordermap_value_config, unordermap, before);

    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    ytccc::Config::LoadFromYaml(root);


    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "after:" << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "after:" << g_float_value_config->toString();

    XX(g_vec_value_config, vec, after);
    XX(g_list_value_config, list, after);
    XX(g_set_value_config, set, after);
    XX(g_unorderset_value_config, unorderset, after);
    XX_M(g_map_value_config, map, after);
    XX_M(g_unordermap_value_config, unordermap, after);
#undef XX
}

int main(int argc, char **argv) {

    //    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    //    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    //    test_yaml();
    test_config();

    return 0;
}
