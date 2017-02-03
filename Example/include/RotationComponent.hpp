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

#ifndef ROTATION_COMPONENT_HPP_
#define ROTATION_COMPONENT_HPP_

#include <xygine/components/Component.hpp>
#include <xygine/Entity.hpp>

class RotationComponent final : public xy::Component
{
public:
    RotationComponent(xy::MessageBus& mb) : xy::Component(mb, this) {}
    ~RotationComponent() = default;

    xy::Component::Type type() const override { return xy::Component::Type::Script; }
    void entityUpdate(float dt)
    {
        getEntity()->rotate(90.f * dt);
    }
};

#endif ///ROTATION_COMPONENT_HPP_