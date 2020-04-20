#include "ZmqMonitorThread.h"
#include <QDebug>
#include <windows.h>
#include <assert.h>
#include <QHostInfo>

ZmqMonitorThread::ZmqMonitorThread(void* context, const QString& url, QObject *parent) : QThread(parent)
{
    mContext = context;
    mUrl = url;
}

void ZmqMonitorThread::run()
{
    if(!mContext) return;
    SocketMonitor(mContext);
}

bool ZmqMonitorThread::GetPeerIPAndPort(int fd, QString& ip, int& port, QString& name)
{
    int client_fd = fd;

    // discovery client information
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    if(getpeername(client_fd, (struct sockaddr*)&addr, &addrlen) == -1){
        return false ;
    }
    // discovery server information
    struct sockaddr_in serverAddr;
    int serverAddrLen = sizeof(serverAddr);
    if(getsockname(client_fd, (struct sockaddr*)&serverAddr, &serverAddrLen) == -1){
        return false ;
    }


    // ip v4 or v6
    char *buf = 0;

    if((buf = inet_ntoa(addr.sin_addr)) == NULL){
        return false;
    }

    port = htons(serverAddr.sin_port);
    ip = QString(buf);
    //qDebug()<<"ip"<<ip<<" port"<<port;
    QHostInfo info = QHostInfo::fromName(ip);
    name = info.hostName();
    return  true;
}

int ZmqMonitorThread::ReadMsg(void* s, zmq_event_t* event, char* ep)
{
    int rc ;
    zmq_msg_t msg1;  // binary part
    zmq_msg_init (&msg1);
    zmq_msg_t msg2;  //  address part
    zmq_msg_init (&msg2);
    rc = zmq_msg_recv (&msg1, s, 0);
    if (rc == -1 && zmq_errno() == ETERM)
        return 1 ;
    assert (rc != -1);
    assert (zmq_msg_more(&msg1) != 0);
    rc = zmq_msg_recv (&msg2, s, 0);
    if (rc == -1 && zmq_errno() == ETERM)
        return 1;
    assert (rc != -1);
    assert (zmq_msg_more(&msg2) == 0);
    // copy binary data to event struct
    const char* data = (char*)zmq_msg_data(&msg1);
    memcpy(&(event->event), data, sizeof(event->event));
    memcpy(&(event->value), data+sizeof(event->event),       sizeof(event->value));
    // copy address part
    const size_t len = zmq_msg_size(&msg2) ;
    ep = (char*)memcpy(ep, zmq_msg_data(&msg2), len);
    *(ep + len) = 0 ;

    return 0 ;
}

// REP socket monitor thread
void *ZmqMonitorThread::SocketMonitor(void *ctx)
{
    zmq_event_t event;
    static char addr[1025] ;
    int rc;
    qDebug("starting monitor...\n");
    void *s = zmq_socket (ctx, ZMQ_PAIR);
    assert (s);
    rc = zmq_connect (s, mUrl.toStdString().c_str());
    assert (rc == 0);
    while (!ReadMsg(s, &event, addr)) {
        switch (event.event) {
        case ZMQ_EVENT_LISTENING:
            qDebug ("listening socket descriptor %d\n", event.value);
            qDebug ("listening socket address %s\n", addr);
            break;
        case ZMQ_EVENT_ACCEPTED:
        {
            qDebug ("accepted socket descriptor %d\n", event.value);
            qDebug ("accepted socket address %s\n", addr);
            QString ip, name; int port;
            if(GetPeerIPAndPort(event.value, ip,  port, name))
            {
                mFdIpMapList[event.value] = ip;
                mFdPortMapList[event.value] = port;
                emit signalClientInOut(ip, name, port, 1);
                qDebug ("client ip = %s port:%d", ip.toUtf8().data(), port );
            }
            break;
        }
        case ZMQ_EVENT_CLOSE_FAILED:
            qDebug ("socket close failure error code %d\n", event.value);
            qDebug ("socket address %s\n", addr);
            break;
        case ZMQ_EVENT_CLOSED:
            qDebug ("closed socket descriptor %d\n", event.value);
            qDebug ("closed socket address %s\n", addr);
            break;
        case ZMQ_EVENT_DISCONNECTED:
        {
            qDebug ("disconnected socket descriptor %d\n", event.value);
            qDebug ("disconnected socket address %s\n", addr);
            if(mFdIpMapList.contains(event.value))
            {
                emit signalClientInOut(mFdIpMapList[event.value],"", mFdPortMapList[event.value], 0);
            }
            break;
        }
        }
    }
    zmq_close (s);
    return NULL;
}
