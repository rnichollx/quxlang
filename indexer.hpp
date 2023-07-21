#ifndef RPNX_INDEXER_HPP
#define RPNX_INDEXER_HPP

#include <cassert>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

namespace rpnx
{
    template < typename Parent >
    class graph_dependency_base : public std::enable_shared_from_this< graph_dependency_base< Parent > >
    {
      protected:
        // TODO: Add multithreading support, design a threading model for resolvables.
        std::atomic< ssize_t > m_unmet_dependencies = 0;

        std::vector< std::weak_ptr< graph_dependency_base > > m_dependents;
        static std::vector< std::weak_ptr< graph_dependency_base > >& use_dependents(graph_dependency_base* th)
        {
            return th->m_dependents;
        }

      public:
        enum class state { initialized, ready, unmet_dependency, error, resolved };

      protected:
        std::exception_ptr m_error_state;
        state m_state = state::initialized;

      public:
        state get_state() const
        {
            return m_state;
        }

        std::exception_ptr get_error() const
        {
            return m_error_state;
        }

        virtual void process(Parent* parent) = 0;
    };

    template < typename Parent, typename T >
    class graph_value_provider : public virtual graph_dependency_base< Parent >
    {
        std::optional< T > m_value;

        T get_value() const
        {
            assert(this->get_state() == graph_dependency_base< Parent >::state::resolved || this->get_state() == graph_dependency_base< Parent >::state::error);
            if (this->get_state() == graph_dependency_base< Parent >::state::error)
            {
                std::rethrow_exception(this->get_error());
            }
            assert(m_value.has_value());
            return *m_value;
        }
    };

    template < typename Parent, typename Resolver >
    class graph_indexable : public virtual graph_dependency_base< Parent >, public graph_value_provider< Parent, typename Resolver::value_type >
    {
        using resolver_type = Resolver;

      private:
        friend resolver_type;

      public:
        using value_type = typename Resolver::value_type;
        using key_type = typename Resolver::key_type;

      private:
        Resolver m_resolver;

      public:
        graph_indexable(key_type input)
            : m_resolver(input)
        {
        }

        bool has_unmet_dependencies() const
        {
            return this->graph_dependency_base< Parent >::m_unmet_dependencies > 0;
        }

        virtual void process(Parent* parent) override final
        {
            assert(this->get_state() == graph_dependency_base< Parent >::state::initialized || this->get_state() == graph_dependency_base< Parent >::state::ready);
            try
            {
                m_resolver.process(parent, this);
            }
            catch (...)
            {
                std::exception_ptr eptr = std::current_exception();
                set_exception(parent, eptr);
            };
        }

        void set_exception(Parent* parent, std::exception_ptr ptr)
        {
            this->m_error_state = ptr;
            this->m_state = graph_dependency_base< Parent >::state::error;
            for (auto& dep : this->m_dependents)
            {
                if (auto sp = dep.lock())
                {
                    sp->m_unmet_dependencies--;
                    if (sp->m_unmet_dependencies == 0)
                    {
                        parent->notify_ready(sp);
                    }
                    assert(sp->m_unmet_dependencies >= 0);
                }
            }

            this->m_dependents.clear();
        }

        void set_value(Parent* parent, value_type value)
        {
            this->m_value = value;
            this->m_state = graph_dependency_base< Parent >::state::resolved;
            for (auto& dep : this->m_dependents)
            {
                if (auto sp = dep.lock())
                {
                    sp->m_unmet_dependencies--;
                    if (sp->m_unmet_dependencies == 0)
                    {
                        parent->notify_ready(sp);
                    }
                    assert(sp->m_unmet_dependencies >= 0);
                }
            }

            this->m_dependents.clear();
        }

        void add_dependency(Parent*, std::shared_ptr< graph_dependency_base< Parent > > dep)
        {
            assert(dep != nullptr);
            this->m_unmet_dependencies++;
            use_dependents(&*dep).push_back(this->shared_from_this());
        }

      private:
    };

    template < typename Parent, typename Resolver >
    class graph_index
    {
      public:
        using key_type = typename Resolver::key_type;
        using value_type = typename Resolver::value_type;

      private:
        using indexable_type = graph_indexable< Parent, Resolver >;

      private:
        std::map< key_type, std::shared_ptr< indexable_type > > m_index;

      public:
        std::shared_ptr< graph_value_provider< Parent, value_type > > get(key_type key)
        {
            auto it = m_index.find(key);
            if (it == m_index.end())
            {
                auto new_indexable = std::make_shared< indexable_type >(key);
                m_index.insert(std::make_pair(key, new_indexable));
                return new_indexable;
            }
            else
            {
                return it->second;
            }
        }
    };

} // namespace rpnx

#endif