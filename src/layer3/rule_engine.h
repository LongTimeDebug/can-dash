// rule_engine.h
// 运行时配置驱动的派生计算 / 状态机 / 报警引擎
//
// 机制:
//   - 启动时 loadRules(yaml)  → yaml-cpp 解析 → exprtk 编译每条表达式 (缓存)
//   - 16ms tick evaluate(ctx)  → 把 ctx (QVariantMap) 喂给 exprtk symbol table
//                              → 表达式内部调 setter (exprtk IFunction)
//                              → setter 回调 C++ 已有的 UI 赋值逻辑
//
// 设计目标: "改业务不动 C++" — 加新规则改 derived_rules.yaml 即可
// 引擎:     exprtk (header-only) + yaml-cpp
//
// 依赖注入:
//   - 表达式可用的变量 (QVariantMap key): ShmDataSource::onTick 末尾拍平
//   - 表达式可用的 setter (exprtk IFunction): 调 registerAction 注册

#pragma once

#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// exprtk 头文件
#include <exprtk.hpp>

// 前置声明: 避免在头文件 include yaml-cpp (拖慢编译)
namespace YAML { class Node; }

namespace candash {

// ── setter 签名: 表达式调用 → 回调 C++ ──
// 例如: set_trip_range_confidence(v) → 调 QtDataBinder setter
//       set_indicator(id, on, flash, hz) → 同上
// 参数统一 double (exprtk 限制), string 参数走 string table 查
class ExprRuleEngine {
public:
    ExprRuleEngine();
    ~ExprRuleEngine();

    // 禁止拷贝 (持有 exprtk expression 指针)
    ExprRuleEngine(const ExprRuleEngine&) = delete;
    ExprRuleEngine& operator=(const ExprRuleEngine&) = delete;

    // ── 注册 setter ──
    // name: yaml 里写的函数名 (如 "set_trip_range_confidence")
    // arity: 参数个数 (1-6, exprtk 限制)
    // fn: 实际逻辑 (调 QtDataBinder / 改 DisplaySnapshot 派生字段)
    using SetterFn = std::function<void(const std::vector<double>&)>;
    void registerSetter(const QString& name, int arity, SetterFn fn);

    // ── 加载 yaml 规则 ──
    // 启动时调一次, 内部用 exprtk 编译每条表达式缓存
    // 失败 → 返回 false, 错误打印到 qWarning
    bool loadRules(const QString& yamlPath);

    // ── 每帧评估 ──
    // ctx: DisplaySnapshot 拍平 + signal_lost 标志
    //      必含 key: 见 config/derived_rules.yaml 头部注释
    void evaluate(const QVariantMap& ctx);

    // 测试用: 加载的规则数
    int ruleCount() const { return static_cast<int>(m_rules.size()); }

private:
    // ── 一条编译后的规则 ──
    struct CompiledRule {
        std::string id;
        std::string expr_source;        // 原始字符串 (调试用)
        std::shared_ptr<exprtk::expression<double>> expr;
        std::shared_ptr<exprtk::symbol_table<double>> syms;
        std::vector<double>         var_slots;  // add_variable 绑定的 double 槽
        std::vector<std::string>    var_names;  // 槽对应变量名
    };

    // ── 已知 ctx key 列表 (启动时 add_variable 全部) ──
    // 跟 config/derived_rules.yaml 头部注释同步
    static const std::vector<std::string> kCtxKeys;

    // ── exprtk 自定义函数: 通用 N 参 setter ──
    template <size_t N>
    class SetterIfunction;

    // ── 内部: 注册 setter 时同时建好 IFunction 给 exprtk 用 ──
    struct SetterEntry {
        int arity;
        SetterFn fn;
        // IFunction 实例 (arity=1..6 各一个)
        std::shared_ptr<exprtk::ifunction<double>> ifunc1;
        std::shared_ptr<exprtk::ifunction<double>> ifunc2;
        std::shared_ptr<exprtk::ifunction<double>> ifunc3;
        std::shared_ptr<exprtk::ifunction<double>> ifunc4;
        std::shared_ptr<exprtk::ifunction<double>> ifunc5;
    };

    std::vector<CompiledRule> m_rules;
    std::unordered_map<std::string, std::shared_ptr<SetterEntry>> m_setters;

    // 内部辅助: 编译单条规则
    bool compileRule(const std::string& id,
                     const std::string& expr_src,
                     CompiledRule& out);
};

}  // namespace candash
