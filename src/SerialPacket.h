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
        const uint32_t dataLen = 0;
        port.write(type);
        port.write(reinterpret_cast<byte const *>(&dataLen), sizeof(uint32_t));
    }

    template<typename T>
    void Send(const byte type, const T t)
    {
        const uint32_t dataLen = sizeof(T);
        port.write(type);
        port.write(reinterpret_cast<byte const *>(&dataLen), sizeof(uint32_t));

        port.write(reinterpret_cast<byte const *>(&t), sizeof(T));
    }

    template<typename T0, typename T1>
    void Send(const byte type, const T0 t0, const T1 t1)
    {
        constexpr const uint32_t dataLen = sizeof(T0) + sizeof(T1);
        static_assert(dataLen <= MAX_DATA_SIZE, "Too much data for packet");

        port.write(type);
        port.write(reinterpret_cast<byte const *>(&dataLen), sizeof(uint32_t));

        port.write(reinterpret_cast<byte const *>(&t0), sizeof(T0));
        port.write(reinterpret_cast<byte const *>(&t1), sizeof(T1));
    }

    template<typename T0, typename T1, typename T2>
    void Send(const byte type, const T0 t0, const T1 t1, const T2 t2)
    {
        constexpr const uint32_t dataLen = sizeof(T0) + sizeof(T1) + sizeof(T2);
        static_assert(dataLen <= MAX_DATA_SIZE, "Too much data for packet");

        port.write(type);
        port.write(reinterpret_cast<byte const *>(&dataLen), sizeof(uint32_t));

        port.write(reinterpret_cast<byte const *>(&t0), sizeof(T0));
        port.write(reinterpret_cast<byte const *>(&t1), sizeof(T1));
        port.write(reinterpret_cast<byte const *>(&t2), sizeof(T2));
    }

    template<typename T0, typename T1, typename T2, typename T3>
    void Send(const byte type, const T0 t0, const T1 t1, const T2 t2, const T3 t3)
    {
        constexpr const uint32_t dataLen = sizeof(T0) + sizeof(T1) + sizeof(T2) + sizeof(T3);
        static_assert(dataLen <= MAX_DATA_SIZE, "Too much data for packet");

        port.write(type);
        port.write(reinterpret_cast<byte const *>(&dataLen), sizeof(uint32_t));

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

    EAvailableStatus Available()
    {
        while(m_pPort && m_pPort->available() >= (m_state == EReadState::ReadDataLen ? 4 : 1))
        {
            const byte recChar = m_pPort->read();

            if (m_state == EReadState::ReadType)
            {
                m_type = recChar;
                m_curDataLen = 0;
                if (m_type > maxCommandChar)
                {
                    m_state = EReadState::ReadString;
                    m_data[m_curDataLen++] = recChar;
                }
                else
                {
                    m_state = EReadState::ReadDataLen;
                }
                continue;
            }
            else if (m_state == EReadState::ReadDataLen)
            {
                // Reconstruct 4 byte data len from recieved data
                m_dataLen = recChar;
                for (int i = 1; i < 4; ++i) 
                {
                    const byte nextChar = m_pPort->read();
                    m_dataLen += nextChar << (8*i);
                }

                if (m_dataLen == 0)
                {
                    // No data - return that packet is ready
                    m_state = EReadState::ReadType;
                    m_dataPtr = m_data;
                    return EAvailableStatus::PacketAvailable;
                }
                else
                {
                    // Start reading data
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

    const byte* GetData() const
    {
        return m_data;
    }

    const uint32_t GetDataLen() const
    {
        return m_dataLen;
    }

    template<typename T>
    T Get()
    {
        constexpr const uint32_t typeSize = sizeof(T);
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
