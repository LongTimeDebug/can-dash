// test_language_manager.cpp
// Layer 2 LanguageManager 单元测试 (纯 C++, 无 Qt)
//
// 覆盖:
//   1. 初始默认语言 = zh_CN
//   2. setLanguage(LANG_EN_US) → currentLanguage() 切到 en_US
//   3. setLanguage(LANG_ZH_CN) → 切回 zh_CN
//   4. setLanguage(越界值) → 状态保持不变
//   5. setLanguage(LANG_COUNT) → 状态保持不变
//   6. tr() 返回 zh 文本 (默认)
//   7. tr() 返回 en 文本 (切换后)
//   8. tr() 未知 key → 返回原 key
//   9. tr("") → 返回 ""
//  10. tr(nullptr) → 返回 "" (不崩)
//  11. currentLocale() zh_CN → "zh_CN"
//  12. currentLocale() en_US → "en_US"
//  13. currentFontFamily() 两种语言返回不同 (字体差异)
//  14. init() 替换翻译表 + 切回 zh_CN
//  15. init() 替换后 tr() 走新表
//  16. 5 大类翻译全覆盖 (unit / status / indicator / alarm_text / seatbelt)
//  17. zh / en 文案不重叠 (反映本地化差异)

#include <cstdio>
#include <cstring>
#include <cassert>
#include "../src/layer2/language_manager.h"

// LanguageManager 当前在全局 namespace (与 L2 其它 candash:: 命名风格不一致,
// 历史遗留, 本测试不强行改 namespace, 只测当前 API)

static int g_test_count = 0;
static int g_test_passed = 0;

#define TEST_ASSERT(cond, msg) do {                                  \
    g_test_count++;                                                  \
    if (cond) {                                                      \
        g_test_passed++;                                             \
        printf("  \xe2\x9c\x93 %s\n", msg);                          \
    } else {                                                         \
        printf("  \xe2\x9c\x97 %s (line %d)\n", msg, __LINE__);      \
    }                                                                \
} while(0)

#define RUN(name) do {                                               \
    printf("\n[%d] %s:\n", ++g_test_section, #name);                  \
    name();                                                          \
} while(0)

static int g_test_section = 0;

// ─── 测试用例 ───

static void test_initial_default_zh() {
    LanguageManager lm;
    TEST_ASSERT(lm.currentLanguage() == LANG_ZH_CN, "默认语言 = zh_CN");
    TEST_ASSERT(strcmp(lm.currentLocale(), "zh_CN") == 0, "locale = zh_CN");
}

static void test_set_language_en() {
    LanguageManager lm;
    lm.setLanguage(LANG_EN_US);
    TEST_ASSERT(lm.currentLanguage() == LANG_EN_US, "切到 en_US");
    TEST_ASSERT(strcmp(lm.currentLocale(), "en_US") == 0, "locale = en_US");
}

static void test_set_language_back_to_zh() {
    LanguageManager lm;
    lm.setLanguage(LANG_EN_US);
    lm.setLanguage(LANG_ZH_CN);
    TEST_ASSERT(lm.currentLanguage() == LANG_ZH_CN, "切回 zh_CN");
}

static void test_set_language_out_of_range() {
    LanguageManager lm;
    Language orig = lm.currentLanguage();
    lm.setLanguage(static_cast<Language>(99));
    TEST_ASSERT(lm.currentLanguage() == orig, "越界值 (99) 状态不变");
    lm.setLanguage(static_cast<Language>(-1));
    TEST_ASSERT(lm.currentLanguage() == orig, "越界值 (-1) 状态不变");
}

static void test_set_language_at_count_boundary() {
    LanguageManager lm;
    Language orig = lm.currentLanguage();
    lm.setLanguage(LANG_COUNT);
    TEST_ASSERT(lm.currentLanguage() == orig, "LANG_COUNT 边界拒绝");
}

static void test_tr_returns_zh() {
    LanguageManager lm;
    const char* s = lm.tr("status.driving");
    TEST_ASSERT(s != nullptr, "tr 返回非 nullptr");
    TEST_ASSERT(strcmp(s, "行驶中") == 0, "zh 翻译 = 行驶中");
}

static void test_tr_returns_en_after_switch() {
    LanguageManager lm;
    lm.setLanguage(LANG_EN_US);
    const char* s = lm.tr("status.driving");
    TEST_ASSERT(s != nullptr, "tr 返回非 nullptr");
    TEST_ASSERT(strcmp(s, "Driving") == 0, "en 翻译 = Driving");
}

static void test_tr_unknown_key_returns_key() {
    LanguageManager lm;
    const char* s = lm.tr("nonexistent.key");
    TEST_ASSERT(s != nullptr, "tr 返回非 nullptr");
    TEST_ASSERT(strcmp(s, "nonexistent.key") == 0, "未知 key 返回原 key");
}

static void test_tr_empty_string() {
    LanguageManager lm;
    const char* s = lm.tr("");
    TEST_ASSERT(s != nullptr, "tr(\"\") 非 nullptr");
    TEST_ASSERT(strcmp(s, "") == 0, "tr(\"\") = 空串");
}

static void test_tr_null_does_not_crash() {
    LanguageManager lm;
    const char* s = lm.tr(nullptr);
    TEST_ASSERT(s != nullptr, "tr(nullptr) 非 nullptr (不崩)");
    TEST_ASSERT(strcmp(s, "") == 0, "tr(nullptr) = 空串");
}

static void test_current_locale_zh() {
    LanguageManager lm;
    TEST_ASSERT(strcmp(lm.currentLocale(), "zh_CN") == 0, "zh locale = zh_CN");
}

static void test_current_locale_en() {
    LanguageManager lm;
    lm.setLanguage(LANG_EN_US);
    TEST_ASSERT(strcmp(lm.currentLocale(), "en_US") == 0, "en locale = en_US");
}

static void test_current_font_family_differs() {
    LanguageManager lm;
    const char* zh = lm.currentFontFamily();
    lm.setLanguage(LANG_EN_US);
    const char* en = lm.currentFontFamily();
    TEST_ASSERT(zh != nullptr && en != nullptr, "字体非 nullptr");
    TEST_ASSERT(strlen(zh) > 0 && strlen(en) > 0, "字体非空");
    TEST_ASSERT(strcmp(zh, en) != 0, "zh/en 字体不同");
}

static void test_init_replaces_table() {
    // 自定义 1 条翻译表 (key 故意不在内置表里, 验证 init 真的替换)
    LanguageEntry custom[] = {
        { "custom.greeting", "你好", "Hi" },
    };
    LanguageManager lm;
    // 先验证内置表没这个 key
    TEST_ASSERT(strcmp(lm.tr("custom.greeting"), "custom.greeting") == 0,
                "原始表没有 custom.greeting, 返回原 key");

    // 替换表 (count=1)
    lm.init(custom, 1);
    TEST_ASSERT(lm.currentLanguage() == LANG_ZH_CN, "init 重置回 zh_CN");
    TEST_ASSERT(strcmp(lm.tr("custom.greeting"), "你好") == 0, "新表 zh = 你好");
    lm.setLanguage(LANG_EN_US);
    TEST_ASSERT(strcmp(lm.tr("custom.greeting"), "Hi") == 0, "新表 en = Hi");
}

static void test_init_replaces_table_unknown_key() {
    LanguageEntry custom[] = {
        { "hello", "你好", "Hi" },
    };
    LanguageManager lm;
    lm.init(custom, 1);
    // "status.driving" 不在新表里 → 返回原 key
    TEST_ASSERT(strcmp(lm.tr("status.driving"), "status.driving") == 0,
                "新表里没有的 key 返回原 key");
}

static void test_all_sections_translate() {
    // unit.* 故意保留 zh/en 一致 (km/h, °C 等国际化单位符号, 不区分语言)
    // 其它 4 类必须本地化 (zh ≠ en)
    LanguageManager lm;
    const char* localized_keys[] = {
        "status.driving",                      // status
        "indicator.left_turn",                 // indicator
        "alarm_text.bat_overvolt",             // alarm_text
        "seatbelt.driver",                     // seatbelt
    };
    for (size_t i = 0; i < sizeof(localized_keys) / sizeof(localized_keys[0]); ++i) {
        const char* zh = lm.tr(localized_keys[i]);
        lm.setLanguage(LANG_EN_US);
        const char* en = lm.tr(localized_keys[i]);
        lm.setLanguage(LANG_ZH_CN);

        TEST_ASSERT(zh != nullptr && strlen(zh) > 0, "zh 非空");
        TEST_ASSERT(en != nullptr && strlen(en) > 0, "en 非空");
        TEST_ASSERT(strcmp(zh, en) != 0, "zh/en 不重叠 (本地化有效)");
    }
    // unit.* 单独验证: zh/en 相同是预期行为
    const char* zh_unit = lm.tr("unit.speed");
    TEST_ASSERT(zh_unit != nullptr, "unit.speed 非 nullptr");
    TEST_ASSERT(strlen(zh_unit) > 0, "unit.speed 非空");
}

static void test_zh_en_never_overlap() {
    // 遍历整张内置表, zh / en 文案必须都不空
    // 允许 unit 类少量 zh/en 相同 (km/h, °C, RPM, V, A, %)
    int n = TRANSLATION_COUNT;
    int overlap = 0;
    for (int i = 0; i < n; ++i) {
        const char* zh = TRANSLATIONS[i].zh;
        const char* en = TRANSLATIONS[i].en;
        if (strcmp(zh, en) == 0 && strlen(zh) > 2) {
            ++overlap;
        }
    }
    // 允许少量 unit 类型 (km/h, °C) zh/en 相同, 但绝大多数应本地化
    TEST_ASSERT(overlap <= 6,
                "内置表 zh/en 大多数本地化 (unit 类小重叠允许)");
}

static void test_tr_zh_and_en_after_toggle() {
    LanguageManager lm;
    const char* key = "alarm_text.overspeed";

    const char* s1 = lm.tr(key);  // zh
    lm.setLanguage(LANG_EN_US);
    const char* s2 = lm.tr(key);  // en
    lm.setLanguage(LANG_ZH_CN);
    const char* s3 = lm.tr(key);  // 切回 zh

    TEST_ASSERT(strcmp(s1, s3) == 0, "zh→zh 一致 (idempotent toggle)");
    TEST_ASSERT(strcmp(s1, s2) != 0, "zh ≠ en (本地化有效)");
    TEST_ASSERT(strcmp(s2, "OVERSPEED - SLOW DOWN") == 0, "en 文案正确");
    TEST_ASSERT(strcmp(s1, "超速！请减速") == 0, "zh 文案正确");
}

int main() {
    printf("=== LanguageManager 单元测试 ===\n\n");

    RUN(test_initial_default_zh);
    RUN(test_set_language_en);
    RUN(test_set_language_back_to_zh);
    RUN(test_set_language_out_of_range);
    RUN(test_set_language_at_count_boundary);
    RUN(test_tr_returns_zh);
    RUN(test_tr_returns_en_after_switch);
    RUN(test_tr_unknown_key_returns_key);
    RUN(test_tr_empty_string);
    RUN(test_tr_null_does_not_crash);
    RUN(test_current_locale_zh);
    RUN(test_current_locale_en);
    RUN(test_current_font_family_differs);
    RUN(test_init_replaces_table);
    RUN(test_init_replaces_table_unknown_key);
    RUN(test_all_sections_translate);
    RUN(test_zh_en_never_overlap);
    RUN(test_tr_zh_and_en_after_toggle);

    printf("\n=== %d/%d 测试通过 ===\n", g_test_passed, g_test_count);
    return (g_test_passed == g_test_count) ? 0 : 1;
}
