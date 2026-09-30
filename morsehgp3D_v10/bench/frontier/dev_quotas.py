"""Exact return quotas for synthetic frontier development scenes.

Communities share 84/100 of the target mass equally, with core:halo = 3:1.
At three communities this gives core = 21/100 and halo = 7/100 per
community, plus background = 16/100. Integer rounding is a single flat
largest-remainder allocation, not successive rounding of communities.
Ties use community order, core before halo, then background last.

Counts describe generated returns before quantization. No coordinates are
generated here, no duplicates are removed, and no number of unique sites
is promised. The helper uses O(groups) storage and O(groups log groups)
ordering, independent of the requested number of returns.
"""

from typing import TypedDict


class DevQuota(TypedDict):
    """One role's count and exact, unreduced target fraction."""

    community: int | None
    role: str
    returns: int
    weight_numerator: int
    weight_denominator: int


class DevAllocation(TypedDict):
    """JSON-compatible allocation; ``returns`` never means unique sites."""

    requested_returns: int
    generated_returns: int
    groups: int
    counting_unit: str
    allocation_method: str
    quotas: list[DevQuota]


def allocate_dev_quotas(n: int, groups: int = 3) -> DevAllocation:
    """Allocate exactly ``n`` generated returns with integer arithmetic only.

    Only built-in integers are accepted; bool and non-integers raise
    TypeError. Negative n or groups below one raise ValueError. Zero n
    retains every role with count zero. Every role receives its target
    floor or ceiling; the remaining returns go to the largest fractional
    remainders, using the documented stable tie order. The target ratio
    3:1 need not hold exactly in each community after integer rounding.
    """
    if type(n) is not int:
        raise TypeError("n must be a built-in integer, not bool")
    if type(groups) is not int:
        raise TypeError("groups must be a built-in integer, not bool")
    if n < 0:
        raise ValueError("n must be non-negative")
    if groups < 1:
        raise ValueError("groups must be at least one")

    denominator = 100 * groups
    quotas: list[DevQuota] = []
    for community in range(groups):
        for role, numerator in (("core", 63), ("halo", 21)):
            quotas.append({
                "community": community,
                "role": role,
                "returns": 0,
                "weight_numerator": numerator,
                "weight_denominator": denominator,
            })
    quotas.append({
        "community": None,
        "role": "background",
        "returns": 0,
        "weight_numerator": 16 * groups,
        "weight_denominator": denominator,
    })

    remainders = []
    for quota in quotas:
        count, remainder = divmod(n * quota["weight_numerator"], denominator)
        quota["returns"] = count
        remainders.append(remainder)
    remaining = n - sum(quota["returns"] for quota in quotas)
    priority = sorted(range(len(quotas)), key=lambda i: (-remainders[i], i))
    for index in priority[:remaining]:
        quotas[index]["returns"] += 1

    return {
        "requested_returns": n,
        "generated_returns": sum(quota["returns"] for quota in quotas),
        "groups": groups,
        "counting_unit": "generated_returns_before_quantization",
        "allocation_method": "stable_largest_remainders",
        "quotas": quotas,
    }
