#ifndef _RTROOT_H_
#define _RTROOT_H_

#include "QDaqObject.h"

#include <QHash>
#include <QMetaObject>
#include <QStringList>
#include <QWidgetList>

class QDaqLogFile;
class QDaqIDE;
class QDaqSession;

/** The QDaq root object.

All QDaqObject are descendands of QDaqRoot.

QDaqRoot is a static object which can be accessed by QDaqObject::root().

\ingroup QDaq-Core

*/
class RTLAB_BASE_EXPORT QDaqRoot : public QDaqObject
{
	Q_OBJECT

    Q_PROPERTY(QString rootDir READ rootDir)
    Q_PROPERTY(QString logDir READ logDir)

protected:
    QString rootDir_, logDir_;
    QDaqLogFile* errorLog_;
    QDaqIDE* ideWindow_;
    QDaqSession* rootSession_;
    QDaqErrorQueue error_queue_;

public:
    QDaqRoot(void);
    virtual ~QDaqRoot(void);

    QString rootDir() const { return rootDir_; }
    QString logDir() const { return logDir_; }

    QString xml();

    /** Create a QDaqObject with specified name and class.
	  Use appendChild or insertBefore to get the new object in the
	  RtLab framework.
	*/
    QDaqObject* createObject(const QString& name, const QString& className);

    void objectCreation(QDaqObject* obj, bool c);

    void postError(const QDaqError& e) { emit error(e); }

    void addDaqWindow(QWidget* w);
    void removeDaqWindow(QWidget* w);
    QWidgetList daqWindows() const { return daqWindows_; }
    QDaqIDE* ideWindow() { return ideWindow_; }
    QDaqIDE* createIdeWindow();
    QDaqSession* rootSession() { return rootSession_; }
    const QDaqErrorQueue* errorQueue() const { return &error_queue_; }


public slots:

    /// Return the names of registered object classes.
	QStringList classNames();

private slots:
    void onError(const QDaqError& err);

signals:
    void error(const QDaqError& e);
    void daqWindowsChanged();


private:

    QWidgetList daqWindows_;

	/*typedef QScriptValue (*ScriptConstructorFunctionSignature)(QScriptContext *, QScriptEngine *, void *);

	struct MetaObjectData
	{
		const QMetaObject* metaObject;
		ScriptConstructorFunctionSignature constructor;
	};*/

    typedef QHash<QString, const QMetaObject*> object_map_t;

    object_map_t object_map_;

public:
    /**Register a QDaqObject class type.
	  This allows:
		- serialization of objects to h5, ...
		- creation of objects from QtScript with new operator
	  */
	void registerClass(const QMetaObject* metaObj);
	/// Returns list of registered metaObjects
	QList<const QMetaObject *> registeredClasses() const;

};




#endif

