// rule_engine.cpp
#include "rule_engine.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <yaml-cpp/yaml.h>

namespace candash {

// ── exprtk 自定义函数: 通用 N 参 setter 包装 ──
// exprtk::ifunction<T> 基类的 operator() 签名: T operator()(const T&, const T&, ...)
// add_function(name, ifunction<T>&) — 接受引用
// ifunction 是 virtual 基类, 必须用 const T& 签名 override
template <size_t N>
class ExprRuleEngine::SetterIfunction : public exprtk::ifunction<double> {
public:
    SetterIfunction(SetterFn fn)
        : exprtk::ifunction<double>(N), m_fn(std::move(fn)) {}

    double operator()(const double& a0) override { m_fn({a0}); return 0.0; }
private:
    SetterFn m_fn;
};
template <>
class ExprRuleEngine::SetterIfunction<2> : public exprtk::ifunction<double> {
public:
    SetterIfunction(SetterFn fn)
        : exprtk::ifunction<double>(2), m_fn(std::move(fn)) {}
    double operator()(const double& a0, const double& a1) override {
        m_fn({a0,a1}); return 0.0;
    }
private:
    SetterFn m_fn;
};
template <>
class ExprRuleEngine::SetterIfunction<3> : public exprtk::ifunction<double> {
public:
    SetterIfunction(SetterFn fn)
        : exprtk::ifunction<double>(3), m_fn(std::move(fn)) {}
    double operator()(const double& a0, const double& a1, const double& a2) override {
        m_fn({a0,a1,a2}); return 0.0;
    }
private:
    SetterFn m_fn;
};
template <>
class ExprRuleEngine::SetterIfunction<4> : public exprtk::ifunction<double> {
public:
    SetterIfunction(SetterFn fn)
        : exprtk::ifunction<double>(4), m_fn(std::move(fn)) {}
    double operator()(const double& a0, const double& a1,
                      const double& a2, const double& a3) override {
        m_fn({a0,a1,a2,a3}); return 0.0;
    }
private:
    SetterFn m_fn;
};
template <>
class ExprRuleEngine::SetterIfunction<5> : public exprtk::ifunction<double> {
public:
    SetterIfunction(SetterFn fn)
        : exprtk::ifunction<double>(5), m_fn(std::move(fn)) {}
    double operator()(const double& a0, const double& a1, const double& a2,
                      const double& a3, const double& a4) override {
        m_fn({a0,a1,a2,a3,a4}); return 0.0;
    }
private:
    SetterFn m_fn;
};

template class ExprRuleEngine::SetterIfunction<1>;
template class ExprRuleEngine::SetterIfunction<2>;
template class ExprRuleEngine::SetterIfunction<3>;
template class ExprRuleEngine::SetterIfunction<4>;
template class ExprRuleEngine::SetterIfunction<5>;

// ── 已知 ctx key (跟 config/derived_rules.yaml 头部注释同步) ──
const std::vector<std::string> ExprRuleEngine::kCtxKeys = {
    "bat_soc", "bat_volt", "vehicle_speed", "motor_temp",
    "precharge_status", "idle_seconds",
    "bat_soc_lost", "bat_volt_lost", "vehicle_speed_lost",
    "driver_occupied", "driver_buckled",
    "passenger_occupied", "passenger_buckled",
    "health", "view_current"
};

ExprRuleEngine::ExprRuleEngine() = default;
ExprRuleEngine::~ExprRuleEngine() = default;

void ExprRuleEngine::registerSetter(const QString& name, int arity, SetterFn fn) {
    if (arity < 1 || arity > 5) {
        qWarning() << "[RuleEngine] setter" << name
                   << "arity must be 1-5, got" << arity;
        return;
    }
    auto entry = std::make_shared<SetterEntry>();
    entry->arity = arity;
    entry->fn = std::move(fn);
    switch (arity) {
        case 1: entry->ifunc1 = std::make_shared<SetterIfunction<1>>(entry->fn); break;
        case 2: entry->ifunc2 = std::make_shared<SetterIfunction<2>>(entry->fn); break;
        case 3: entry->ifunc3 = std::make_shared<SetterIfunction<3>>(entry->fn); break;
        case 4: entry->ifunc4 = std::make_shared<SetterIfunction<4>>(entry->fn); break;
        case 5: entry->ifunc5 = std::make_shared<SetterIfunction<5>>(entry->fn); break;
    }
    m_setters[name.toStdString()] = std::move(entry);
}

bool ExprRuleEngine::loadRules(const QString& yamlPath) {
    QFileInfo fi(yamlPath);
    if (!fi.exists()) {
        qWarning() << "[RuleEngine] yaml not found:" << yamlPath;
        return false;
    }

    YAML::Node root;
    try {
        root = YAML::LoadFile(yamlPath.toStdString());
    } catch (const std::exception& e) {
        qWarning() << "[RuleEngine] yaml parse error:" << e.what();
        return false;
    }

    m_rules.clear();
    for (auto group : root) {
        for (const auto& rule : group.second) {
            std::string id  = rule["id"].as<std::string>();
            std::string src = rule["expr"].as<std::string>();
            CompiledRule cr;
            if (!compileRule(id, src, cr)) return false;
            m_rules.push_back(std::move(cr));
        }
    }
    qInfo() << "[RuleEngine] loaded" << m_rules.size() << "rules from" << yamlPath;
    return true;
}

bool ExprRuleEngine::compileRule(const std::string& id,
                                  const std::string& expr_src,
                                  CompiledRule& out) {
    out.id = id;
    out.expr_source = expr_src;
    out.expr = std::make_shared<exprtk::expression<double>>();
    out.syms = std::make_shared<exprtk::symbol_table<double>>();

    // 1. 绑定 ctx key → double 槽 (add_variable 必须在 compile 前完成)
    out.var_slots.assign(kCtxKeys.size(), 0.0);
    out.var_names = kCtxKeys;
    for (size_t i = 0; i < kCtxKeys.size(); ++i) {
        out.syms->add_variable(kCtxKeys[i], out.var_slots[i]);
    }

    // 2. 注册 setter (add_function 接受 ifunction<T>& 引用)
    for (auto& [name, entry] : m_setters) {
        switch (entry->arity) {
            case 1: out.syms->add_function(name, *entry->ifunc1); break;
            case 2: out.syms->add_function(name, *entry->ifunc2); break;
            case 3: out.syms->add_function(name, *entry->ifunc3); break;
            case 4: out.syms->add_function(name, *entry->ifunc4); break;
            case 5: out.syms->add_function(name, *entry->ifunc5); break;
        }
    }

    out.expr->register_symbol_table(*out.syms);

    // 3. 编译
    exprtk::parser<double> parser;
    if (!parser.compile(expr_src, *out.expr)) {
        qWarning() << "[RuleEngine] compile error in rule" << id.c_str()
                   << ":" << parser.error().c_str();
        qWarning() << "[RuleEngine]   source:" << expr_src.c_str();
        return false;
    }
    return true;
}

void ExprRuleEngine::evaluate(const QVariantMap& ctx) {
    for (auto& r : m_rules) {
        // 改 slot 值 (exprtk 读的是 var_slots 内存)
        for (size_t i = 0; i < r.var_names.size(); ++i) {
            auto it = ctx.find(QString::fromStdString(r.var_names[i]));
            if (it != ctx.end()) {
                bool ok = false;
                double v = it.value().toDouble(&ok);
                if (ok) r.var_slots[i] = v;
            }
        }
        r.expr->value();
    }
}

}  // namespace candash
