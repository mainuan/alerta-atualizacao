#include "update_checker.h"

#include <QLoggingCategory>
#include <QProcessEnvironment>

Q_LOGGING_CATEGORY(logUpdateChecker, "system-upgrade.checker")

namespace {
constexpr int aptUpdateTimeoutMilliseconds = 300'000;
constexpr int queryTimeoutMilliseconds = 60'000;

std::size_t commandIndex(UpdateChecker::Step step)
{
    return static_cast<std::size_t>(step) - static_cast<std::size_t>(UpdateChecker::Step::AptUpdate);
}

bool isCommandStep(UpdateChecker::Step step)
{
    return step == UpdateChecker::Step::AptUpdate || step == UpdateChecker::Step::AptQuery
        || step == UpdateChecker::Step::FlatpakSystem || step == UpdateChecker::Step::FlatpakUser;
}
}

UpdateCommand UpdateChecker::defaultCommand(Step step)
{
    switch (step) {
    case Step::AptUpdate:
        return {QStringLiteral("/usr/bin/sudo"),
                {QStringLiteral("-n"),
                 QStringLiteral("/usr/bin/apt-get"),
                 QStringLiteral("-o"),
                 QStringLiteral("Acquire::Retries=3"),
                 QStringLiteral("-o"),
                 QStringLiteral("APT::Update::Error-Mode=any"),
                 QStringLiteral("update")},
                aptUpdateTimeoutMilliseconds};
    case Step::AptQuery:
        return {QStringLiteral("/usr/bin/apt-get"),
                {QStringLiteral("-s"),
                 QStringLiteral("-o"),
                 QStringLiteral("Debug::NoLocking=1"),
                 QStringLiteral("upgrade")},
                queryTimeoutMilliseconds};
    case Step::FlatpakSystem:
        return {QStringLiteral("/usr/bin/flatpak"),
                {QStringLiteral("--system"),
                 QStringLiteral("remote-ls"),
                 QStringLiteral("--updates"),
                 QStringLiteral("--columns=ref")},
                queryTimeoutMilliseconds};
    case Step::FlatpakUser:
        return {QStringLiteral("/usr/bin/flatpak"),
                {QStringLiteral("--user"),
                 QStringLiteral("remote-ls"),
                 QStringLiteral("--updates"),
                 QStringLiteral("--columns=ref")},
                queryTimeoutMilliseconds};
    case Step::Idle:
    case Step::Complete:
        break;
    }
    return {};
}

void UpdateChecker::setCommand(Step step, const UpdateCommand &command)
{
    if (isCommandStep(step)) {
        m_commands[commandIndex(step)] = command;
    }
}

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_commands{defaultCommand(Step::AptUpdate),
                 defaultCommand(Step::AptQuery),
                 defaultCommand(Step::FlatpakSystem),
                 defaultCommand(Step::FlatpakUser)}
{
    m_timeout.setSingleShot(true);

    connect(&m_process,
            &QProcess::started,
            this,
            [this] { m_started = true; });
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (m_stepCompleted) {
                    return;
                }
                m_timeout.stop();
                m_stepCompleted = true;
                const bool succeeded = m_started && exitStatus == QProcess::NormalExit
                    && exitCode == 0;
                completeStep(succeeded,
                             QString::fromLocal8Bit(m_process.readAllStandardOutput()),
                             QString::fromLocal8Bit(m_process.readAllStandardError()));
            });
    connect(&m_process,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error != QProcess::FailedToStart || m_stepCompleted) {
                    return;
                }
                m_timeout.stop();
                m_stepCompleted = true;
                completeStep(false, {}, m_process.errorString());
            });
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (m_stepCompleted) {
            return;
        }
        qCWarning(logUpdateChecker) << "Update command timed out:" << m_process.program();
        m_process.kill();
    });
}

void UpdateChecker::start()
{
    if (m_step != Step::Idle) {
        return;
    }
    m_step = Step::AptUpdate;
    runNextStep();
}

void UpdateChecker::runCommand(Step step)
{
    const UpdateCommand &command = m_commands[commandIndex(step)];
    m_step = step;
    m_started = false;
    m_stepCompleted = false;

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(command.program);
    m_process.setArguments(command.arguments);
    m_timeout.start(command.timeoutMilliseconds);
    m_process.start();
}

void UpdateChecker::completeStep(bool commandSucceeded,
                                 const QString &standardOutput,
                                 const QString &standardError)
{
    if (!commandSucceeded) {
        qCWarning(logUpdateChecker).noquote()
            << "Update command failed:" << m_process.program()
            << standardError.simplified();
    }

    switch (m_step) {
    case Step::AptUpdate:
        m_state.aptUpdateSucceeded = commandSucceeded;
        m_state.aptHasUpdates = false;
        // A failed refresh (no network yet at login, one broken repository with
        // Error-Mode=any) must not hide updates the local lists already know.
        m_step = Step::AptQuery;
        break;
    case Step::AptQuery:
        m_state.aptQuerySucceeded = commandSucceeded;
        m_state.aptHasUpdates = commandSucceeded && aptSimulationHasUpdates(standardOutput);
        m_step = Step::FlatpakSystem;
        break;
    case Step::FlatpakSystem:
        m_state.flatpakSystemQuerySucceeded = commandSucceeded;
        m_state.flatpakSystemHasUpdates = commandSucceeded
            && flatpakOutputHasUpdates(standardOutput);
        m_step = Step::FlatpakUser;
        break;
    case Step::FlatpakUser:
        m_state.flatpakUserQuerySucceeded = commandSucceeded;
        m_state.flatpakUserHasUpdates = commandSucceeded
            && flatpakOutputHasUpdates(standardOutput);
        m_step = Step::Complete;
        break;
    case Step::Idle:
    case Step::Complete:
        return;
    }

    scheduleNextStep();
}

void UpdateChecker::scheduleNextStep()
{
    // completeStep() runs inside QProcess signal handlers. Starting the same
    // QProcess again from errorOccurred(FailedToStart) is undone by QProcess
    // when the handler returns: the next step would never finish and the
    // checker would hang. Start the next command from the event loop instead.
    QMetaObject::invokeMethod(this, &UpdateChecker::runNextStep, Qt::QueuedConnection);
}

void UpdateChecker::runNextStep()
{
    switch (m_step) {
    case Step::AptUpdate:
    case Step::AptQuery:
    case Step::FlatpakSystem:
    case Step::FlatpakUser:
        runCommand(m_step);
        break;
    case Step::Complete:
        emit finished(m_state);
        break;
    case Step::Idle:
        break;
    }
}
