#include "IbkrClient.h"

const int PING_DEADLINE = 2;        // seconds
const int SLEEP_BETWEEN_PINGS = 30; // seconds

IbkrClient::IbkrClient() : m_osSignal(2000),
                           m_pClient(new EClientSocket(this, &m_osSignal)),
                           m_state(ST_CONNECT),
                           m_sleepDeadline(0),
                           m_orderId(0),
                           m_pReader(0),
                           m_extraAuth(false)
{
}

IbkrClient::~IbkrClient()
{
    if (m_pReader)
    {
        delete m_pReader;
    }
    delete m_pClient;
}

void IbkrClient::processMessages()
{
    time_t now = time(NULL);
    switch (m_state)
    {
    case ST_PNLSINGLE:

        break;
    case ST_PNLSINGLE_ACK:
        break;
    default:
        break;
    }
}

bool IbkrClient::connect(const char *host, int port, int clientId)
{
    printf("Connecting to %s:%d, clientId: %d\n", !(host && *host) ? "127.0.0.1" : host, port, clientId);

    bool res = m_pClient->eConnect(host, port, clientId, m_extraAuth);
    if (res)
    {
        printf("Connected to %s:%d, clientId: %d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);
        m_pReader = new EReader(m_pClient, &m_osSignal);
        m_pReader->start();
    }
    else
    {
        printf("Cannot connect to %s:%d, clientId: %d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);
    }
    return res;
}

void IbkrClient::disconnect() const
{
    m_pClient->eDisconnect();
    printf("Disconnected\n");
}

void IbkrClient::connectAck()
{
    if (!m_extraAuth && m_pClient->asyncEConnect())
    {
        m_pClient->startApi();
    }
}

bool IbkrClient::isConnected() const
{
    return m_pClient->isConnected();
}

void IbkrClient::pnlSingleOperation()
{
    m_pClient->reqPnLSingle(7002, "", "", 268084);
}