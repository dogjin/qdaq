#ifndef QDAQFOPDT_H
#define QDAQFOPDT_H

#include "filters_global.h"

#include "QDaqFilterPlugin.h"
#include "QDaqJob.h"
#include "QDaqTypes.h"

class FILTERSSHARED_EXPORT QDaqFOPDT :
        public QDaqJob,
        public QDaqFilterPlugin
{
    Q_OBJECT
    Q_INTERFACES(QDaqFilterPlugin)

    Q_PROPERTY(double kp READ kp WRITE setKp)
    Q_PROPERTY(uint tp READ tp WRITE setTp)
    Q_PROPERTY(uint td READ td WRITE setTd)


    double kp_;
    uint tp_, td_;
    double y_;


    double h_;

    QDaqVector ubuff;
    uint ibuff;

public:
    QDaqFOPDT();

    // getters
    double kp() const { return kp_; }
    uint tp() const { return tp_; }
    uint td() const { return td_; }

    // setters
    void setKp(double k);
    void setTp(uint t);
    void setTd(uint t);

    // QDaqFilterPlugin interface implementation
    virtual QString iid()
    { return QString("fopdt-v0.1"); }
    virtual QString errorMsg() { return QString(); }
    virtual bool init();
    virtual bool operator()(const double* vin, double* vout);
    virtual int nInputChannels() const { return 1; }
    virtual int nOutputChannels() const { return 1; }

};

#endif // QDAQFOPDT_H
