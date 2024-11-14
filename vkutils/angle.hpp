#pragma once

namespace vkutils {
    template<typename tScalar>
    constexpr tScalar pi = tScalar(3.14159265358979323846);

    template<typename tScalar>
    constexpr tScalar deg_to_rad(tScalar aDegrees) {
        return aDegrees * pi<tScalar> / tScalar(180);
    }

    template<typename tScalar>
    constexpr tScalar rad_to_deg(tScalar aRadians) {
        return aRadians * tScalar(180) / pi<tScalar>;
    }


    template<typename tScalar>
    class Degrees;
    template<typename tScalar>
    class Radians;

    template<typename tScalar>
    class Degrees {
    public:
        constexpr explicit Degrees(tScalar aValue = tScalar(0)) noexcept
            : mValue(aValue) {
        }

        constexpr Degrees(const Degrees& aOther) noexcept
            : mValue(aOther.mValue) {
        }

        constexpr Degrees& operator=(const Degrees& aOther) noexcept {
            mValue = aOther.mValue;
            return *this;
        }

        constexpr Degrees(const Radians<tScalar>& aRadians) noexcept
            : mValue(rad_to_deg(aRadians.value())) {
        }

        constexpr Degrees& operator=(const Radians<tScalar>& aRadians) noexcept {
            mValue = rad_to_deg(aRadians.value());
            return *this;
        }

        constexpr tScalar value() const noexcept {
            return mValue;
        }

    private:
        tScalar mValue;
    };

    template<typename tScalar>
    class Radians {
    public:
        constexpr explicit Radians(tScalar aValue = tScalar(0)) noexcept
            : mValue(aValue) {
        }

        constexpr Radians(const Radians& aOther) noexcept
            : mValue(aOther.mValue) {
        }

        constexpr Radians& operator=(const Radians& aOther) noexcept {
            mValue = aOther.mValue;
            return *this;
        }

        constexpr Radians(const Degrees<tScalar>& aDegrees) noexcept
            : mValue(deg_to_rad(aDegrees.value())) {
        }

        constexpr Radians& operator=(const Degrees<tScalar>& aDegrees) noexcept {
            mValue = deg(aDegrees.value());
            return *this;
        }

        constexpr tScalar value() const noexcept {
            return mValue;
        }

    private:
        tScalar mValue;
    };


    using Degreesf = Degrees<float>;
    using Radiansf = Radians<float>;

    template<typename tScalar>
    constexpr tScalar to_degrees(Degrees<tScalar> aValue) noexcept {
        return aValue.value();
    }

    template<typename tScalar>
    constexpr tScalar to_radians(Radians<tScalar> aValue) noexcept {
        return aValue.value();
    }


    inline namespace literals {
        constexpr Radiansf operator "" _radf(const long double value) noexcept {
            return Radiansf(static_cast<float>(value));
        }

        constexpr Degreesf operator "" _degf(const long double value) noexcept {
            return Degreesf(static_cast<float>(value));
        }
    }
}
