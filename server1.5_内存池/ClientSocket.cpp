#include "ClientSocket.h"

ClientSocket::ClientSocket(SOCKET sock)
{
	m_ClientSocket = sock;
	m_nMsgLastPos = 0;
	m_nSendLastPos = 0;
	memset(m_chMsgBuf, 0, sizeof(m_chMsgBuf));
	memset(m_chSendBuf, 0, sizeof(m_chSendBuf));
}

ClientSocket::~ClientSocket()
{
	m_ClientSocket = INVALID_SOCKET;
	m_nMsgLastPos = 0;
	memset(m_chMsgBuf, 0, sizeof(m_chMsgBuf));
}

int ClientSocket::GetLastPos()
{
	return m_nMsgLastPos;
}

void ClientSocket::SetLastPos(int pos)
{
	m_nMsgLastPos = pos;
}
void ClientSocket::SetSocket(SOCKET sock)
{
	m_ClientSocket = sock;
}

char* ClientSocket::GetMsgBuf()
{
	return m_chMsgBuf;
}

int ClientSocket::SendData(DataHeader* header)
{
	int nRet = SOCKET_ERROR;
	if (!header)
		return nRet;

	//当发送缓冲区满了再发送

	//要发送数据的长度
	int nSendLen = header->dataLength;
	//数据转换一下
	const char* chData = (const char*)header;

	while (true)
	{
		//如果要发送的数据长度，大于发送缓冲区空余的长度
		if (nSendLen > SEND_BUFF_SIZE - m_nSendLastPos)
		{
			//计算可以发送的长度
			int nCopyLen = SEND_BUFF_SIZE - m_nSendLastPos;
			memcpy(m_chSendBuf + m_nSendLastPos, chData, nCopyLen);
			//剩余要发送的数据长度和数据
			nSendLen -= nCopyLen;
			chData += nCopyLen;

			nRet = send(m_ClientSocket, m_chSendBuf, sizeof(m_chSendBuf), 0);
			m_nSendLastPos = 0;

			if (nRet = SOCKET_ERROR)
				return nRet;
		}
		else
		{
			memcpy(m_chSendBuf + m_nSendLastPos, chData, nSendLen);
			m_nSendLastPos += nSendLen;
			break;
		}
	}
	return 0;
}

SOCKET ClientSocket::GetSocket()
{
	return m_ClientSocket;
}
