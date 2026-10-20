#include "NativeBridge.hpp"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include "NetworkBridge.hpp"

NativeBridge* NativeBridge::s_instance = nullptr;

NativeBridge::NativeBridge(QObject *parent) : QObject(parent) {
    s_instance = this;
    m_metricsTimer = new QTimer(this);
    connect(m_metricsTimer, &QTimer::timeout, this, &NativeBridge::updateMetrics);

    // Conectar el bus de eventos crudos al procesador de lógica
    connect(this, &NativeBridge::newRawEvent, this, &NativeBridge::handleIncomingEvent);
}

NativeBridge::~NativeBridge() {
    interlock::InterlockCore::shutdown();
}

void NativeBridge::boot() {
    if (m_isBooted) return;

    qDebug() << "[NativeBridge] Initializing Integrated Polyglot Engine...";

    QString resourcePath;
#ifdef Q_OS_MAC
    // En macOS, si estamos en un Bundle, apuntamos a Resources
    if (QCoreApplication::applicationDirPath().contains(".app/Contents/MacOS")) {
        QDir bundleDir(QCoreApplication::applicationDirPath());
        bundleDir.cdUp();
        resourcePath = bundleDir.absolutePath() + "/Resources";
    } else {
        // En desarrollo, usamos el path del proyecto
        resourcePath = QDir::currentPath() + "/backend";
    }
#else
    resourcePath = QDir::currentPath() + "/backend";
#endif

    qDebug() << "[NativeBridge] Root Resource Path:" << resourcePath;

    // Inyectar el path raíz al Core C++
    interlock::InterlockCore::setBasePath(resourcePath.toUtf8().constData());
    interlock::InterlockCore::setEventCallback(NativeBridge::onNativeEvent);
    interlock::InterlockCore::boot();

    m_isBooted = true;
    m_metricsTimer->start(1000);
    emit bootStatusChanged();
}

void NativeBridge::createOperation(const QString &name, const QString &preset) {
    interlock::InterlockCore::createOperation(name.toUtf8().constData(), preset.toUtf8().constData());
}

void NativeBridge::joinOperation(const QString &opId) {
    interlock::InterlockCore::joinOperation(opId.toUtf8().constData());
}

void NativeBridge::executeAction(const QString &action) {
    qDebug() << "[NativeBridge] Action Triggered:" << action;
    if (action == "ADVANCE_PHASE") {
        interlock::InterlockCore::advancePhase();
    }
}

void NativeBridge::injectMessage(const QString &to, const QString &content) {
    interlock::InterlockCore::injectMessage(to.toUtf8().constData(), content.toUtf8().constData());
}

void NativeBridge::selectModel(const QString &modelId) {
    interlock::InterlockCore::selectModel(modelId.toUtf8().constData());
}

QString NativeBridge::processInference(const QString &prompt) {
    std::string result = interlock::InterlockCore::searchIntelligence(prompt.toUtf8().constData());
    return QString::fromStdString(result);
}

QString NativeBridge::deployPayload(uint32_t seed) {
    std::string artifact = interlock::InterlockCore::triggerZigPayload(seed);
    return QString::fromStdString(artifact);
}

QVariantList NativeBridge::simulateSignalPropagation(const QVariantList &transmitters, const QVariantList &obstacles) {
    QJsonArray t_array;
    for (const QVariant &v : transmitters) {
        QVariantMap m = v.toMap();
        QJsonObject obj;
        obj["x"] = m["x"].toDouble();
        obj["y"] = m["y"].toDouble();
        obj["z"] = m["z"].toDouble();
        t_array.append(obj);
    }

    QJsonArray o_array;
    for (const QVariant &v : obstacles) {
        QVariantMap m = v.toMap();
        QJsonObject obj;
        obj["x"] = m["x"].toDouble();
        obj["y"] = m["y"].toDouble();
        obj["z"] = m["z"].toDouble();
        o_array.append(obj);
    }

    std::string t_json = QJsonDocument(t_array).toJson(QJsonDocument::Compact).toStdString();
    std::string o_json = QJsonDocument(o_array).toJson(QJsonDocument::Compact).toStdString();

    std::string result_json = interlock::InterlockCore::simulateSignalPropagation(t_json, o_json);
    QJsonDocument res_doc = QJsonDocument::fromJson(QByteArray::fromStdString(result_json));

    if (res_doc.isArray()) {
        return res_doc.array().toVariantList();
    }
    return QVariantList();
}

bool NativeBridge::distributeTicket(uint32_t type, const QString &base64Data, const QVariantList &pids) {
    QStringList pidStrings;
    for (const QVariant &v : pids) {
        pidStrings << v.toString();
    }
    QString pidsCsv = pidStrings.join(",");

    return interlock::InterlockCore::distributeTicket(
        type,
        base64Data.toUtf8().constData(),
        pidsCsv.toUtf8().constData()
    );
}

QVariantMap NativeBridge::compileChrx(const QString &source) {
    std::string result = interlock::InterlockCore::compileChrx(source.toUtf8().constData());
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(result));
    if (doc.isObject()) return doc.object().toVariantMap();
    return QVariantMap{{"success", false}, {"error", "Invalid compiler response"}};
}

bool NativeBridge::validateChoreography(const QString &chrxJson) {
    return interlock::InterlockCore::validateChoreography(chrxJson.toUtf8().constData());
}

void NativeBridge::updateMetrics() {
    // 1. Snapshot Sync (Estado de la misión)
    std::string snapshotJson = interlock::InterlockCore::getSnapshotJSON();
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(snapshotJson));
    if (!doc.isNull() && doc.isObject()) {
        QVariantMap newSnapshot = doc.object().toVariantMap();
        if (m_snapshot != newSnapshot) {
            m_snapshot = newSnapshot;
            emit snapshotUpdated();
        }
    }

    // 2. Hardware Telemetry Sync
    std::string hardwareJson = interlock::InterlockCore::getHardwareMetricsJSON();
    QJsonDocument hwDoc = QJsonDocument::fromJson(QByteArray::fromStdString(hardwareJson));
    if (!hwDoc.isNull() && hwDoc.isArray()) {
        QVariantList newHw = hwDoc.array().toVariantList();
        if (m_hardwareNodes != newHw) {
            m_hardwareNodes = newHw;
            emit hardwareUpdated();
        }
    }

    // 3. Network Stats Sync
    interlock::NetworkStats ns = interlock::NetworkBridge::getInstance().getStats();
    QVariantMap newNetwork;
    newNetwork["bytesSent"] = (qulonglong)ns.bytesSent;
    newNetwork["bytesReceived"] = (qulonglong)ns.bytesReceived;
    newNetwork["activeConnections"] = ns.activeConnections;

    if (m_networkStats != newNetwork) {
        m_networkStats = newNetwork;
        emit networkUpdated();
    }
}

void NativeBridge::handleIncomingEvent(const QString &json) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QVariantMap eventData = doc.object().toVariantMap();

    // Mantener un log rotativo de los últimos 100 eventos para la UI
    m_eventLog.prepend(eventData);
    if (m_eventLog.size() > 100) {
        m_eventLog.removeLast();
    }

    emit eventLogChanged();
}

void NativeBridge::onNativeEvent(const char* jsonEvent) {
    if (s_instance) {
        // IMPORTANTE: Este callback viene de un thread de C++,
        // emitimos la señal para cruzar al thread principal de Qt (GUI).
        emit s_instance->newRawEvent(QString::fromUtf8(jsonEvent));
    }
}
