#ifndef NATIVEBRIDGE_HPP
#define NATIVEBRIDGE_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "InterlockCore.h"

class NativeBridge : public QObject {
    Q_OBJECT

    // Core Status
    Q_PROPERTY(bool isBooted READ isBooted NOTIFY bootStatusChanged)

    // Data Mappings
    Q_PROPERTY(QVariantMap snapshot READ snapshot NOTIFY snapshotUpdated)
    Q_PROPERTY(QVariantList hardwareNodes READ hardwareNodes NOTIFY hardwareUpdated)
    Q_PROPERTY(QVariantList eventLog READ eventLog NOTIFY eventLogChanged)
    Q_PROPERTY(QVariantMap networkStats READ networkStats NOTIFY networkUpdated)

    // Fast Accessors
    Q_PROPERTY(QString operationName READ operationName NOTIFY snapshotUpdated)
    Q_PROPERTY(QString phase READ phase NOTIFY snapshotUpdated)
    Q_PROPERTY(float confidence READ confidence NOTIFY snapshotUpdated)

public:
    explicit NativeBridge(QObject *parent = nullptr);
    ~NativeBridge();

    bool isBooted() const { return m_isBooted; }
    QVariantMap snapshot() const { return m_snapshot; }
    QVariantList hardwareNodes() const { return m_hardwareNodes; }
    QVariantList eventLog() const { return m_eventLog; }
    QVariantMap networkStats() const { return m_networkStats; }

    QString operationName() const { return m_snapshot.value("name", "IDLE").toString(); }
    QString phase() const { return m_snapshot.value("phase", "INITIALIZING").toString(); }
    float confidence() const { return m_snapshot.value("confidence", 0.0).toFloat(); }

    Q_INVOKABLE void boot();
    Q_INVOKABLE void createOperation(const QString &name, const QString &preset);
    Q_INVOKABLE void joinOperation(const QString &opId);
    Q_INVOKABLE void executeAction(const QString &action);
    Q_INVOKABLE void injectMessage(const QString &to, const QString &content);
    Q_INVOKABLE void selectModel(const QString &modelId);
    Q_INVOKABLE QString processInference(const QString &prompt);
    Q_INVOKABLE QString deployPayload(uint32_t seed);
    Q_INVOKABLE QVariantList simulateSignalPropagation(const QVariantList &transmitters, const QVariantList &obstacles);

    // Shadow-Mover (Zig Integration)
    Q_INVOKABLE bool distributeTicket(uint32_t type, const QString &base64Data, const QVariantList &pids);

    // .chrxxx DSL Integration
    Q_INVOKABLE QVariantMap compileChrx(const QString &source);
    Q_INVOKABLE bool validateChoreography(const QString &chrxJson);

signals:
    void snapshotUpdated();
    void hardwareUpdated();
    void networkUpdated();
    void eventLogChanged();
    void bootStatusChanged();
    void newRawEvent(QString jsonEvent);

private slots:
    void updateMetrics();
    void handleIncomingEvent(const QString &json);

private:
    static void onNativeEvent(const char* jsonEvent);
    static NativeBridge* s_instance;

    bool m_isBooted = false;
    QVariantMap m_snapshot;
    QVariantList m_hardwareNodes;
    QVariantList m_eventLog;
    QVariantMap m_networkStats;
    QTimer *m_metricsTimer;
};

#endif // NATIVEBRIDGE_HPP
