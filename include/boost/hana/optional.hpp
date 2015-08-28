/*!
@file
Defines `boost::hana::optional`.

@copyright Louis Dionne 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_OPTIONAL_HPP
#define BOOST_HANA_OPTIONAL_HPP

#include <boost/hana/fwd/optional.hpp>

#include <boost/hana/bool.hpp>
#include <boost/hana/core/datatype.hpp>
#include <boost/hana/detail/operators/adl.hpp>
#include <boost/hana/detail/operators/comparable.hpp>
#include <boost/hana/detail/operators/monad.hpp>
#include <boost/hana/detail/operators/orderable.hpp>
#include <boost/hana/equal.hpp>
#include <boost/hana/eval_if.hpp>
#include <boost/hana/functional/compose.hpp>
#include <boost/hana/functional/id.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/fwd/any_of.hpp>
#include <boost/hana/fwd/ap.hpp>
#include <boost/hana/fwd/concat.hpp>
#include <boost/hana/fwd/core/make.hpp>
#include <boost/hana/fwd/empty.hpp>
#include <boost/hana/fwd/find_if.hpp>
#include <boost/hana/fwd/flatten.hpp>
#include <boost/hana/fwd/lift.hpp>
#include <boost/hana/fwd/transform.hpp>
#include <boost/hana/fwd/type.hpp>
#include <boost/hana/fwd/unpack.hpp>
#include <boost/hana/lazy.hpp>
#include <boost/hana/less.hpp>

#include <type_traits>
#include <utility>


namespace boost { namespace hana {
    //////////////////////////////////////////////////////////////////////////
    // optional<>
    //////////////////////////////////////////////////////////////////////////
    namespace maybe_detail {
        template <typename T, typename = typename datatype<T>::type>
        struct nested_type { };

        template <typename T>
        struct nested_type<T, Type> { using type = typename T::type; };
    }

    template <typename T>
    struct optional<T> : operators::adl, maybe_detail::nested_type<T> {
        T val;
        static constexpr bool is_just = true;

        optional() = default;
        optional(optional const&) = default;
        optional(optional&&) = default;
        optional(optional&) = default;

        template <typename U, typename = decltype(T(std::declval<U>()))>
        explicit constexpr optional(U&& u)
            : val(static_cast<U&&>(u))
        { }

        constexpr T& operator*() & { return this->val; }
        constexpr T const& operator*() const& { return this->val; }
        constexpr T operator*() && { return std::move(this->val); }

        constexpr T* operator->() & { return &this->val; }
        constexpr T const* operator->() const& { return &this->val; }
    };

    //! @cond
    template <typename T>
    constexpr auto make_just_t::operator()(T&& t) const {
        return optional<typename std::decay<T>::type>(
            static_cast<T&&>(t)
        );
    }
    //! @endcond

    template <typename ...T>
    struct datatype<optional<T...>> {
        using type = Optional;
    };

    //////////////////////////////////////////////////////////////////////////
    // make<Optional>
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct make_impl<Optional> {
        template <typename X>
        static constexpr auto apply(X&& x)
        { return hana::just(static_cast<X&&>(x)); }

        static constexpr auto apply()
        { return hana::nothing; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <>
        struct comparable_operators<Optional> {
            static constexpr bool value = true;
        };
        template <>
        struct orderable_operators<Optional> {
            static constexpr bool value = true;
        };
        template <>
        struct monad_operators<Optional> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // is_just and is_nothing
    //////////////////////////////////////////////////////////////////////////
    // Remove warnings generated by poor confused Doxygen
    //! @cond

    template <typename M>
    constexpr auto is_just_t::operator()(M const&) const
    { return hana::bool_<M::is_just>; }

    template <typename M>
    constexpr auto is_nothing_t::operator()(M const&) const
    { return hana::bool_<!M::is_just>; }

    //////////////////////////////////////////////////////////////////////////
    // from_maybe and from_just
    //////////////////////////////////////////////////////////////////////////
    template <typename Default, typename M>
    constexpr decltype(auto) from_maybe_t::operator()(Default&& default_, M&& m) const {
        return hana::maybe(static_cast<Default&&>(default_), hana::id,
                                            static_cast<M&&>(m));
    }

    template <typename M>
    constexpr decltype(auto) from_just_t::operator()(M&& m) const {
        static_assert(std::remove_reference<M>::type::is_just,
        "trying to extract the value inside a boost::hana::nothing "
        "with boost::hana::from_just");
        return hana::id(static_cast<M&&>(m).val);
    }

    //////////////////////////////////////////////////////////////////////////
    // only_when
    //////////////////////////////////////////////////////////////////////////
    template <typename Pred, typename F, typename X>
    constexpr decltype(auto) only_when_t::operator()(Pred&& pred, F&& f, X&& x) const {
        return hana::eval_if(static_cast<Pred&&>(pred)(x),
            hana::make_lazy(hana::compose(hana::just, static_cast<F&&>(f)))(
                static_cast<X&&>(x)
            ),
            hana::make_lazy(hana::nothing)
        );
    }

    //////////////////////////////////////////////////////////////////////////
    // sfinae
    //////////////////////////////////////////////////////////////////////////
    namespace maybe_detail {
        struct sfinae_impl {
            template <typename F, typename ...X, typename = decltype(
                std::declval<F>()(std::declval<X>()...)
            )>
            constexpr decltype(auto) operator()(int, F&& f, X&& ...x) const {
                using Return = decltype(static_cast<F&&>(f)(static_cast<X&&>(x)...));
                static_assert(!std::is_same<Return, void>::value,
                "hana::sfinae(f)(args...) requires f(args...) to be non-void");

                return hana::just(static_cast<F&&>(f)(static_cast<X&&>(x)...));
            }

            template <typename F, typename ...X>
            constexpr auto operator()(long, F&&, X&& ...) const
            { return hana::nothing; }
        };
    }

    template <typename F>
    constexpr decltype(auto) sfinae_t::operator()(F&& f) const {
        return hana::partial(maybe_detail::sfinae_impl{}, int{},
                             static_cast<F&&>(f));
    }

    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<Optional, Optional> {
        template <typename T, typename U>
        static constexpr auto apply(hana::optional<T> const& t, hana::optional<U> const& u)
        { return hana::equal(t.val, u.val); }

        static constexpr auto apply(hana::optional<> const&, hana::optional<> const&)
        { return hana::true_; }

        template <typename T, typename U>
        static constexpr auto apply(T const&, U const&)
        { return hana::false_; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Orderable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct less_impl<Optional, Optional> {
        template <typename T>
        static constexpr auto apply(hana::optional<> const&, hana::optional<T> const&)
        { return hana::true_; }

        static constexpr auto apply(hana::optional<> const&, hana::optional<> const&)
        { return hana::false_; }

        template <typename T>
        static constexpr auto apply(hana::optional<T> const&, hana::optional<> const&)
        { return hana::false_; }

        template <typename T, typename U>
        static constexpr auto apply(hana::optional<T> const& x, hana::optional<U> const& y)
        { return hana::less(x.val, y.val); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Functor
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct transform_impl<Optional> {
        template <typename M, typename F>
        static constexpr decltype(auto) apply(M&& m, F&& f) {
            return hana::maybe(
                hana::nothing,
                hana::compose(hana::just, static_cast<F&&>(f)),
                static_cast<M&&>(m)
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Applicative
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct lift_impl<Optional> {
        template <typename X>
        static constexpr decltype(auto) apply(X&& x)
        { return hana::just(static_cast<X&&>(x)); }
    };

    template <>
    struct ap_impl<Optional> {
        template <typename F, typename X>
        static constexpr decltype(auto) apply_impl(F&& f, X&& x, decltype(hana::true_)) {
            return hana::just(static_cast<F&&>(f).val(static_cast<X&&>(x).val));
        }

        template <typename F, typename X>
        static constexpr auto apply_impl(F&&, X&&, decltype(false_))
        { return hana::nothing; }

        template <typename F, typename X>
        static constexpr auto apply(F&& f, X&& x) {
            return ap_impl::apply_impl(
                static_cast<F&&>(f), static_cast<X&&>(x),
                hana::bool_<
                    decltype(hana::is_just(f))::value &&
                    decltype(hana::is_just(x))::value
                >
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Monad
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct flatten_impl<Optional> {
        template <typename MMX>
        static constexpr decltype(auto) apply(MMX&& mmx) {
            return hana::maybe(hana::nothing, hana::id, static_cast<MMX&&>(mmx));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // MonadPlus
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct concat_impl<Optional> {
        template <typename Y>
        static constexpr auto apply(hana::optional<>&, Y&& y)
        { return static_cast<Y&&>(y); }

        template <typename Y>
        static constexpr auto apply(hana::optional<>&&, Y&& y)
        { return static_cast<Y&&>(y); }

        template <typename Y>
        static constexpr auto apply(hana::optional<> const&, Y&& y)
        { return static_cast<Y&&>(y); }

        template <typename X, typename Y>
        static constexpr auto apply(X&& x, Y&&)
        { return static_cast<X&&>(x); }
    };

    template <>
    struct empty_impl<Optional> {
        static constexpr auto apply()
        { return hana::nothing; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct unpack_impl<Optional> {
        template <typename M, typename F>
        static constexpr decltype(auto) apply(M&& m, F&& f)
        { return static_cast<F&&>(f)(static_cast<M&&>(m).val); }

        template <typename F>
        static constexpr decltype(auto) apply(hana::optional<> const&, F&& f)
        { return static_cast<F&&>(f)(); }

        template <typename F>
        static constexpr decltype(auto) apply(hana::optional<>&&, F&& f)
        { return static_cast<F&&>(f)(); }

        template <typename F>
        static constexpr decltype(auto) apply(hana::optional<>&, F&& f)
        { return static_cast<F&&>(f)(); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Searchable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct find_if_impl<Optional> {
        template <typename M, typename Pred>
        static constexpr decltype(auto) apply(M&& m, Pred&& pred) {
            return hana::only_when(static_cast<Pred&&>(pred), id,
                                        static_cast<M&&>(m).val);
        }

        template <typename Pred>
        static constexpr auto apply(hana::optional<> const&, Pred&&)
        { return hana::nothing; }

        template <typename Pred>
        static constexpr auto apply(hana::optional<>&&, Pred&&)
        { return hana::nothing; }

        template <typename Pred>
        static constexpr auto apply(hana::optional<>&, Pred&&)
        { return hana::nothing; }
    };

    template <>
    struct any_of_impl<Optional> {
        template <typename M, typename Pred>
        static constexpr decltype(auto) apply(M&& m, Pred&& p) {
            return hana::maybe(hana::false_,
                static_cast<Pred&&>(p),
                static_cast<M&&>(m)
            );
        }
    };
}} // end namespace boost::hana

#endif // !BOOST_HANA_OPTIONAL_HPP