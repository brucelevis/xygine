/*********************************************************************
© Matt Marchant 2014 - 2017
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

#ifndef XY_COMPONENT_SOURCE_HPP_
#define XY_COMPONENT_SOURCE_HPP

#include <xygine/detail/ObjectPool.hpp>
#include <xygine/Entity.hpp>
#include <xygine/Config.hpp>

#include <vector>

namespace xy
{
    class Component;
    namespace Detail
    {
        template <class T>
        class XY_EXPORT_API ComponentSource final
        {
        public:
            ComponentSource() : m_pool(2048)
            {
                static_assert(std::is_base_of<xy::Component, T>::value);

            }
            ~ComponentSource() {}

            void update(float dt) 
            {
                m_components.erase(std::remove_if(std::begin(m_components), std::end(m_components),
                    [](const ObjectPool<T>::Ptr& p)
                {
                    auto ptr = static_cast<xy::Component*>(p.get());
                    return (!ptr->m_entity || m_entity->destroyed());
                }));

                for (auto& c : m_components) c->update(dt);
            }

            template <typename... Args>
            T* create(Args&&... args) { return nullptr; }

        private:
            ObjectPool<T> m_pool;
            std::vector<ObjectPool<T>::Ptr> m_components;
        };
    }
}
#endif //XY_COMPONENT_SOURCE_HPP_