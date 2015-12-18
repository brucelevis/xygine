/*********************************************************************
Matt Marchant 2014 - 2015
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

//attempts to weld and fix two bodies together at a specific point

#ifndef XY_WELD_JOINT_HPP_
#define XY_WELD_JOINT_HPP_

#include <xygine/physics/Joint.hpp>
#include <Box2D/Dynamics/Joints/b2WeldJoint.h>
#include <SFML/System/Vector2.hpp>

namespace xy
{
    namespace Physics
    {
        class RigidBody;
        class WeldJoint final : public Joint
        {
        public:
            WeldJoint(const RigidBody& bodyA, const sf::Vector2f& worldWeldPoint);
            ~WeldJoint() = default;
            WeldJoint(const WeldJoint&) = default;
            WeldJoint& operator = (const WeldJoint&) = default;

            //returns the type of this joint
            Joint::Type type() const override { return Joint::Type::Weld; }
            //sets whether or not the attached bosies can collide
            //this has no affect once bodies are joined
            void canCollide(bool) override;
            //returns true if the attached bodies can collide with each other
            bool canCollide() const;

            //returns the reference angle between the bodies in degrees
            float getReferenceAngle() const;
            //sets the frequency of the joint in hertz
            void setFrequency(float);
            //gets the current frequency of the joint in hertz
            float getFrequency() const;
            //set the damping ratio of the joint
            void setDampingRatio(float);
            //get the current dampin ratio of the joint
            float getDampingRatio() const;

        private:

            b2JointDef* getDefinition() override;

            b2WeldJointDef m_definition;
            b2Vec2 m_anchor;
        };
    }
}

#endif //XY_WELDJOINT_HPP_