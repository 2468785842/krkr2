#include "MessageQueue.h"
#include "Clock.h"
#include "DemuxPacket.h"
#include <cmath>

NS_KRMOVIE_BEGIN
CDVDMessageQueue::CDVDMessageQueue(const std::string &owner) :
    /*m_hEvent(true),*/ m_owner(owner) {
    m_iDataSize = 0;
    m_bAbortRequest = false;
    m_bInitialized = false;

    m_TimeBack = DVD_NOPTS_VALUE;
    m_TimeFront = DVD_NOPTS_VALUE;
    m_TimeSize = 1.0 / 4.0; /* 4 seconds */
    m_iMaxDataSize = 0;
}

CDVDMessageQueue::~CDVDMessageQueue() {
    // remove all remaining messages
    Flush(CDVDMsg::NONE);
}

void CDVDMessageQueue::Init() {
    m_iDataSize = 0;
    m_bAbortRequest = false;
    m_bInitialized = true;
    m_TimeBack = DVD_NOPTS_VALUE;
    m_TimeFront = DVD_NOPTS_VALUE;
    m_drain = false;
}

void CDVDMessageQueue::Flush(CDVDMsg::Message type) {
    CSingleLock lock(m_section);

    m_messages.remove_if([type](const DVDMessageListItem &item) {
        return type == CDVDMsg::NONE || item.message->IsType(type);
    });

    m_prioMessages.remove_if([type](const DVDMessageListItem &item) {
        return type == CDVDMsg::NONE || item.message->IsType(type);
    });

    if(type == CDVDMsg::DEMUXER_PACKET || type == CDVDMsg::NONE) {
        m_iDataSize = 0;
        m_TimeBack = DVD_NOPTS_VALUE;
        m_TimeFront = DVD_NOPTS_VALUE;
    }
}

void CDVDMessageQueue::Abort() {
    CSingleLock lock(m_section);

    m_bAbortRequest = true;

    // inform waiter for abort action
    m_hEvent.notify_all();
}

void CDVDMessageQueue::End() {
    CSingleLock lock(m_section);

    Flush(CDVDMsg::NONE);

    m_bInitialized = false;
    m_iDataSize = 0;
    m_bAbortRequest = false;
}

MsgQueueReturnCode CDVDMessageQueue::Put(CDVDMsg *pMsg, int priority,
                                         bool front) {
    CSingleLock lock(m_section);

    if(!m_bInitialized) {
        //	CLog::Log(LOGWARNING, "CDVDMessageQueue(%s)::Put
        // MSGQ_NOT_INITIALIZED", m_owner.c_str());
        pMsg->Release();
        return MSGQ_NOT_INITIALIZED;
    }
    if(!pMsg) {
        //	CLog::Log(LOGFATAL, "CDVDMessageQueue(%s)::Put
        // MSGQ_INVALID_MSG", m_owner.c_str());
        return MSGQ_INVALID_MSG;
    }

    if(priority > 0) {
        int prio = priority;
        if(!front)
            prio++;

        auto it = std::find_if(m_prioMessages.begin(), m_prioMessages.end(),
                               [prio](const DVDMessageListItem &item) {
                                   return prio <= item.priority;
                               });
        m_prioMessages.emplace(it, pMsg, priority);
    } else {
        if(front)
            m_messages.emplace_front(pMsg, priority);
        else
            m_messages.emplace_back(pMsg, priority);
    }

    if(pMsg->IsType(CDVDMsg::DEMUXER_PACKET) && priority == 0) {
        DemuxPacket *packet = ((CDVDMsgDemuxerPacket *)pMsg)->GetPacket();
        if(packet) {
            m_iDataSize += packet->iSize;
            if(packet->dts != DVD_NOPTS_VALUE)
                m_TimeFront = packet->dts;
            else if(packet->pts != DVD_NOPTS_VALUE)
                m_TimeFront = packet->pts;

            if(m_TimeBack == DVD_NOPTS_VALUE)
                m_TimeBack = m_TimeFront;
        }
    }

    pMsg->Release();

    // inform waiter for new packet
    m_hEvent.notify_all();

    return MSGQ_OK;
}

MsgQueueReturnCode CDVDMessageQueue::Get(CDVDMsg **pMsg,
                                         unsigned int iTimeoutInMilliSeconds,
                                         int &priority) {
    CSingleLock lock(m_section);

    *pMsg = nullptr;

    int ret = 0;

    if(!m_bInitialized) {
        //	CLog::Log(LOGFATAL, "CDVDMessageQueue(%s)::Get
        // MSGQ_NOT_INITIALIZED", m_owner.c_str());
        return MSGQ_NOT_INITIALIZED;
    }

    while(!m_bAbortRequest) {
        std::list<DVDMessageListItem> &msgs =
            (priority > 0 || !m_prioMessages.empty()) ? m_prioMessages
                                                      : m_messages;

        if(!msgs.empty() && (msgs.back().priority >= priority || m_drain)) {
            DVDMessageListItem &item(msgs.back());
            priority = item.priority;

            if(item.message->IsType(CDVDMsg::DEMUXER_PACKET) &&
               item.priority == 0) {
                DemuxPacket *packet =
                    ((CDVDMsgDemuxerPacket *)item.message)->GetPacket();
                if(packet) {
                    m_iDataSize -= packet->iSize;
                    if(packet->dts != DVD_NOPTS_VALUE)
                        m_TimeBack = packet->dts;
                    else if(packet->pts != DVD_NOPTS_VALUE)
                        m_TimeBack = packet->pts;
                }
            }

            *pMsg = item.message->AddRef();
            msgs.pop_back();

            ret = MSGQ_OK;
            break;
        } else if(!iTimeoutInMilliSeconds) {
            ret = MSGQ_TIMEOUT;
            break;
        } else {
            //			m_hEvent.Reset();
            m_section.unlock();

            // wait for a new message
            std::unique_lock<std::mutex> eventLock(m_mtxEvent);
            if(m_hEvent.wait_for(
                   eventLock,
                   std::chrono::milliseconds(iTimeoutInMilliSeconds)) ==
               std::cv_status::timeout) {
                m_section.lock();
                return MSGQ_TIMEOUT;
            }

            m_section.lock();
        }
    }

    if(m_bAbortRequest)
        return MSGQ_ABORT;

    return (MsgQueueReturnCode)ret;
}

unsigned CDVDMessageQueue::GetPacketCount(CDVDMsg::Message type) {
    CSingleLock lock(m_section);

    if(!m_bInitialized)
        return 0;

    unsigned count = 0;
    for(const auto &item : m_messages) {
        if(item.message->IsType(type))
            count++;
    }
    for(const auto &item : m_prioMessages) {
        if(item.message->IsType(type))
            count++;
    }

    return count;
}

void CDVDMessageQueue::WaitUntilEmpty() {
    {
        CSingleLock lock(m_section);
        m_drain = true;
    }

    //	CLog::Log(LOGNOTICE, "CDVDMessageQueue(%s)::WaitUntilEmpty",
    // m_owner.c_str());
    CDVDMsgGeneralSynchronize *msg =
        new CDVDMsgGeneralSynchronize(40000, SYNCSOURCE_ANY);
    Put(msg->AddRef());
    msg->Wait(m_bAbortRequest, 0);
    msg->Release();

    {
        CSingleLock lock(m_section);
        m_drain = false;
    }
}

int CDVDMessageQueue::GetLevel() {
    CSingleLock lock(m_section);

    if(m_iDataSize > m_iMaxDataSize)
        return 100;
    if(m_iDataSize == 0)
        return 0;

    if(IsDataBased())
        return std::min(100, 100 * m_iDataSize / m_iMaxDataSize);

    int level = std::min(100.0,
                         std::ceil(100.0 * m_TimeSize *
                                   (m_TimeFront - m_TimeBack) / DVD_TIME_BASE));

    // if we added lots of packets with NOPTS, make sure that the
    // queue is not signalled empty
    if(level == 0 && m_iDataSize != 0) {
        //	CLog::Log(LOGDEBUG, "CDVDMessageQueue::GetLevel() - can't
        // determine level");
        return 1;
    }

    return level;
}

void CDVDMessageQueue::SetMaxTimeSize(double sec) {
    m_TimeSize = 1.0 / std::max(1.0, sec);
}

int CDVDMessageQueue::GetTimeSize() {
    CSingleLock lock(m_section);

    if(IsDataBased())
        return 0;
    else
        return (int)((m_TimeFront - m_TimeBack) / DVD_TIME_BASE);
}

bool CDVDMessageQueue::IsDataBased() const {
    return (m_TimeBack == DVD_NOPTS_VALUE || m_TimeFront == DVD_NOPTS_VALUE ||
            m_TimeFront <= m_TimeBack);
}
NS_KRMOVIE_END