///**************************************************************************
//* @File: MetaTypeInfo.cpp
//* @Description:  元类型信息类
//* @Copyright: Copyright (c) 2017
//* @Company: 深圳置辰海信科技有限公司
//* @WebSite: http://www.szcenterstar.com/
//* @author 李鹭
//* @Revision History
//*
//* <pre>
//* ----------------------------------------------------------------------
//*   Ver     Date       Who             Comments
//*  ----- ----------  --------  ---------------------------------------
//*   1.0  2017/04/01    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/
#include <algorithm>
#include <cstring>		// for tolower
#include <string>
#include <vector>

#include <QLoggingCategory>

#include "Threading.h"
#include "Exception.h"

#include "Header.h"
#include "MetaTypeInfo.h"

using namespace ZCHX::Messages;

/** Internal class that provides monotonically increasing sequence numbers for messages. There is a separate
    generator for each thread that requests a sequence number so that threads will not generate sequences with
    gaps in them.
*/
class MetaTypeInfo::SequenceGenerator
{
public:
    class PerThreadInfo;

    /** Constructor. Creates a unique pthread_key_t for a MetaTypeInfo object.
     */
    SequenceGenerator();

    /** Destructor. Destroys the unique pthread_key_t allocated by the constructor.
     */
    ~SequenceGenerator();

    /** Access the thread-specific sequence number generator and return the next sequence value from it.

        \return next sequence number
    */
    SequenceType getNextSequenceNumber();

private:
    pthread_key_t perThreadInfoKey_;
};

/** Internal message sequence counter that exists for each thread.
 */
struct MetaTypeInfo::SequenceGenerator::PerThreadInfo
{
    /** Constructor. Initialize the sequence counter to 0
     */
    PerThreadInfo() : sequenceCounter_(0) {}

    /** Obtain the next sequence counter value.

        \return next sequence counter
    */
    SequenceType getNextSequenceNumber() { return ++sequenceCounter_; }

    /** Thread-specific sequence counter
     */
    SequenceType sequenceCounter_;
};

extern "C" {

    /** Stub C function that deletes the PerThreadInfo object associated with a thread that is terminating.

        \param obj the PerThreadInfo object to delete
    */
    static void MetaTypeInfoDestroyPerThreadInfoStub(void* obj)
    {
    using PerThreadInfo = MetaTypeInfo::SequenceGenerator::PerThreadInfo;
    delete static_cast<PerThreadInfo*>(obj);
    }
}

MetaTypeInfo::SequenceGenerator::SequenceGenerator()
{
    qCDebug(radarmsg) << "SequenceGenerator" ;
    int rc = ::pthread_key_create(&perThreadInfoKey_, &MetaTypeInfoDestroyPerThreadInfoStub);
    if (rc)
    {
        qCWarning(radarmsg) << "*** failed pthread_key_create: " << rc << " - " << ::strerror(rc);
    }
}

MetaTypeInfo::SequenceGenerator::~SequenceGenerator()
{
    qCDebug(radarmsg) << "~SequenceGenerator" ;
    int rc = ::pthread_key_delete(perThreadInfoKey_);
    if (rc)
    {
        qCWarning(radarmsg) << "*** failed pthread_key_delete: " << rc << " - " << ::strerror(rc);
    }
}

MetaTypeInfo::SequenceType
MetaTypeInfo::SequenceGenerator::getNextSequenceNumber()
{
    qCDebug(radarmsg) << "getNextSequenceNumber" ;
    PerThreadInfo* pti = static_cast<PerThreadInfo*>(::pthread_getspecific(perThreadInfoKey_));
    if (! pti) {
        qCDebug(radarmsg) << "created new PerThreadInfo object" ;
       pti = new PerThreadInfo();
       ::pthread_setspecific(perThreadInfoKey_, pti);
    }

    return pti->getNextSequenceNumber();
}

/** Internal class that mananges MetaTypeInfo registrations.
 */
struct MetaTypeInfo::Registrations
{
    using KeyVector = std::vector<int>;
    using MetaTypeInfoVector = std::vector<MetaTypeInfo*>;
    KeyVector keys_;
    MetaTypeInfoVector infos_;

    Registrations() : keys_(), infos_() {}

    void add(int, MetaTypeInfo*);
    void remove(int);
    void dump() const;
    const MetaTypeInfo* find(int key) const;
    const MetaTypeInfo* find(const std::string& name) const;
};


void
MetaTypeInfo::Registrations::add(int key, MetaTypeInfo* info)
{
    qCDebug(radarmsg) << "add " << keys_.size() << ' ' << infos_.size();

    if (keys_.empty() || key > keys_.back()) {
        qCDebug(radarmsg) << "adding to back";
        keys_.push_back(key);
        infos_.push_back(info);
    }
    else {
    KeyVector::iterator pos(std::upper_bound(keys_.begin(), keys_.end(), key));
    if (pos != keys_.begin() && *(pos - 1) == key) {
        ZchxRadarUtils::Exception ex("duplicate MetaTypeInfo keys - ");
        ex << int(key) << '/' << info->getName() << ' '
           << infos_[pos - keys_.begin() - 1]->getName()
           << ' ' << info << ' '
           << infos_[pos - keys_.begin() - 1];
//	    log.thrower(ex);
    }

    size_t index(pos - keys_.begin());
    qCDebug(radarmsg) << keys_.size() << ' ' << index ;
    keys_.insert(pos, key);
    infos_.insert(infos_.begin() + index, info);
    }

    dump();
}

void
MetaTypeInfo::Registrations::dump() const
{
    qCDebug(radarmsg) << "dump" ;
    MetaTypeInfoVector::const_iterator pos(infos_.begin());
    MetaTypeInfoVector::const_iterator end(infos_.end());
    while (pos != end) {
//       qCDebug(radarmsg)  << (**pos).getKey() << ' ' << (**pos).getName();
    ++pos;
    }
}

void
MetaTypeInfo::Registrations::remove(int key)
{
    qCDebug(radarmsg) << "remove" << keys_.size() << ' ' << infos_.size();

    KeyVector::iterator pos(std::lower_bound(keys_.begin(), keys_.end(), key));
    if (pos == keys_.end()) {
       qCInfo(radarmsg)  << "object is not registered";
       return;
    }

    size_t index(pos - keys_.begin());
    qCDebug(radarmsg) << keys_.size() << ' ' << index;
    keys_.erase(pos);
    infos_.erase(infos_.begin() + index);
    dump();
}

const MetaTypeInfo*
MetaTypeInfo::Registrations::find(int key) const
{
    qCDebug(radarmsg) << "Find" << key;

    KeyVector::const_iterator pos(std::lower_bound(keys_.begin(), keys_.end(),
                                                   key));
    if (pos == keys_.end()) {
    ZchxRadarUtils::Exception ex("no MetaTypeInfo registered for key ");
    ex << int(key);
//	log.thrower(ex);
    }

    return infos_[pos - keys_.begin()];
}

const MetaTypeInfo*
MetaTypeInfo::Registrations::find(const std::string& name) const
{
   qCDebug(radarmsg) << "find(name)" << QString::fromStdString(name) ;

    MetaTypeInfoVector::const_iterator pos(infos_.begin());
    MetaTypeInfoVector::const_iterator end(infos_.end());
    while (pos != end) {
    if ((**pos).getName() == name)
        break;
    ++pos;
    }

    if (pos == end) {
    ZchxRadarUtils::Exception ex("no MetaTypeInfo registered with name ");
    ex << name;
//	log.thrower(ex);
    }

    return *pos;
}


MetaTypeInfo::Registrations* MetaTypeInfo::registrations_ = 0;


MetaTypeInfo::MetaTypeInfo(Value key, const std::string& name, CDRLoader cdrLoader)
    : key_(key), name_(name), cdrLoader_(cdrLoader),
      sequenceGenerator_(new SequenceGenerator)
{

    qCDebug(radarmsg) << "MetaTypeInfo" << static_cast<uint32_t>(key) << ' ' << QString::fromStdString(name)  << ' ';

    lock_guard_type locker(mutex_);
    if (! registrations_) registrations_ = new Registrations;
    registrations_->add(GetValueValue(key_), this);
}

MetaTypeInfo::~MetaTypeInfo()
{
    delete sequenceGenerator_;
}

void
MetaTypeInfo::unregister()
{
    qCDebug(radarmsg) << "unregister"  << static_cast<uint32_t>(key_) << '/' << QString::fromStdString(name_);
    lock_guard_type locker(mutex_);
    registrations_->remove(GetValueValue(key_));
}

MetaTypeInfo::Value
MetaTypeInfo::getKey() const
{
    return key_;
}

const std::string&
MetaTypeInfo::getName() const
{
    return name_;
}

MetaTypeInfo::SequenceType
MetaTypeInfo::getNextSequenceNumber() const
{
    return sequenceGenerator_->getNextSequenceNumber();
}

MetaTypeInfo::CDRLoader
MetaTypeInfo::getCDRLoader() const
{
    return cdrLoader_;
}

bool
MetaTypeInfo::operator<(const MetaTypeInfo& rhs) const
{
    return key_ < rhs.key_;
}

const MetaTypeInfo*
MetaTypeInfo::Find(Value key)
{
    return registrations_->find(GetValueValue(key));
}

const MetaTypeInfo*
MetaTypeInfo::Find(const std::string& name)
{
    return registrations_->find(name);
}

