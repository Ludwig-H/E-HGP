"""Lightweight, JSON-safe progress events for long power-cover runs."""

from __future__ import annotations

from collections.abc import Callable, Mapping
import time
from typing import Any, TypeAlias


ProgressEvent: TypeAlias = dict[str, Any]
ProgressCallback: TypeAlias = Callable[[ProgressEvent], None]


def emit_progress(
    callback: ProgressCallback | None,
    *,
    stage: str,
    kind: str,
    completed: int | None = None,
    total: int | None = None,
    unit: str | None = None,
    started_at: float | None = None,
    details: Mapping[str, Any] | None = None,
) -> None:
    """Emit one synchronous event without reading or synchronizing device state."""

    if callback is None:
        return
    event: ProgressEvent = {"stage": str(stage), "kind": str(kind)}
    if completed is not None:
        event["completed"] = int(completed)
    if total is not None:
        event["total"] = int(total)
    if unit is not None:
        event["unit"] = str(unit)
    if started_at is not None:
        event["elapsed_seconds"] = float(
            max(0.0, time.perf_counter() - float(started_at))
        )
    if details:
        # Call sites deliberately pass only built-in JSON scalar values.  Copying
        # the mapping prevents later mutation from changing an already emitted
        # event.
        event["details"] = {str(key): value for key, value in details.items()}
    callback(event)
