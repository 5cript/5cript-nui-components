#pragma once

#include <string>

namespace ScriptNuiComponents
{
    enum class StyleVariant
    {
        Regular,
        Primary,
        Warning,
        Danger,
        Transparent
    };

    inline std::string toString(StyleVariant variant)
    {
        switch (variant)
        {
            case StyleVariant::Regular:
                return "regular";
            case StyleVariant::Primary:
                return "primary";
            case StyleVariant::Warning:
                return "warning";
            case StyleVariant::Danger:
                return "danger";
            case StyleVariant::Transparent:
                return "transparent";
            default:
                return "regular";
        }
    }
}