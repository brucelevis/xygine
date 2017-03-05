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
        //ensures ownership of components by entities
        template <class T>
        class XY_EXPORT_API ComponentPtr
        {
        public:
            ComponentPtr(T* p = nullptr) : m_ptr(p) { static_assert(std::is_base_of<xy::Component, T>::value, "Requires component type"); }
            ~ComponentPtr() { if(m_ptr) static_cast<xy::Component*>(m_ptr)->destroy(); }
            ComponentPtr(const ComponentPtr<T>&) = delete;
            ComponentPtr<T>& operator = (const ComponentPtr<T>&) = delete;
            ComponentPtr(ComponentPtr<T>&& other)
            {
                if (&other == this) return;
                
                this->m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            ComponentPtr<T>& operator = (ComponentPtr<T>&& other)
            {
                if (&other == this) return *this;
                
                this->m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
                return *this;
            }

            bool operator == (const ComponentPtr<T>& other) const
            {
                return m_ptr == other.m_ptr;
            }

            bool operator != (const ComponentPtr<T>& other) const 
            {
                return m_ptr != other.m_ptr;
            }

            bool operator == (nullptr_t n) const
            {
                return m_ptr == n;
            }

            bool operator != (nullptr_t n) const
            {
                return m_ptr != n;
            }

            explicit operator bool() const
            {
                return m_ptr != nullptr;
            }

            T& operator * () const noexcept
            {
                return *m_ptr;
            }

            T* operator -> () const noexcept
            {
                return m_ptr;
            }

            T* get() { return m_ptr; }

        private:
            T* m_ptr;
        };

        template <class T>
        class XY_EXPORT_API ComponentSource final
        {
        public:
            ComponentSource() : m_pool(2048)
            {
                static_assert(std::is_base_of<xy::Component, T>::value, "Requires component type");
            }
            ~ComponentSource() {}

            void update(float dt) 
            {
                m_components.erase(std::remove_if(std::begin(m_components), std::end(m_components),
                    [](const ObjectPool<T>::Ptr& p)
                {
                    auto ptr = static_cast<xy::Component*>(p.get());
                    return (!ptr->m_entity || ptr->m_entity->destroyed() || ptr->destroyed());
                }));

                for (auto& c : m_components) c->update(dt);
            }

            template <typename... Args>
            ComponentPtr<T> create(Args&&... args)
            {
                m_components.push_back(std::move(m_pool.get(std::forward<Args>(args)...)));
                ComponentPtr<T> ptr(m_components.back().get());
                return std::move(ptr);
            }

        private:
            ObjectPool<T> m_pool;
            std::vector<std::unique_ptr<T, std::function<void(T*)>>> m_components;
        };
    }
}
#endif //XY_COMPONENT_SOURCE_HPP_