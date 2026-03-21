#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/range.hpp>

#include <map>
#include <unordered_map>

namespace ScriptNuiComponents::Detail
{
    template <typename T>
    struct IsMapImpl : std::false_type
    {};

    template <typename... TArgs>
    struct IsMapImpl<std::map<TArgs...>> : std::true_type
    {};
    template <typename... TArgs>
    struct IsMapImpl<std::unordered_map<TArgs...>> : std::true_type
    {};

    template <typename T>
    inline constexpr bool IsMap = IsMapImpl<std::decay_t<T>>::value;

    template <typename T>
    struct AttributeTraitsImpl
    {
        using Type = T;
        using Actual = T;
        constexpr static bool isObserved = false;
        constexpr static bool isMap = IsMap<T>;

        static Type getValue(T const& value)
        {
            return value;
        }
        static auto range(Actual const& actual)
        {
            return Nui::range(actual);
        }
        static Type const& unwrap(Actual const& actual)
        {
            return actual;
        }
        static Type& unwrap(Actual& actual)
        {
            return actual;
        }
        static auto observe(Actual const& actual, auto&& fn)
        {
            return fn(actual);
        }
    };

    template <typename T>
    struct AttributeTraitsImpl<Nui::Observed<T>>
    {
        using Type = T;
        using Actual = std::reference_wrapper<Nui::Observed<T>>;
        constexpr static bool isObserved = true;
        constexpr static bool isMap = IsMap<T>;

        static Type getValue(Actual observed)
        {
            return observed.get().value();
        }
        template <typename U>
        static void assignValue(Actual observed, U&& value)
        {
            observed.get() = std::forward<U>(value);
        }
        static auto range(Actual observed)
        {
            return Nui::range(observed.get());
        }
        static Type const& unwrap(Actual const& observed)
        {
            return observed.get().value();
        }
        static Type& unwrap(Actual& observed)
        {
            return observed.get().value();
        }
        static auto observe(Actual const& actual, auto&& fn)
        {
            return Nui::observe(actual.get()).generate(std::forward<decltype(fn)>(fn));
        }
    };

    template <typename T>
    struct AttributeTraitsImpl<std::shared_ptr<Nui::Observed<T>>>
    {
        using Type = T;
        using Actual = std::weak_ptr<Nui::Observed<T>>;
        constexpr static bool isObserved = true;
        constexpr static bool isMap = IsMap<T>;

        static Type getValue(Actual const& observed)
        {
            if (auto shared = observed.lock(); shared)
                return shared->value();
            return Type{};
        }
        template <typename U>
        static void assignValue(Actual& observed, U&& value)
        {
            if (auto shared = observed.lock(); shared)
                *shared = std::forward<U>(value);
        }
        static auto range(Actual observed)
        {
            if (auto shared = observed.lock(); shared)
                return Nui::range(*shared);
            return Nui::range(std::vector<typename Type::value_type>{});
        }
        static Type const& unwrap(Actual const& observed)
        {
            if (const auto shared = observed.lock(); shared)
                return shared->value();
            return Type{};
        }
        static Type& unwrap(Actual& observed)
        {
            if (auto shared = observed.lock(); shared)
                return shared->value();
            throw std::runtime_error("Observed value is not available");
        }
        static auto observe(Actual const& actual, auto&& fn)
        {
            if (auto shared = actual.lock(); shared)
                return Nui::observe(*shared).generate(std::forward<decltype(fn)>(fn));
            return fn(Type{});
        }
    };

    template <typename T>
    using AttributeTraits = AttributeTraitsImpl<std::decay_t<T>>;

    template <typename Lhs, typename Rhs, typename = void>
    struct CanAssign : std::false_type
    {};

    template <typename Lhs, typename Rhs>
    struct CanAssign<Lhs, Rhs, std::void_t<decltype(std::declval<Lhs>() = std::declval<Rhs>())>> : std::true_type
    {};
}