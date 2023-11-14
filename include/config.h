//
// Created by YTCCC on 2023/10/31.
//

#ifndef SYLAR_FRAMEWORK_CONFIG_H
#define SYLAR_FRAMEWORK_CONFIG_H


#include "log.h"
#include "thread.h"
#include <boost/lexical_cast.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <yaml-cpp/yaml.h>

#include <functional>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace ytccc {

#define YTC_LOCK_CONFIG RWMutex;

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string &name, const std::string &description = "")
        : m_name(name), m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_description; }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string &val) = 0;
    virtual std::string getTypeName() const = 0;

private:
    std::string m_name;
    std::string m_description;
};

//从F类型转化成T类型 仿函数
template<class F, class T>
class LexicalCast {
public:
    T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

//常用容器的偏特化 vector
template<class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    // 将string转换成vector
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for (auto &&i: node) {
            ss.str("");
            ss << i;
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};
template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    // 将vector转换成string
    std::string operator()(const std::vector<T> &v) {
        YAML::Node node;
        for (auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//list支持
template<class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};
template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T> &v) {
        YAML::Node node;
        for (auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//set支持
template<class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};
template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T> &v) {
        YAML::Node node;
        for (auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
//unordered_set支持
template<class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};
template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T> &v) {
        YAML::Node node;
        for (auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//map支持
template<class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};
template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T> &v) {
        YAML::Node node;
        for (auto &i: v) {
            node[i.first] = YAML::Node(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
//unordermap支持
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};
template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T> &v) {
        YAML::Node node;
        for (auto &i: v) {
            node[i.first] = YAML::Node(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/*FromStr T operator()(const std::string&)
ToStr std::string operator()()const T&
*/
template<class T, class FromStr = LexicalCast<std::string, T>,
         class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex MutexType;
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T &old_value, const T &new_value)>
            on_change_cb;
    //functional封装成指针，lambda表达式，
    ConfigVar(const std::string &name, const T &default_value,
              const std::string &description = "")
        : ConfigVarBase(name, description), m_val(default_value) {}

    std::string toString() override {
        try {
            //return boost::lexical_cast<std::string>(m_val);
            MutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                    << "ConfigVar::toString exception" << e.what()
                    << "convert:" << typeid(m_val).name() << "to string";
        }
        return "";
    }

    bool fromString(const std::string &val) override {
        try {
            //            m_val = boost::lexical_cast<T>(val);

            setValue(FromStr()(val));
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                    << "ConfigVar::toString exception" << e.what()
                    << "convert : string to" << typeid(m_val).name();
        }
        return false;
    }

    std::string getTypeName() const override { return typeid(T).name(); }
    const T getValue(){
        MutexType::ReadLock lock(m_mutex);
        return m_val;
    }
    void setValue(const T &v) {
        {
            MutexType::ReadLock lock(m_mutex);
            if (v == m_val) return;
            for (auto &i: m_cbs) i.second(m_val, v);
        }
        MutexType::WriteLock lock(m_mutex);
        m_val = v;
    }

    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;

        MutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }
    void delListener(uint64_t key) {
        MutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }
    on_change_cb getListener(uint64_t key) {
        MutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }
    void clearListener() {
        MutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    MutexType m_mutex;
    T m_val;
    //变更回调函数组，functional没有比较函数，
    std::map<uint64_t, on_change_cb> m_cbs;
};

class Config {
public:

    typedef RWMutex MutexType;
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>
    static typename ConfigVar<T>::ptr
    // 用于创建或获取对象参数名的配置参数
    // 配置参数名称，T类型的参数默认值，字符串形式的参数描述
    Lookup(const std::string &name, const T &default_value,
           const std::string &description = "") {
        //typedef WriteScopeLockImpl<RWMutex> WriteLock;
        MutexType::WriteLock lock(GetMutex());
        auto it = GetDatas().find(name);
        // 在map中找到相同名称的配置项
        if (it != GetDatas().end()) {
            // it.second是ConfigVarBase::ptr类型指针，指向ConfigVar某个全特化对象
            // 属于基类转换成子类
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            /* 返回为空，两种情况，真不存在，或者存在但是类型不同
             * 如果tmp为空，表示map中存在相同名字的配置项，
             * 但是map中配置项的参数类型和传入的默认值的参数类型不一致，返回nullptr
             * 否者返回正确的配置参数 */
            if (tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                        << "Lookup name = " << name << " exists";
                return tmp;
            } else {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                        << "Lookup name = " << name << " exists but type not "
                        << typeid(T).name()
                        << "real_type = " << it->second->getTypeName() << ""
                        << it->second->toString();
                return nullptr;
            }
        }
        if (name.find_first_not_of("qwertyuiopasdfghjklzxcvbnm"
                                   "._0123456789") != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invall" << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(
                new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }
    // 查找配置参数，传入配置参数名称，返回对应的配置参数，若不存在或者类型错误的话，都会返回nullptr
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        MutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) { return nullptr; }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }
    // 使用yaml初始化配置模块，传入yaml对象，采用类似于遍历二叉树的方式遍历。
    static void LoadFromYaml(const YAML::Node &root);
    // 返回基类指针
    static ConfigVarBase::ptr LookupBase(const std::string &name);

    //为了调试方便，返回整个map内容
    static void Visit(const std::function<void(ConfigVarBase::ptr)>& cb);
private:
    static ConfigVarMap &GetDatas() {
        static ConfigVarMap m_datas;
        return m_datas;
    }
    // 静态成员初始化顺序要比其他的晚，会出现问题，内存错误
    static MutexType &GetMutex() {
        static MutexType m_mutex;
        return m_mutex;
    }
};

#undef YTC_LOCK_CONFIG
}// namespace ytccc


#endif//SYLAR_FRAMEWORK_CONFIG_H
