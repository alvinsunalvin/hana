/*!
@file
Defines `boost::hana::Maybe`.

@copyright Louis Dionne 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_MAYBE_HPP
#define BOOST_HANA_MAYBE_HPP

#include <boost/hana/fwd/maybe.hpp>

#include <boost/hana/applicative.hpp>
#include <boost/hana/bool.hpp>
#include <boost/hana/comparable.hpp>
#include <boost/hana/core/datatype.hpp>
#include <boost/hana/core/operators.hpp>
#include <boost/hana/detail/std/decay.hpp>
#include <boost/hana/detail/std/declval.hpp>
#include <boost/hana/detail/std/forward.hpp>
#include <boost/hana/detail/std/integral_constant.hpp>
#include <boost/hana/detail/std/is_same.hpp>
#include <boost/hana/detail/std/move.hpp>
#include <boost/hana/detail/std/remove_reference.hpp>
#include <boost/hana/foldable.hpp>
#include <boost/hana/functional/always.hpp>
#include <boost/hana/functional/compose.hpp>
#include <boost/hana/functional/id.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/functor.hpp>
#include <boost/hana/fwd/type.hpp>
#include <boost/hana/lazy.hpp>
#include <boost/hana/logical.hpp>
#include <boost/hana/monad.hpp>
#include <boost/hana/monad_plus.hpp>
#include <boost/hana/orderable.hpp>
#include <boost/hana/searchable.hpp>
#include <boost/hana/traversable.hpp>


namespace boost { namespace hana {
    //////////////////////////////////////////////////////////////////////////
    // _just
    //////////////////////////////////////////////////////////////////////////
    namespace maybe_detail {
        template <typename T, typename = typename datatype<T>::type>
        struct nested_type { };

        template <typename T>
        struct nested_type<T, Type> { using type = typename T::type; };
    }

    template <typename T>
    struct _just : operators::adl, maybe_detail::nested_type<T> {
        T val;
        static constexpr bool is_just = true;
        struct hana { using datatype = Maybe; };

        _just() = default;
        _just(_just const&) = default;
        _just(_just&&) = default;
        _just(_just&) = default;

        template <typename U, typename = decltype(T(detail::std::declval<U>()))>
        constexpr _just(U&& u)
            : val(static_cast<U&&>(u))
        { }

        constexpr T& operator*() & { return this->val; }
        constexpr T const& operator*() const& { return this->val; }
        constexpr T operator*() && { return detail::std::move(this->val); }

        constexpr T* operator->() & { return &this->val; }
        constexpr T const* operator->() const& { return &this->val; }
    };

    //! @cond
    template <typename T>
    constexpr auto _make_just::operator()(T&& t) const {
        return _just<typename detail::std::decay<T>::type>(
            static_cast<T&&>(t)
        );
    }
    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct operators::of<Maybe>
        : operators::of<Comparable, Orderable, Monad>
    { };

    //////////////////////////////////////////////////////////////////////////
    // is_just and is_nothing
    //////////////////////////////////////////////////////////////////////////
    // Remove warnings generated by poor confused Doxygen
    //! @cond

    template <typename M>
    constexpr decltype(auto) _is_just::operator()(M const&) const
    { return bool_<M::is_just>; }

    template <typename M>
    constexpr decltype(auto) _is_nothing::operator()(M const&) const
    { return bool_<!M::is_just>; }

    //////////////////////////////////////////////////////////////////////////
    // from_maybe and from_just
    //////////////////////////////////////////////////////////////////////////
    template <typename Default, typename M>
    constexpr decltype(auto) _from_maybe::operator()(Default&& default_, M&& m) const {
        return hana::maybe(static_cast<Default&&>(default_), id,
                                            static_cast<M&&>(m));
    }

    template <typename M>
    constexpr decltype(auto) _from_just::operator()(M&& m) const {
        static_assert(detail::std::remove_reference<M>::type::is_just,
        "trying to extract the value inside a boost::hana::nothing "
        "with boost::hana::from_just");
        return hana::id(static_cast<M&&>(m).val);
    }

    //////////////////////////////////////////////////////////////////////////
    // only_when
    //////////////////////////////////////////////////////////////////////////
    template <typename Pred, typename F, typename X>
    constexpr decltype(auto) _only_when::operator()(Pred&& pred, F&& f, X&& x) const {
        return hana::eval_if(static_cast<Pred&&>(pred)(x),
            hana::lazy(hana::compose(just, static_cast<F&&>(f)))(
                static_cast<X&&>(x)
            ),
            hana::lazy(nothing)
        );
    }

    //////////////////////////////////////////////////////////////////////////
    // sfinae
    //////////////////////////////////////////////////////////////////////////
    namespace maybe_detail {
        struct sfinae_impl {
            template <typename F, typename ...X, typename = decltype(
                detail::std::declval<F>()(detail::std::declval<X>()...)
            )>
            constexpr decltype(auto) operator()(int, F&& f, X&& ...x) const {
                constexpr bool returns_void = detail::std::is_same<
                    void, decltype(
                        static_cast<F&&>(f)(static_cast<X&&>(x)...)
                    )
                >{};
                static_assert(!returns_void,
                "hana::sfinae(f)(args...) requires f(args...) to be non-void");
                return hana::just(
                    static_cast<F&&>(f)(static_cast<X&&>(x)...)
                );
            }

            template <typename F, typename ...X>
            constexpr auto operator()(long, F&&, X&& ...) const {
                return hana::nothing;
            }
        };
    }

    template <typename F>
    constexpr decltype(auto) _sfinae::operator()(F&& f) const {
        return hana::partial(maybe_detail::sfinae_impl{}, int{},
                             static_cast<F&&>(f));
    }

    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<Maybe, Maybe> {
        template <typename T, typename U>
        static constexpr decltype(auto) apply(_just<T> const& t, _just<U> const& u)
        { return hana::equal(t.val, u.val); }

        static constexpr auto apply(_nothing const&, _nothing const&)
        { return true_; }

        template <typename T, typename U>
        static constexpr auto apply(T const&, U const&)
        { return false_; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Orderable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct less_impl<Maybe, Maybe> {
        template <typename T>
        static constexpr auto apply(_nothing const&, _just<T> const&)
        { return true_; }

        static constexpr auto apply(_nothing const&, _nothing const&)
        { return false_; }

        template <typename T>
        static constexpr auto apply(_just<T> const&, _nothing const&)
        { return false_; }

        template <typename T, typename U>
        static constexpr auto apply(_just<T> const& x, _just<U> const& y)
        { return hana::less(x.val, y.val); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Functor
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct transform_impl<Maybe> {
        template <typename M, typename F>
        static constexpr decltype(auto) apply(M&& m, F&& f) {
            return hana::maybe(
                nothing,
                hana::compose(just, static_cast<F&&>(f)),
                static_cast<M&&>(m)
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Applicative
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct lift_impl<Maybe> {
        template <typename X>
        static constexpr decltype(auto) apply(X&& x)
        { return hana::just(static_cast<X&&>(x)); }
    };

    template <>
    struct ap_impl<Maybe> {
        template <typename F, typename X>
        static constexpr decltype(auto) apply_impl(F&& f, X&& x, detail::std::true_type) {
            return hana::just(static_cast<F&&>(f).val(static_cast<X&&>(x).val));
        }

        template <typename F, typename X>
        static constexpr auto apply_impl(F&&, X&&, detail::std::false_type)
        { return nothing; }

        template <typename F, typename X>
        static constexpr auto apply(F&& f, X&& x) {
            auto f_is_just = hana::is_just(f);
            auto x_is_just = hana::is_just(x);
            return apply_impl(
                static_cast<F&&>(f), static_cast<X&&>(x),
                detail::std::integral_constant<bool,
                    hana::value(f_is_just) && hana::value(x_is_just)
                >{}
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Monad
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct flatten_impl<Maybe> {
        template <typename MMX>
        static constexpr decltype(auto) apply(MMX&& mmx) {
            return hana::maybe(nothing, id, static_cast<MMX&&>(mmx));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // MonadPlus
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct concat_impl<Maybe> {
        template <typename Y>
        static constexpr auto apply(_nothing, Y&& y)
        { return static_cast<Y&&>(y); }

        template <typename X, typename Y>
        static constexpr auto apply(X&& x, Y const&)
        { return static_cast<X&&>(x); }
    };

    template <>
    struct empty_impl<Maybe> {
        static constexpr auto apply()
        { return nothing; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Traversable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct traverse_impl<Maybe> {
        template <typename A, typename F>
        static constexpr decltype(auto) apply(_nothing const&, F&& f)
        { return lift<A>(nothing); }

        template <typename A, typename T, typename F>
        static constexpr decltype(auto) apply(_just<T> const& x, F&& f)
        { return hana::transform(static_cast<F&&>(f)(x.val), just); }

        template <typename A, typename T, typename F>
        static constexpr decltype(auto) apply(_just<T>& x, F&& f)
        { return hana::transform(static_cast<F&&>(f)(x.val), just); }

        template <typename A, typename T, typename F>
        static constexpr decltype(auto) apply(_just<T>&& x, F&& f) {
            return hana::transform(static_cast<F&&>(f)(
                                        detail::std::move(x.val)), just);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct unpack_impl<Maybe> {
        template <typename M, typename F>
        static constexpr decltype(auto) apply(M&& m, F&& f)
        { return static_cast<F&&>(f)(static_cast<M&&>(m).val); }

        template <typename F>
        static constexpr decltype(auto) apply(_nothing const&, F&& f)
        { return static_cast<F&&>(f)(); }

        template <typename F>
        static constexpr decltype(auto) apply(_nothing&&, F&& f)
        { return static_cast<F&&>(f)(); }

        template <typename F>
        static constexpr decltype(auto) apply(_nothing&, F&& f)
        { return static_cast<F&&>(f)(); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Searchable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct find_if_impl<Maybe> {
        template <typename M, typename Pred>
        static constexpr decltype(auto) apply(M&& m, Pred&& pred) {
            return hana::only_when(static_cast<Pred&&>(pred), id,
                                        static_cast<M&&>(m).val);
        }

        template <typename Pred>
        static constexpr decltype(auto) apply(_nothing const&, Pred&&)
        { return nothing; }

        template <typename Pred>
        static constexpr decltype(auto) apply(_nothing&&, Pred&&)
        { return nothing; }

        template <typename Pred>
        static constexpr decltype(auto) apply(_nothing&, Pred&&)
        { return nothing; }
    };

    template <>
    struct any_of_impl<Maybe> {
        template <typename M, typename Pred>
        static constexpr decltype(auto) apply(M&& m, Pred&& p) {
            return hana::maybe(false_,
                static_cast<Pred&&>(p),
                static_cast<M&&>(m)
            );
        }
    };
}} // end namespace boost::hana

#endif // !BOOST_HANA_MAYBE_HPP
