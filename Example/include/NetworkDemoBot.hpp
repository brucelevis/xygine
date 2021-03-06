/*********************************************************************
Matt Marchant 2014 - 2017
http://trederia.blogspot.com

xygine - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#ifndef NETBOT_HPP_
#define NETBOT_HPP_

#include <xygine/network/Config.hpp>
#include <xygine/network/ClientConnection.hpp>

#include <SFML/System/Vector2.hpp>

class NetBot final
{
public:
    NetBot();
    ~NetBot() = default;

    void update(float);

    void connect(sf::IpAddress, xy::PortNumber);
    void disconnect();

    bool connected() const { return m_connection.connected(); }

private:
    xy::Network::ClientConnection m_connection;
    xy::Network::ClientConnection::PacketHandler m_packetHandler;

    struct Ball
    {
        sf::Vector2f position;
        sf::Vector2f velocity;
    }m_ball;
    float m_position;

    void handlePacket(xy::Network::PacketType, sf::Packet&, xy::Network::ClientConnection*);
};

#endif // NETBOT_HPP_