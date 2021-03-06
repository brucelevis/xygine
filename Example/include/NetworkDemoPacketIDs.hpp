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

#ifndef NET_PACKET_IDS_HPP_
#define NET_PACKET_IDS_HPP_

#include <xygine/network/Config.hpp>

namespace NetDemo
{
    enum PacketID
    {
        //ent count, ent ID, float x, float y
        PositionUpdate = xy::Network::PacketType::Count,
        //ent ID, float x, float y, float velX, float velY
        BallSpawned,
        //ent ID, float x, floay y, float velX, float velY, step
        BallUpdate,
        //player id, ent id, position x, position y, name
        PlayerSpawned,
        //player id, player name
        PlayerDetails,
        //input struct
        PlayerInput,
        //player count, ent ID, position, last input id
        PlayerUpdate,
        //ent id
        EntityDestroyed,
        //player number, player score
        ScoreUpdate
    };
}
#endif //NET_PACKET_IDS_HPP_