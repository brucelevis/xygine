/*********************************************************************
� Matt Marchant 2014 - 2017
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

#include <xygine/physics/CollisionRectangleShape.hpp>
#include <xygine/physics/World.hpp>

#include <cstring>
#include <cmath>

using namespace xy;
using namespace xy::Physics;

CollisionRectangleShape::CollisionRectangleShape(const sf::Vector2f& size, const sf::Vector2f& position)
{
    setRect(size, position);
    setShape(m_rectangleShape);
}

xy::Physics::CollisionRectangleShape::CollisionRectangleShape(const sf::FloatRect & size)
{
	setRect(size);
	setShape(m_rectangleShape);
}

CollisionRectangleShape::CollisionRectangleShape(const CollisionRectangleShape& other)
    : CollisionShape(other)
{
    setRect(other.m_size, other.m_position);
    setShape(m_rectangleShape);
}


//public
void CollisionRectangleShape::setRect(const sf::Vector2f& size, const sf::Vector2f& position)
{
    auto boxSize = World::sfToBoxVec(size / 2.f);
    auto boxPosition = World::sfToBoxVec(position);
    m_rectangleShape.SetAsBox(boxSize.x, std::abs(boxSize.y), boxSize + boxPosition, 0.f);

    m_size = size;
    m_position = position;
}

void CollisionRectangleShape::setRect(const sf::FloatRect& size)
{
    setRect({ size.width, size.height }, { size.left, size.top });
}

const sf::Vector2f& CollisionRectangleShape::getSize() const
{
    return m_size;
}

const sf::Vector2f& CollisionRectangleShape::getPosition() const
{
    return m_position;
}
