#pragma once

#include "Arduino.h"

// The size of the buffer when recieving data. Set this as small as possible
// as Arduino boards tend to have extremely limited RAM
const uint8_t MAX_DATA_SIZE = 16;

// Used for sending data.
//
// Create a new OutPacket passing it the stream/serial port to use and
// then simply call send passing in a type and extra data
// 
// OutPacket packet(Serial)
// packet.Send(1, 0.0f, true);
class OutPacket
{
public:
    OutPacket(Stream &port)
        : port(port)
    {}

    void Send(const byte type)
    {
        const byte dataLen = 0;
        port.write(type);
        port.write(dataLen);
    }

    template<typename T>
    void Send(const byte type, const T t)
    {
        const byte dataLen = sizeof(T);
        port.write(type);
        port.write(dataLen);

        port.write(reinterpret_cast<byte const *>(&t), sizeof(T));
    }

    template<typename T0, typename T1>
    void Send(const byte type, const T0 t0, const T1 t1)
    {
        constexpr const byte dataLen = sizeof(T0) + sizeof(T1);
        static_assert(dataLen <= MAX_DATA_SIZE, "Too much data for packet");

        port.write(type);
        port.write(dataLen);

        port.write(reinterpret_cast<byte const *>(&t0), sizeof(T0));
        port.write(reinterpret_cast<byte const *>(&t1), sizeof(T1));
    }

    template<typename T0, typename T1, typename T2>
    void Send(const byte type, const T0 t0, const T1 t1, const T2 t2)
    {
        constexpr const byte dataLen = sizeof(T0) + sizeof(T1) + sizeof(T2);
        static_assert(dataLen <= MAX_DATA_SIZE, "Too much data for packet");

        port.write(type);
        port.write(dataLen);

        port.write(reinterpret_cast<byte const *>(&t0), sizeof(T0));
        port.write(reinterpret_cast<byte const *>(&t1), sizeof(T1));
        port.write(reinterpret_cast<byte const *>(&t2), sizeof(T2));
    }

    template<typename T0, typename T1, typename T2, typename T3>
    void Send(const byte type, const T0 t0, const T1 t1, const T2 t2, const T3 t3)
    {
        constexpr const byte dataLen = sizeof(T0) + sizeof(T1) + sizeof(T2) + sizeof(T3);
        static_assert(dataLen <= MAX_DATA_SIZE, "Too much data for packet");

        port.write(type);
        port.write(dataLen);

        port.write(reinterpret_cast<byte const *>(&t0), sizeof(T0));
        port.write(reinterpret_cast<byte const *>(&t1), sizeof(T1));
        port.write(reinterpret_cast<byte const *>(&t2), sizeof(T2));
        port.write(reinterpret_cast<byte const *>(&t3), sizeof(T3));
    }

private:
    Stream& port;
};

// Packet class for recieving data from serial connection
// The same InPacket can be used repeatedly.
// Once Available return trues the entire packet data must be read before the
// next call to Available, as at that point the previous packet will be invalidated
class InPacket
{
public:	
	void Begin(Stream &port)
	{
		m_pPort = &port;
	}

    bool Available()
    {
        while(m_pPort && m_pPort->available() > 0)
        {
            const byte recChar = m_pPort->read();

            if (m_state == EReadState::ReadType)
            {
                m_type = recChar;
                m_state = EReadState::ReadDataLen;
                continue;
            }
            else if (m_state == EReadState::ReadDataLen)
            {
                m_dataLen = recChar;

                if (m_dataLen == 0)
                {
					m_state = EReadState::ReadType;
                    return true;
                }
                else
                {
                    m_curDataLen = 0;
                    m_state = EReadState::ReadData;
                    continue;
                }
            }
            else if (m_state == EReadState::ReadData)
            {
                if (m_curDataLen < MAX_DATA_SIZE)
                {
                    // Add the incoming data to the buffer
                    m_data[m_curDataLen++] = recChar;
                }
                else
                {
                    // Not room left to store the incoming data in the buffer so
                    // just discard instead
                    m_curDataLen++;
                }
                
                if (m_curDataLen == m_dataLen)
                {
                    // Add data now read, so packet is complete
                    m_state = EReadState::ReadType;
                    return true;
                }
            }
        }

        return false;
    }

    const byte Gettype() const
    {
        return m_type;
    }

private:
    enum class EReadState : uint8_t
    {
        ReadType,
        ReadDataLen,
        ReadData,
    };

    Stream* m_pPort = nullptr;

    EReadState m_state = EReadState::ReadType;
    byte m_type;
    byte m_dataLen;
    byte m_curDataLen;
    byte m_data[MAX_DATA_SIZE];
};
