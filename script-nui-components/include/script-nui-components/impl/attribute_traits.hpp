#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/range.hpp>

namespace ScriptNuiComponents::Detail
{
    template <typename T>
    struct AttributeTraitsImpl
    {
        using Type = T;
        using Actual = T;
        constexpr static bool isObserved = false;

        static Type getValue(T const& value)
        {
            return value;
        }
        template <typename U>
        static void assignValue(T& attribute, U&& value)
        {
            attribute = std::forward<U>(value);
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
    };

    template <typename T>
    struct AttributeTraitsImpl<Nui::Observed<T>>
    {
        using Type = T;
        using Actual = std::reference_wrapper<Nui::Observed<T>>;
        constexpr static bool isObserved = true;

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
    };

    template <typename T>
    struct AttributeTraitsImpl<std::shared_ptr<Nui::Observed<T>>>
    {
        using Type = T;
        using Actual = std::weak_ptr<Nui::Observed<T>>;
        constexpr static bool isObserved = true;

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
    };

    template <typename T>
    using AttributeTraits = AttributeTraitsImpl<std::decay_t<T>>;
}