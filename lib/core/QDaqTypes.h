#ifndef _QDAQTYPES_H_
#define _QDAQTYPES_H_

#include "QDaqGlobal.h"

#include "math_util.h"

#include <QMetaType>
#include <QScriptValue>

class QScriptEngine;
class QDaqObject;
class QDaqVector;

struct QDAQ_EXPORT QDaqTypes {
    static int registerWithJS(QScriptEngine* eng);

    static bool isNumeric(const QVariant&);
    static bool isString(const QVariant&);
    static bool isBool(const QVariant&);

    static bool isStringList(const QVariant&);
    static bool isVector(const QVariant&);

    static QStringList toStringList(const QVariant&);
    static QDaqVector toVector(const QVariant&);

    static bool isNumeric(const QVariantList&);
    static bool isString(const QVariantList&);
    static bool isBool(const QVariantList&);

    static QScriptValue toScriptValue(QScriptEngine *eng, QDaqObject* const  &obj, int ownership = 2);
    static QScriptValue toScriptValue(QScriptEngine *eng, const QScriptValue& scriptObj, QDaqObject* const &obj, int ownership = 2);
    static void fromScriptValue(const QScriptValue &value, QDaqObject* &obj);
};

#include <QString>
#include <QDateTime>

/** Time representation in QDaq.

  @ingroup Core
  @ingroup Types

  Time is represented internaly as a double number
  corresponding to seconds since 1 Jan 1970
  as returned by the POSIX ftime function.
  It is recorded with millisecond resolution.
  */

class QDAQ_EXPORT QDaqTimeValue
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


/**
 * @brief A buffer for storing double numbers.
 * @ingroup Types
 *
 * It is used for storing data from QDaqChannel objects.
 *
 * The buffer has 3 modes, according to its StorageType property:
 * it can be circular, i.e., new data overwrite old data, it can have fixed
 * size or it can be expandable.
 *
 * Data are inserted at the end of the buffer by the function push() or
 * the operator<<(). The contents can be read by the function get() or the
 * operator[](). The class provides read-only access to the data. It is not
 * possible to change the value of a stored element.
 *
 * The class defines functions for getting the min/max value,
 * the mean and std deviation.
 * The calculation of these quantities is stored internally and multiple
 * calls to these functions do not cause a recalculation (except if
 * the buffer contents have changed).
 *
 * The buffer is explicitly shared, i.e., multiple instances share
 * the same underlying data. This is used primarily for displaying
 * real-time plots of data without copying the buffer.
 *
 */
class QDAQ_EXPORT QDaqVector
{
    typedef math::buffer<double> buffer_t;
    QExplicitlySharedDataPointer<buffer_t> d_ptr;
public:
    /// Create a buffer with n elements, initially filled with 0.
    explicit QDaqVector(int n = 0) : d_ptr(new buffer_t(n))
    {
        for(int i=0; i<n; i++) push(0.);
    }
    QDaqVector(const QDaqVector& other) : d_ptr(other.d_ptr)
    {}
    QDaqVector& operator=(const QDaqVector& rhs)
    {
        d_ptr = rhs.d_ptr;
        return (*this);
    }
    QDaqVector clone() const
    {
        QDaqVector V(*this);
        V.d_ptr.detach();
        return V;
    }
    /// Return the number of elememts stored in the buffer.
    int size() const { return d_ptr->size(); }
    /// set the size
    void setSize(int n) { d_ptr->setSize(n); }
    void resize(int n) { d_ptr->setSize(n); }
    /// Return true if Circular
    bool isCircular() const { return d_ptr->isCircular(); }
    /// Set circular on or off
    void setCircular(bool on) { d_ptr->setCircular(on); }
    /// Return the currently allocated memory capacity (in number of elements).
    int capacity() const { return d_ptr->capacity(); }
    /// Set the capacity
    void setCapacity(int c) { d_ptr->setCapacity(c); }
    /// Empty the buffer.
    void clear() { d_ptr->clear(); }
    /// Get the i-th element
    double get(int i) const { return d_ptr->get(i); }
    /// Return the i-th element
    double operator[](int i) const { return d_ptr->get(i); }
    /// Return the i-th element
    double& operator[](int i) { return (*d_ptr)[i]; }
    /// Append a value to the buffer.
    /// Pushing to a circular vector of 0 capacity 0 leads to an error.
    void push(double v) { d_ptr->push(v); }
    /// Append n values stored in memory location v to the buffer
    void push(const double* v, int n) { d_ptr->push(v, n); }
    /// Append another vector
    void push(const QDaqVector& v) { d_ptr->push(v.constData(), v.size()); }
    /// Remove the last point
    void pop() { d_ptr->pop(); }
    /// Append a value to the buffer
    QDaqVector& operator<<(const double& v) { d_ptr->push(v); return (*this); }
    /// Append another vector
    QDaqVector& operator<<(const QDaqVector& v) { push(v); return (*this); }
    /// Return a const pointer to the data.
    const double* constData() const { return d_ptr->constData(); }
    /// Return a pointer to the data.
    double* data() { return d_ptr->data(); }
    /// Minimum value in the buffer.
    double vmin() const { return d_ptr->vmin(); }
    /// Maximum value in the buffer.
    double vmax() const { return d_ptr->vmax(); }
    /// Mean value in the buffer.
    double mean() const { return d_ptr->mean(); }
    /// Standard deviation the buffer values.
    double std() const { return d_ptr->std(); }

    bool operator ==(const QDaqVector& other) const
    {
        return *d_ptr == *(other.d_ptr);
    }
    bool operator !=(const QDaqVector& other) const
    {
        return *d_ptr != *(other.d_ptr);
    }
    bool isEmpty() const
    {
        return d_ptr->size()==0;
    }
};

Q_DECLARE_METATYPE(QDaqVector)
//Q_DECLARE_METATYPE(QDaqVector*)
//Q_DECLARE_METATYPE(QByteArray*)

#endif

