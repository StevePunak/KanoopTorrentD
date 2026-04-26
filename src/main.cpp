#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTextStream>

#include <csignal>
#include <unistd.h>

#include <Kanoop/log.h>
#include <Kanoop/timespan.h>

#include "kanooptorrentdaemon.h"
#include "settings.h"

const QString keyDebugMode  = "debug";
const QString keyHelp       = "help";
const QString keyVerbose    = "verbose";

// Self-pipe so the async-signal-safe SIGINT/SIGTERM handler can wake the
// Qt event loop. The handler can only do write(); a QSocketNotifier on the
// read end translates that into a regular slot that can call quit().
static int g_signalFd[2] = {-1, -1};

static void signalHandler(int)
{
    char byte = 1;
    ssize_t r = ::write(g_signalFd[1], &byte, 1);
    (void)r;
}

static bool installSignalNotifier(QObject* parent)
{
    if(::pipe(g_signalFd) != 0) {
        return false;
    }

    auto* notifier = new QSocketNotifier(g_signalFd[0], QSocketNotifier::Read, parent);
    QObject::connect(notifier, &QSocketNotifier::activated, parent, [](QSocketDescriptor fd) {
        char byte;
        ssize_t r = ::read(static_cast<int>(fd), &byte, 1);
        (void)r;
        Log::logText(LVL_NOTICE, "Signal received — shutting down");
        QCoreApplication::quit();
    });

    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    return true;
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName("kanooptorrentd");
    QCoreApplication::setOrganizationName("Kanoop");
    QCoreApplication::setApplicationVersion("99.99.99");

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);

    QString logFilePath = QDir(dataDir).filePath("kanooptorrentd.log");
    Log::setFilename(logFilePath);
    Log::enableOutputFlags(static_cast<Log::OutputFlags>(Log::Standard | Log::File));

    QCommandLineParser parser;
    parser.setApplicationDescription("Kanoop headless torrent daemon");
    parser.addOptions({
        {{ "d", keyDebugMode }, "Debug mode"},
        {{ "v", keyVerbose },   "Verbose output"},
        {{ "?", keyHelp },      "Print usage and exit"},
    });
    parser.process(application);

    if(parser.isSet(keyHelp)) {
        parser.showHelp(0);
    }

    if(parser.isSet(keyVerbose) || parser.isSet(keyDebugMode)) {
        Log::setLevel(Log::Debug);
    }

    Log::logText(LVL_INFO, QString("kanooptorrentd starting in %1").arg(QDir::currentPath()));

    Settings::instance();

    if(!installSignalNotifier(&application)) {
        QTextStream(stderr) << "Failed to install signal handlers" << Qt::endl;
        return 1;
    }

    KanoopTorrentDaemon daemon;
    daemon.start();

    int rc = application.exec();
    daemon.stop(TimeSpan::fromSeconds(5));
    return rc;
}
