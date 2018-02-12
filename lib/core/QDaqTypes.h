#ifndef _QDAQTYPES_H_
#define _QDAQTYPES_H_

#include "QDaqGlobal.h"

#include <QVector>
#include <QExplicitlySharedDataPointer>
#include <QMetaType>

typedef QVector<int> QDaqIntVector;
typedef QVector<unsigned int> QDaqUintVector;
typedef QVector<double> QDaqVector;

Q_DECLARE_METATYPE(QDaqIntVector)
Q_DECLARE_METATYPE(QDaqUintVector)
Q_DECLARE_METATYPE(QDaqVector)

class QScriptEngine;

int registerVectorTypes(QScriptEngine* eng);

#include <QString>
#include <QDateTime>

/** Time representation in RtLab.

  \ingroup RtCore

  Time is represented internaly as a double number
  corresponding to seconds since 1 Jan 1970
  as returned by the POSIX ftime function.
  It is recorded with millisecond resolution.
  */

class QDaqTimeValue
{
    double v_;

public:
    /// default constructor
    QDaqTimeValue() : v_(0.0)
    {}
    /// construct from double
    explicit QDaqTimeValue(double d) : v_(d)
    {}
    /// copy constructor
    QDaqTimeValue(const QDaqTimeValue& rhs) : v_(rhs.v_)
    {}

    /// copy operator
    QDaqTimeValue& operator=(const QDaqTimeValue& rhs)
    {
        v_ = rhs.v_;
        return *this;
    }

    /// type conversion operator to double
    operator double() { return v_; }

    /// convert to QDateTime
    operator QDateTime()
    {
        return QDateTime::fromMSecsSinceEpoch(1000*v_);
    }

    /// return current time in QDaqTimeValue format
    static QDaqTimeValue now()
    {
        return QDaqTimeValue(0.001*(QDateTime::currentMSecsSinceEpoch()));
    }

    /// convert to string
    QString toString()
    {
        QTime T = QDateTime(*this).time();
        return T.toString("hh:mm:ss.zzz");
    }
};

template<class T>
class buffer : public QSharedData
{
public:
    enum StorageType { Open, Fixed, Circular };

    typedef QVector<T> container_t;

private:
    typedef buffer<T> _Self;

    /// memory buffer
    container_t mem;
    /// vector size
    int sz;
    /// vector capacity
    int cp;
    /// vector type
    StorageType type_;
    /// pointer to next position for circular vectors
    int tail;
    /// min & max values
    T x1, x2;
    /// flag set if min & max need recalc
    bool recalcBounds;

    void normalize_()
    {
        if (type_==Circular && sz && sz==cp && tail)
        {
            int m = sz-tail;
            T* head = mem.data();
            T* temp = head + sz;
            if (tail<=sz/2)
            {
                memcpy(temp,head,tail*sizeof(T));
                memmove(head,head+tail,m*sizeof(T));
                memcpy(head + m,temp,tail*sizeof(T));
            }
            else
            {
                memcpy(temp,head+tail,m*sizeof(T));
                memmove(head+m,head,tail*sizeof(T));
                memcpy(head,temp,m*sizeof(T));
            }
            tail = 0;
        }
    }

    int idx_(int i) const
    {
        return (type_==Circular && sz==cp) ? ((tail+i) % sz) : i;
    }

    void set_(int i, const T& v)
    {
        mem[i] = v;
    }

    void calcBounds_()
    {
        int n(size());
        if (n>0)
        {
            x1 = x2 = get(0);
            for(int i=1; i<n; ++i)
            {
                double v = get(i);
                if (v<x1) x1 = v;
                if (v>x2) x2 = v;
            }
        }
        else x1 = x2 = 0.;
        recalcBounds = false;
    }

public:
    explicit buffer(int acap = 0) : mem((int)acap),
        sz(0), cp(acap), type_(Fixed), tail(0),
        x1(0), x2(0), recalcBounds(true)
    {
    }
    buffer(const _Self& rhs) : mem(rhs.mem),
        sz(rhs.sz), cp(rhs.cp), type_(rhs.type_), tail(rhs.tail),
        x1(rhs.x1), x2(rhs.x2), recalcBounds(rhs.recalcBounds)
    {
    }
    ~buffer(void)
    {
    }

    _Self& operator=(const _Self& rhs)
    {
        mem = rhs.mem;
        sz = rhs.sz;
        cp = rhs.cp;
        type_ = rhs.type_;
        tail = rhs.tail;
        x1 = rhs.x1;
        x2 = rhs.x2;
        recalcBounds = rhs.recalcBounds;
        return (*this);
    }

    int size() const { return (int)sz; }

    StorageType type() const { return type_; }
    void setType(StorageType newt)
    {
        if (newt==type_) return;

        normalize_();

        if (newt==Circular)
            mem.resize(cp + cp/2);
        if (type_==Circular)
            mem.resize(cp);

        type_ = newt;
    }

    int capacity() const { return (int)cp; }

    void setCapacity(int c)
    {
        if (c==cp) return;

        normalize_();

        switch (type_)
        {
        case Fixed:
        case Open:
            mem.resize(c);
            if (sz>c) sz = c;
            recalcBounds = true;
            break;
        case Circular:
            mem.resize(c + c/2);
            if (c>cp)
            {
                if (sz==cp) tail = sz;
            }
            else // (c<cp)
            {
                if (sz>c)
                {
                    sz = c; tail = 0;
                    recalcBounds = true;
                }
            }
            break;
        }
        cp = c;
    }
    void clear()
    {
        sz = 0;
        tail = 0;
        recalcBounds = true;
    }
    void replace(const container_t& other)
    {
        clear();
        mem = other;
        sz = mem.size();
        cp = mem.capacity();
        if (type_==Circular) mem.resize(cp  + cp/2);
        tail = 0;
    }
    const T& get(int i) const
    {
        return mem[idx_(i)];
    }
    const T& operator[](int i) const
    {
        return get(i);
    }
    void push(const T& v)
    {
        switch (type_)
        {
        case Open:
            if (sz==cp) mem.resize(++cp);
            set_(sz++,v);
            break;
        case Fixed:
            if (sz<cp) set_(sz++,v);
            break;
        case Circular:
            set_(tail++,v);
            if (sz<cp) sz++;
            if (sz==cp) tail %= sz;
            break;
        }
        recalcBounds = true;
    }
    void push(const T* v, int n)
    {
        int m;
        switch (type_)
        {
        case Open:
            if (sz+n>cp) { cp = sz+n; mem.resize(cp); }
            memcpy(mem.data()+sz,v,n*sizeof(T));
            sz += n;
            break;
        case Fixed:
            m = n;
            if (sz+n>cp) m = cp - sz;
            if (m) {
                memcpy(mem.data()+sz,v,m*sizeof(T));
                sz += m;
            }
            break;
        case Circular:
            if (n>=cp) {
                memcpy(mem.data(),v+n-cp,cp*sizeof(T));
                tail = 0;
                sz = cp;
            }
            else if (n<=cp-tail) {
                memcpy(mem.data()+tail,v,n*sizeof(T));
                tail += n;
                if (sz<cp) sz += n;
                if (sz==cp) tail %= cp;
            } else {
                m = cp-tail;
                memcpy(mem.data()+tail,v,m*sizeof(T));
                v += m; n -= m;
                memcpy(mem.data(),v,n*sizeof(T));
                tail = n;
                sz = cp;
            }
            break;
        }
        recalcBounds = true;
    }
    _Self& operator<<(const T& v)
    {
        push(v); return (*this);
    }
    const T* constData() const
    {
        const_cast< _Self * >( this )->normalize_();
        return mem.constData();
    }
    container_t vector() const
    {
        const_cast< _Self * >( this )->normalize_();
        return mem.mid(0,sz);
    }
    double vmin() const
    {
        if (recalcBounds)
            const_cast< _Self * >( this )->calcBounds_();
        return x1;
    }
    double vmax() const
    {
        if (recalcBounds)
            const_cast< _Self * >( this )->calcBounds_();
        return x2;
    }
    double mean() const
    {
        double s(0.0);
        int n(size());
        for(int i=0; i<n; ++i) s += get(i);
        return s/n;
    }
    double std() const
    {
        double s1(0.0), s2(0.0);
        int n(size());
        for(int i=0; i<n; ++i) {
            double v = get(i);
            s1 += v; s2 += v*v;
        }
        s1 /= n;
        s2 /= n;
        s1 = s2 - s1*s1;
        if (s1<=0.0) return 0.0;
        else return sqrt(s1);
    }
};

class QDaqBuffer
{
    typedef buffer<double> buffer_t;
    QExplicitlySharedDataPointer<buffer_t> d_ptr;
public:
    enum StorageType {
        Open = buffer_t::Open, Fixed, Circular
    };

    explicit QDaqBuffer(int cap = 0)
    {
        d_ptr = new buffer_t(cap);
    }
    QDaqBuffer(const QDaqBuffer& other) : d_ptr(other.d_ptr)
    {}
    QDaqBuffer& operator=(const QDaqBuffer& rhs)
    {
        d_ptr = rhs.d_ptr;
        return (*this);
    }
    int size() const { return d_ptr->size(); }
    StorageType type() const { return (StorageType)(d_ptr->type()); }
    void setType(StorageType newt) {
        d_ptr->setType((buffer_t::StorageType)newt);
    }
    int capacity() const { return d_ptr->capacity(); }
    void setCapacity(int c) { d_ptr->setCapacity(c); }
    void clear() { d_ptr->clear(); }
    void replace(const QDaqVector& v) { d_ptr->replace(v); }
    double get(int i) const { return d_ptr->get(i); }
    double operator[](int i) const { return d_ptr->get(i); }
    void push(double v) { d_ptr->push(v); }
    void push(const double* v, int n) { d_ptr->push(v, n); }
    QDaqBuffer& operator<<(const double& v)
    {
        d_ptr->push(v); return (*this);
    }
    const double* constData() const { return d_ptr->constData(); }
    QDaqVector toVector() const { return d_ptr->vector(); }
    double vmin() const { return d_ptr->vmin(); }
    double vmax() const { return d_ptr->vmax(); }
    double mean() const { return d_ptr->mean(); }
    double std() const { return d_ptr->std(); }
};

Q_DECLARE_METATYPE(QDaqBuffer)

#endif
