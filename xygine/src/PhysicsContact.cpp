/*********************************************************************
Matt Marchant 2014 - 2016
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

#include <xygine/physics/Contact.hpp>
#include <xygine/Assert.hpp>

#include <Box2D/Dynamics/Contacts/b2Contact.h>

using namespace xy;
using namespace xy::Physics;

bool Contact::touching() const
{
    XY_ASSERT(m_contact, "Contact is null");
    return m_contact->IsTouching();
}

void Contact::enabled(bool enabled)
{
    XY_ASSERT(m_contact, "Contact is null");
    m_contact->SetEnabled(enabled);
}

bool Contact::enabled() const
{
    XY_ASSERT(m_contact, "Contact is null");
    return m_contact->IsEnabled();
}

CollisionShape* Contact::getCollisionShapeA()
{
    XY_ASSERT(m_contact, "Contact is null");
    return static_cast<CollisionShape*>(m_contact->GetFixtureA()->GetUserData());
}

const CollisionShape* Contact::getCollisionShapeA() const
{
    XY_ASSERT(m_contact, "Contact is null");
    return static_cast<CollisionShape*>(m_contact->GetFixtureA()->GetUserData());
}

CollisionShape* Contact::getCollisionShapeB()
{
    XY_ASSERT(m_contact, "Contact is null");
    return static_cast<CollisionShape*>(m_contact->GetFixtureB()->GetUserData());
}

const CollisionShape* Contact::getCollisionShapeB() const
{
    XY_ASSERT(m_contact, "Contact is null");
    return static_cast<CollisionShape*>(m_contact->GetFixtureB()->GetUserData());
}

float Contact::getFriction() const
{
    XY_ASSERT(m_contact, "Contact is null");
    return m_contact->GetFriction();
}

void Contact::setFriction(float friction)
{
    XY_ASSERT(m_contact, "Contact is null");
    XY_ASSERT(friction >= 0, "Friction value must be positive");
    m_contact->SetFriction(friction);
}

void Contact::resetFriction()
{
    XY_ASSERT(m_contact, "Contact is null");
    m_contact->ResetFriction();
}

float Contact::getRestitution() const
{
    XY_ASSERT(m_contact, "Contact is null");
    return m_contact->GetRestitution();
}

void Contact::setRestitution(float value)
{
    XY_ASSERT(m_contact, "Contact is null");
    XY_ASSERT(value >= 0, "Restitution must be positive");
    m_contact->SetRestitution(value);
}

void Contact::resetRestitution()
{
    XY_ASSERT(m_contact, "Contact is null");
    m_contact->ResetRestitution();
}

void Contact::setTangentSpeed(float speed)
{
    XY_ASSERT(m_contact, "Contact is null");
    m_contact->SetTangentSpeed(speed);
}

float Contact::getTangentSpeed() const
{
    XY_ASSERT(m_contact, "Contact is null");
    return m_contact->GetTangentSpeed();
}