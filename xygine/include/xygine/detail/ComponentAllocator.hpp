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

#ifndef XY_COMPONENT_ALLOCATOR_HPP_
#define XY_COMPONENT_ALLOCATOR_HPP_

#include <xygine/detail/ComponentSource.hpp>

#include <typeindex>
#include <map>

namespace xy
{
    namespace Detail
    {
        /*
        \brief Makes sure memory is allocated to any component sources
        which are needed for a particular type
        */
        class ComponentAllocator final
        {
        public:
            ComponentAllocator() = default;
            ~ComponentAllocator() = default;
            ComponentAllocator(const ComponentAllocator&) = delete;
            ComponentAllocator& operator = (const ComponentAllocator&) = delete;

            void update(float dt)
            {
                for (auto& r : m_sources) r.second->update(dt);
            }

            template <typename T, typename... Args>
            std::unique_ptr<T, std::function<void(T*)>> getComponent(Args... args)
            {
                static_assert(std::is_base_of<Component, T>::vale, "Must be a component type");
                std::type_index idx(typeid(T));
                if (m_sources.count(idx) == 0)
                {
                    m_sources.insert(std::make_pair(idx, std::make_unique<ComponentSource<T>>()));
                }
                return std::move(dynamic_cast<T*>(m_sources[idx].get())->create(std::forward<Args>(args)...));
            }

        private:

            std::map<std::type_index, std::unique_ptr<BaseComponentSource>> m_sources;
        };
    }
}

#endif //XY_COMPONENT_ALLOCATOR_HPP_