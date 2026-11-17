#pragma once

#include "update_state.h"

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

#include <array>

struct UpdateCommand {
    QString program;
    QStringList arguments;
    int timeoutMilliseconds = 0;
};

class UpdateChecker final : public QObject
{
    Q_OBJECT

public:
    enum class Step {
        Idle,
        AptUpdate,
        AptQuery,
        FlatpakSystem,
        FlatpakUser,
        Complete,
    };

    explicit UpdateChecker(QObject *parent = nullptr);

    void start();

    // Replaces the command of one check step (AptUpdate..FlatpakUser); tests
    // use it to run the real state machine without root, APT or Flatpak.
    void setCommand(Step step, const UpdateCommand &command);
    [[nodiscard]] static UpdateCommand defaultCommand(Step step);

signals:
    void finished(const UpdateState &state);

private:

    void runCommand(Step step);
    void completeStep(bool commandSucceeded,
                      const QString &standardOutput,
                      const QString &standardError);
    void runNextStep();
    void scheduleNextStep();

    std::array<UpdateCommand, 4> m_commands;
    QProcess m_process;
    QTimer m_timeout;
    UpdateState m_state;
    Step m_step = Step::Idle;
    bool m_started = false;
    bool m_stepCompleted = false;
};
