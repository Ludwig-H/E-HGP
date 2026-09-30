from fractions import Fraction

limit = 2**18 - 1
margin = Fraction(1, 50)
threshold_rounding = Fraction(1, 2**14)
for bits in (53, 52):
    epsilon = Fraction(1, 2**bits)
    relative_ratio = (1 + epsilon)**2 / (1 - epsilon) - 1
    ratio_ok = relative_ratio < Fraction(301, 100) * epsilon
    centre_error = Fraction(402, 100) * epsilon * limit
    gamma5 = 5 * epsilon / (1 - 5 * epsilon)
    error = gamma5 * 3 * (limit + centre_error)**2 + 3 * centre_error * (2 * limit + centre_error)
    safe = ratio_ok and 2 * error + threshold_rounding < margin
    print(f"epsilon=2^-{bits} ratio_bound_ok={ratio_ok} centre_error={float(centre_error):.17g} distance_error={float(error):.17g} two_errors_plus_threshold={float(2*error+threshold_rounding):.17g} margin={float(margin):.17g} safe={safe}")
    print(f"distance_error_exact={error.numerator}/{error.denominator}")
    if not safe:
        raise SystemExit(1)
print("margin_fraction_ok")
