#include "QDaqRoot.h"
#include "QDaqJob.h"
#include "QDaqLogFile.h"
#include "QDaqChannel.h"
#include "QDaqDataBuffer.h"
#include "QDaqSession.h"
#include "QDaqIde.h"
#include "QDaqInterface.h"
#include "QDaqDevice.h"
#include "QDaqFilter.h"

#include "qdaqplugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>
#include <QLibraryInfo>
#include <QDebug>

QDaqRoot* QDaqObject::root_;

QDaqRoot::QDaqRoot(void) : QDaqObject("qdaq"), ideWindow_(0)
{
    root_ = this;

    qRegisterMetaType<QDaqError>();

    registerClass(&QDaqObject::staticMetaObject);
    registerClass(&QDaqJob::staticMetaObject);
    registerClass(&QDaqLoop::staticMetaObject);
    registerClass(&QDaqChannel::staticMetaObject);
    registerClass(&QDaqDataBuffer::staticMetaObject);

    // DAQ objects/devices
    registerClass(&QDaqDevice::staticMetaObject);

    pluginManager.loadPlugins();
    object_map_.unite(pluginManager.object_map_);

    // root dir = current dir when app starts
    QDir pwd = QDir::current();
    rootDir_ = pwd.absolutePath();

    // log dir = rootDir/log
    if (!pwd.cd("log")) {
        if (pwd.mkdir("log")) pwd.cd("log");
    }
    logDir_ = pwd.absolutePath();

    // create error log file
    errorLog_ = new QDaqLogFile(false,',',this);
    errorLog_->open(QDaqLogFile::getDecoratedName("error"));

    connect(this,SIGNAL(error(QDaqError)),this,SLOT(onError(QDaqError)));

    rootSession_ = new QDaqSession(this);

}

void jobDisarmHelper(QDaqObjectList lst)
{
    foreach(QDaqObject* obj, lst)
    {
        if (obj->hasChildren()) jobDisarmHelper(obj->children());
        QDaqLoop* j = qobject_cast<QDaqLoop*>(obj);
        if (j) j->disarm();
    }
}

QDaqRoot::~QDaqRoot(void)
{
    jobDisarmHelper(children());
    //foreach(QDaqObject* obj, children()) obj->detach();
    /*{QDaqIDE*
		QDaqObject* rtobj = qobject_cast<QDaqObject*>(obj);
		if (rtobj) 
		{
			rtobj->detach();
			delete rtobj;
		}
    }*/
}

QDaqIDE* QDaqRoot::createIdeWindow()
{
    if (!ideWindow_) {
        ideWindow_ = new QDaqIDE;
    }
    return ideWindow_;
}

QString QDaqRoot::xml()
{
    QString ret;
    return ret;
}

QDaqObject* QDaqRoot::createObject(const QString& name, const QString& className)
{
    QDaqObject* obj = 0;
    object_map_t::iterator it = object_map_.find(className);
    const QMetaObject* metaObj = (it==object_map_.end()) ? 0 : *it;
    if (metaObj)
    {
        obj = (QDaqObject*)(metaObj->newInstance(Q_ARG(QString,name)));
		if (!obj)
			this->throwScriptError("The object could not be created");
    }
    return obj;
}

QStringList QDaqRoot::classNames()
{
    QStringList lst;
    for(object_map_t::iterator it = object_map_.begin(); it!=object_map_.end(); it++)
        lst << it.key();
    return lst;
}

void QDaqRoot::registerClass(const QMetaObject* metaObj)
{
    object_map_[metaObj->className()] = metaObj;
}

QList<const QMetaObject*> QDaqRoot::registeredClasses() const
{
	QList<const QMetaObject*> lst;
	for(object_map_t::const_iterator it = object_map_.begin(); it!=object_map_.end(); it++)
			lst << it.value();
	return lst;
}

void QDaqRoot::onError(const QDaqError &err)
{
    error_queue_.push(err);
    if (errorLog_) *errorLog_ <<
           QString("%1,%2,%3,%4,%5")
               .arg(err.t.toString("dd.MM.yyyy"))
               .arg(err.t.toString("hh:mm:ss.zzz"))
               .arg(err.objectName)
               .arg(err.type).arg(err.descr);
}

void QDaqRoot::addDaqWindow(QWidget* w)
{
    if (!daqWindows_.contains(w)) {
        daqWindows_.push_back(w);
        emit daqWindowsChanged();
    }
}

void QDaqRoot::removeDaqWindow(QWidget* w)
{
    if (daqWindows_.contains(w)) {
        daqWindows_.removeOne(w);
        emit daqWindowsChanged();
    }
}

void QDaqPluginManager::loadFrom(const QString &path)
{
    QDir pluginsDir = QDir(path);
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        qDebug() << "Loading plugin " << loader.fileName();
        QObject* instance = loader.instance();
        if (instance) {
            QDaqPlugin *plugin = qobject_cast<QDaqPlugin*>(instance);
            if (plugin) {
                QList<const QMetaObject *> metaObjects = plugin->pluginClasses();

                foreach (const QMetaObject* metaObj, metaObjects)
                    if (metaObj->inherits(&QDaqObject::staticMetaObject))
                    {
                        object_map_[metaObj->className()] = metaObj;
                        qDebug() << "  " << metaObj->className();
                    }
            }
            else
            {
                qDebug() << "Error: could not cast to QDaqPlugin";
            }
        } else {
            QString errString = loader.errorString();
            qDebug() << errString;
        }
    }
}

void QDaqPluginManager::loadPlugins()
{
    // first look at app path
    QDir pluginsDir = QDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    if (pluginsDir.cd("plugins")) loadFrom(pluginsDir.absolutePath());

    // now look at Qt plugin folder
    pluginsDir = QDir(QLibraryInfo::location(QLibraryInfo::PluginsPath));
    if (pluginsDir.cd("qdaq")) loadFrom(pluginsDir.absolutePath());
}


