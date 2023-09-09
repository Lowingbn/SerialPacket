#pragma once

#include "Arduino.h"

// Used for sending data.
//
// Create a new OutPacket passing it the stream/serial port to use and
// then simply call send passing in a type and extra data
// 
// SerialOutPacket<16> packet(Serial)
// packet.Send(1, 0.0f, true);
template<const uint8_t MAX_DATA_SIZE = 16>
class SerialOutPacket
{
public:
    SerialOutPacket(Stream &port)
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
// The same SerialInPacket can be used repeatedly.
// Strings and packets can be received.  A packet has type-length-data format and
// starts with a type <= maxCommandChar.  A string starts with any other character
// and continues until a newline is sent.
// This allows the same serial interface to be used for both string console commands
// and binary packets.
template<const uint32_t MAX_DATA_SIZE = 1024>
class SerialInPacket
{
public:	
    void Begin(Stream &port)
    {
        m_pPort = &port;
    }

    enum class EAvailableStatus : uint8_t
    {
        None = 0,
        StringAvailable,  // A string terminated with \n is available
        PacketAvailable,  // A serial packet is available
    };

    bool Available()
    {
        while(m_pPort && m_pPort->available() > 0)
        {
            const byte recChar = m_pPort->read();

            if (m_state == EReadState::ReadType)
            {
                m_type = recChar;
                if (m_type > maxCommandChar)
                {
                    m_state = EReadState::ReadString;
                }
                else
                {
                    m_state = EReadState::ReadDataLen;
                    m_dataL
                }
                continue;
            }
            else if (m_state == EReadState::ReadDataLen)
            {
                // Wait until a 4 byte length is available
                if (m_pPort->available() < 4) break;

                m_dataLen = recChar;
                for (int i = 0; i < 3; ++i) {
                    m_dataLen <<= 8;
                    m_dataLen += m_pPort->read();
                }

                if (m_dataLen == 0)
                {
                    m_state = EReadState::ReadType;
                    m_dataPtr = m_data;
                    return EAvailableStatus::PacketAvailable;
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
                    m_dataLen = (m_dataLen > MAX_DATA_SIZE ? MAX_DATA_SIZE : m_dataLen);
                    m_dataPtr = m_data;
                    return EAvailableStatus::PacketAvailable;
                }
            }
            else if (m_state == EReadState::ReadString)
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

                if (recChar == endStringChar)
                {
                    // Terminate string
                    if (m_curDataLen < MAX_DATA_SIZE)
                    {
                        m_data[m_curDataLen] = 0;
                    }
                    else
                    {
                        m_data[MAX_DATA_SIZE-1] = 0;
                    }

                    m_state = EReadState::ReadType;
                    m_dataLen = (m_curDataLen > MAX_DATA_SIZE ? MAX_DATA_SIZE : m_curDataLen);
                    m_dataPtr = m_data;
                    return EAvailableStatus::StringAvailable;
                }
            }
        }

        return EAvailableStatus::None;
    }

    const byte GetType() const
    {
        return m_type;
    }

    const char* GetStr() const
    {
        return (const char*)m_data;
    }

    const int GetStrLen() const
    {
        return m_dataLen;
    }

    template<typename T>
    T Get()
    {
        constexpr const byte typeSize = sizeof(T);
        if (m_dataPtr - m_data + typeSize > m_dataLen)
        {
            return T();
        }
        else
        {
            // Requires that the type is POD.
            T value;
            byte* valueAsBytePtr = reinterpret_cast<byte*>(&value);
            memcpy(valueAsBytePtr, m_dataPtr, typeSize);
            m_dataPtr += typeSize;
            return value;
        } 
    }

private:
    enum class EReadState : uint8_t
    {
        ReadType,
        ReadDataLen,
        ReadData,
        ReadString,
    };

    Stream* m_pPort = nullptr;

    static const byte endStringChar = '\n';
    static const byte maxCommandChar = ' ';

    EReadState m_state = EReadState::ReadType;
    byte m_type;
    uint32_t m_dataLen;
    uint32_t m_curDataLen;
    byte m_data[MAX_DATA_SIZE];
    byte* m_dataPtr;
};
