/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal                                                                                                          *
*                                                                                                                      *
* Copyright (c) 2012-2024 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Implementation of SCPISocketTransport
	@ingroup transports
 */

#include "scopehal.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

/**
	@brief Connects to an instrument

	@param args	Arguments, of the format host:port
				If port number is not specified, defaults to 5025
 */
SCPISocketTransport::SCPISocketTransport(const string& args)
	: m_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
{
	char hostname[128];
	unsigned int port = 0;
	if(2 != sscanf(args.c_str(), "%127[^:]:%u", hostname, &port))
	{
		//default if port not specified
		m_hostname = args;
		m_port = 5025;
	}
	else
	{
		m_hostname = hostname;
		m_port = port;
	}

	SharedCtorInit();
}

/**
	@brief Connects to an instrument

	@param hostname		IP or hostname of the instrument
	@param port			Port number of the instrument
 */
SCPISocketTransport::SCPISocketTransport(const string& hostname, unsigned short port)
	: m_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
	, m_hostname(hostname)
	, m_port(port)
{
	SharedCtorInit();
}

/**
	@brief Helper function that actually opens the socket connection
 */
void SCPISocketTransport::SharedCtorInit()
{
	LogDebug("Connecting to SCPI device at %s:%d\n", m_hostname.c_str(), m_port);

	if(!m_socket.Connect(m_hostname, m_port))
	{
		m_socket.Close();
		LogError("Couldn't connect to socket\n");
		return;
	}
	if(!m_socket.SetRxTimeout(5000000))
		LogWarning("No Rx timeout: %s\n", strerror(errno));
	if(!m_socket.SetTxTimeout(5000000))
		LogWarning("No Tx timeout: %s\n", strerror(errno));
	if(!m_socket.DisableNagle())
	{
		m_socket.Close();
		LogError("Couldn't disable Nagle\n");
		return;
	}
	if(!m_socket.DisableDelayedACK())
	{
		m_socket.Close();
		LogError("Couldn't disable delayed ACK\n");
		return;
	}
}

SCPISocketTransport::~SCPISocketTransport()
{
}

bool SCPISocketTransport::IsConnected()
{
	return m_socket.IsValid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual transport code

///@brief Returns the constant transport name "lan"
string SCPISocketTransport::GetTransportName()
{
	return "lan";
}

string SCPISocketTransport::GetConnectionString()
{
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "%s:%u", m_hostname.c_str(), m_port);
	return string(tmp);
}

bool SCPISocketTransport::SendCommand(const string& cmd)
{
	LogTrace("[%s] Sending %s\n", m_hostname.c_str(), cmd.c_str());
	string tempbuf = cmd + "\n";
	return m_socket.SendLooped((unsigned char*)tempbuf.c_str(), tempbuf.length());
}

string SCPISocketTransport::ReadReply(bool endOnSemicolon, [[maybe_unused]] function<void(float)> progress)
{
	//FIXME: there *has* to be a more efficient way to do this...
	char tmp = ' ';
	string ret;
	while(true)
	{
		if(!m_socket.RecvLooped((unsigned char*)&tmp, 1))
			break;
		if( (tmp == '\n') || ( (tmp == ';') && endOnSemicolon ) )
			break;
		else
			ret += tmp;
	}
	LogTrace("[%s] Got %s\n", m_hostname.c_str(), ret.c_str());
	return ret;
}

void SCPISocketTransport::FlushRXBuffer(void)
{
	m_socket.FlushRxBuffer();
}

void SCPISocketTransport::SendRawData(size_t len, const unsigned char* buf)
{
	m_socket.SendLooped(buf, len);
}

size_t SCPISocketTransport::ReadRawData(size_t len, unsigned char* buf, std::function<void(float)> progress)
{
	size_t chunk_size = len;
	if (progress)
	{
		/* carve up the chunk_size into either 1% or 32kB chunks, whichever is larger; later, we'll want RecvLooped to do this for us */
		chunk_size /= 100;
		if (chunk_size < 32768)
			chunk_size = 32768;
	}

	for (size_t pos = 0; pos < len; )
	{
		size_t n = chunk_size;
		if (n > (len - pos))
			n = len - pos;
		if(!m_socket.RecvLooped(buf + pos, n))
		{
			LogTrace("Failed to get %zu bytes (@ pos %zu)\n", len, pos);
			return 0;
		}
		pos += n;
		if (progress)
		{
			progress((float)pos / (float)len);
		}
	}

	LogTrace("Got %zu bytes\n", len);
	return len;
}

bool SCPISocketTransport::IsCommandBatchingSupported()
{
	return true;
}
