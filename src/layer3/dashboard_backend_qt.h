// dashboard_backend_qt.h
// Layer 3: Qt 适配层 - DashboardBackend（仅显示，从共享内存读取）
// 暴露 Q_PROPERTY 给 QML

#pragma once
#include <QObject>
#include <QVariant>
#include <QTimer>
#include <QVariantMap>
#include "layer1/shm/shm_display.h"

class LanguageManager;

class DashboardBackend : public QObject {
    Q_OBJECT

    // ─── 报警状态（QML 绑定）───
    Q_PROPERTY(bool alarmActive READ alarmActive NOTIFY alarmActiveChanged)
    Q_PROPERTY(QString alarmMessageZh READ alarmMessageZh NOTIFY alarmActiveChanged)
    Q_PROPERTY(QVariantList alarmList READ alarmList NOTIFY alarmActiveChanged)

    // ─── 安全带状态（QML 绑定）───
    Q_PROPERTY(bool seatBeltWarningActive READ seatBeltWarningActive NOTIFY seatBeltWarningChanged)
    Q_PROPERTY(QString seatBeltMessage READ seatBeltMessage NOTIFY seatBeltWarningChanged)
    Q_PROPERTY(QVariantList seatIconStates READ seatIconStates NOTIFY seatIconStatesChanged)
    Q_PROPERTY(bool isMoving READ isMoving NOTIFY movingChanged)

    // ─── 显示数据（QML 绑定）───
    Q_PROPERTY(QVariantMap displayData READ displayData NOTIFY displayDataChanged)

    // ─── 指示灯状态（QML 绑定）───
    Q_PROPERTY(QVariantMap indicatorStates READ indicatorStates NOTIFY indicatorStatesChanged)

    // ─── 多语言 ───
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString currentFont READ currentFont NOTIFY languageChanged)

    // ─── 处理器健康（QML 绑定，超时显示降级 UI）───
    Q_PROPERTY(bool processorOnline READ processorOnline NOTIFY processorHealthChanged)
    Q_PROPERTY(QString processorStatus READ processorStatus NOTIFY processorHealthChanged)

    // ─── 数据健康（PR2 新增：FPS/数据龄/帧序号/字段有效性）───
    Q_PROPERTY(qulonglong dataAgeMs READ dataAgeMs NOTIFY dataHealthChanged)
    Q_PROPERTY(qulonglong frameSeq READ frameSeq NOTIFY dataHealthChanged)
    Q_PROPERTY(double dataFps READ dataFps NOTIFY dataHealthChanged)
    Q_PROPERTY(QVariantMap fieldValidity READ fieldValidity NOTIFY dataHealthChanged)
    Q_PROPERTY(uint64_t droppedFrames READ droppedFrames NOTIFY dataHealthChanged)

public:
    explicit DashboardBackend(QObject* parent = nullptr);
    ~DashboardBackend();

    void init();

    // Qt 属性访问器
    bool alarmActive() const { return m_backendAlarmActive; }
    QString alarmMessageZh() const { return m_backendAlarmMessageZh; }
    QVariantList alarmList() const { return m_alarmList; }

    bool seatBeltWarningActive() const { return m_seatBeltActive; }
    QString seatBeltMessage() const { return m_seatBeltMessage; }
    QVariantList seatIconStates() const { return m_seatIconStates; }
    bool isMoving() const { return m_isMoving; }

    QVariantMap displayData() const { return m_displayData; }
    QVariantMap indicatorStates() const { return m_indicatorStates; }

    // QML 通用接口
    Q_INVOKABLE QVariant get(const QString& key) const;
    Q_INVOKABLE void set(const QString& key, const QVariant& value);

    // 多语言
    Q_INVOKABLE QString tr(const QString& key) const;
    Q_INVOKABLE void setLanguage(const QString& lang);
    QString currentLanguage() const { return m_currentLanguage; }
    QString currentFont() const;

    // 健康状态
    bool processorOnline() const { return m_processorOnline; }
    QString processorStatus() const { return m_processorStatus; }

    // 数据健康（PR2 新增）
    qulonglong dataAgeMs() const { return m_dataAgeMs; }
    qulonglong frameSeq() const { return m_frameSeq; }
    double dataFps() const { return m_dataFps; }
    QVariantMap fieldValidity() const { return m_fieldValidity; }
    uint64_t droppedFrames() const { return m_droppedFrames; }

    // 指示灯查询
    Q_INVOKABLE bool indicatorOn(const QString& key) const;

    // 指示灯控制（来自 QML，仅调试用）
    Q_INVOKABLE void setIndicator(const QString& widget_id, bool on, bool flash, float hz);
    Q_INVOKABLE void setIndicator(const QString& widget_id, bool on);

signals:
    void alarmActiveChanged();
    void indicatorStatesChanged();
    void seatBeltWarningChanged();
    void seatIconStatesChanged();
    void movingChanged();
    void displayDataChanged();
    void languageChanged();
    void processorHealthChanged();
    void dataHealthChanged();   // PR2 新增（FPS/age/frameSeq/validity）

public slots:
    void onTick();

private:
    QVariantMap indicatorSlotToMap(const ShmIndicatorSlot& slot) const;

    LanguageManager* m_langManager = nullptr;
    QString m_currentLanguage = "zh_CN";

    // 报警状态
    bool m_backendAlarmActive = false;
    QString m_backendAlarmMessageZh;
    QVariantList m_alarmList;

    // 安全带状态
    bool m_seatBeltActive = false;
    QString m_seatBeltMessage;
    QVariantList m_seatIconStates;

    bool m_isMoving = false;
    QVariantMap m_displayData;
    QVariantMap m_indicatorStates;

    QTimer* m_tickTimer = nullptr;

    // 处理器健康状态（dash 端监测）
    bool m_processorOnline = false;
    QString m_processorStatus = QStringLiteral("disconnected");
    uint64_t m_lastSeenMs = 0;

    // 数据健康状态（PR2 新增）
    qulonglong m_dataAgeMs = 0;        // 距上次 commit 的毫秒数
    qulonglong m_frameSeq = 0;         // 上一帧的 frame_seq
    qulonglong m_lastFrameSeq = 0;     // 上次已处理的 frame_seq（用于检测丢帧）
    uint64_t m_droppedFrames = 0;      // 累计丢帧数（m_frameSeq - m_lastFrameSeq - 1）
    double m_dataFps = 0.0;            // 数据 FPS（最近 1s 内 commit 次数）
    QVariantMap m_fieldValidity;       // 字段名 → bool (true=有效/false=无效)
    uint64_t m_fpsWindowStart = 0;     // FPS 滑动窗口起点
    uint32_t m_fpsCountInWindow = 0;   // 窗口内 commit 次数
    uint32_t m_lastUpdatedMask = 0;    // 上次的 updated_mask（用于生成 fieldValidity）
};
