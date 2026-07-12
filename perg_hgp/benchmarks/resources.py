"""Low-overhead RSS and optional per-process NVML peak sampling."""

from __future__ import annotations

import os
import resource
import threading
import time
from dataclasses import dataclass
from typing import Any


def _proc_rss_bytes() -> int | None:
    try:
        import psutil

        return int(psutil.Process(os.getpid()).memory_info().rss)
    except (ImportError, OSError):
        pass
    try:
        with open("/proc/self/statm", "r", encoding="ascii") as stream:
            resident_pages = int(stream.read().split()[1])
        return resident_pages * os.sysconf("SC_PAGE_SIZE")
    except (OSError, ValueError, IndexError):
        return None


def _lifetime_peak_rss_bytes() -> int | None:
    try:
        value = int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)
    except (ValueError, OSError):
        return None
    # Linux reports KiB; macOS reports bytes.
    return value if os.uname().sysname == "Darwin" else value * 1024


@dataclass
class _NVMLSampler:
    module: Any
    handles: list[Any]
    devices: list[dict[str, Any]]

    @classmethod
    def create(cls) -> tuple["_NVMLSampler | None", str | None]:
        try:
            import pynvml as nvml
        except ImportError:
            return None, "pynvml_not_installed"
        try:
            nvml.nvmlInit()
            handles = []
            devices = []
            for index in range(int(nvml.nvmlDeviceGetCount())):
                handle = nvml.nvmlDeviceGetHandleByIndex(index)
                name = nvml.nvmlDeviceGetName(handle)
                uuid = nvml.nvmlDeviceGetUUID(handle)
                handles.append(handle)
                devices.append(
                    {
                        "index": index,
                        "name": name.decode() if isinstance(name, bytes) else str(name),
                        "uuid": uuid.decode() if isinstance(uuid, bytes) else str(uuid),
                    }
                )
            return cls(nvml, handles, devices), None
        except Exception as error:  # NVML has version-specific exception types.
            try:
                nvml.nvmlShutdown()
            except Exception:
                pass
            return None, f"nvml_initialization_failed: {type(error).__name__}: {error}"

    def sample(self, pid: int) -> tuple[int, int]:
        process_used = 0
        device_used = 0
        for handle in self.handles:
            try:
                device_used += int(self.module.nvmlDeviceGetMemoryInfo(handle).used)
            except Exception:
                pass
            processes = None
            for function_name in (
                "nvmlDeviceGetComputeRunningProcesses_v3",
                "nvmlDeviceGetComputeRunningProcesses_v2",
                "nvmlDeviceGetComputeRunningProcesses",
            ):
                function = getattr(self.module, function_name, None)
                if function is None:
                    continue
                try:
                    processes = function(handle)
                    break
                except Exception:
                    continue
            for process in processes or ():
                if int(getattr(process, "pid", -1)) != pid:
                    continue
                used = getattr(process, "usedGpuMemory", 0)
                if isinstance(used, int) and 0 <= used < (1 << 63):
                    process_used += used
        return process_used, device_used

    def close(self) -> None:
        try:
            self.module.nvmlShutdown()
        except Exception:
            pass


class ResourceMonitor:
    """Sample resource peaks over one isolated benchmark operation."""

    def __init__(self, interval_seconds: float = 0.05):
        if interval_seconds <= 0.0:
            raise ValueError("interval_seconds must be positive")
        self.interval_seconds = float(interval_seconds)
        self.pid = os.getpid()
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._nvml: _NVMLSampler | None = None
        self._nvml_error: str | None = None
        self._started_at = 0.0
        self._stopped_at = 0.0
        self.samples = 0
        self.rss_baseline_bytes = _proc_rss_bytes()
        self.rss_peak_bytes = self.rss_baseline_bytes
        self.process_vram_baseline_bytes: int | None = None
        self.process_vram_peak_bytes: int | None = None
        self.device_used_baseline_bytes: int | None = None
        self.device_used_peak_bytes: int | None = None

    def __enter__(self) -> "ResourceMonitor":
        self._nvml, self._nvml_error = _NVMLSampler.create()
        if self._nvml is not None:
            process, device = self._nvml.sample(self.pid)
            self.process_vram_baseline_bytes = process
            self.process_vram_peak_bytes = process
            self.device_used_baseline_bytes = device
            self.device_used_peak_bytes = device
        self._started_at = time.perf_counter()
        self._sample()
        self._thread = threading.Thread(
            target=self._loop, name="benchmark-resource-monitor", daemon=True
        )
        self._thread.start()
        return self

    def _sample(self) -> None:
        rss = _proc_rss_bytes()
        if rss is not None:
            self.rss_peak_bytes = max(self.rss_peak_bytes or 0, rss)
        if self._nvml is not None:
            process, device = self._nvml.sample(self.pid)
            self.process_vram_peak_bytes = max(
                self.process_vram_peak_bytes or 0, process
            )
            self.device_used_peak_bytes = max(
                self.device_used_peak_bytes or 0, device
            )
        self.samples += 1

    def _loop(self) -> None:
        while not self._stop.wait(self.interval_seconds):
            self._sample()

    def __exit__(self, exc_type: Any, exc: Any, traceback: Any) -> None:
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=max(1.0, 4.0 * self.interval_seconds))
        self._sample()
        self._stopped_at = time.perf_counter()
        if self._nvml is not None:
            self._nvml.close()

    def to_dict(self) -> dict[str, Any]:
        baseline_rss = self.rss_baseline_bytes
        peak_rss = self.rss_peak_bytes
        baseline_vram = self.process_vram_baseline_bytes
        peak_vram = self.process_vram_peak_bytes
        return {
            "sampling_interval_seconds": self.interval_seconds,
            "samples": self.samples,
            "monitor_wall_seconds": max(0.0, self._stopped_at - self._started_at),
            "rss_baseline_bytes": baseline_rss,
            "rss_peak_sampled_bytes": peak_rss,
            "rss_peak_increment_bytes": (
                None
                if baseline_rss is None or peak_rss is None
                else max(0, peak_rss - baseline_rss)
            ),
            "rss_peak_process_lifetime_bytes": _lifetime_peak_rss_bytes(),
            "nvml_available": self._nvml is not None,
            "nvml_error": self._nvml_error,
            "nvml_devices": [] if self._nvml is None else self._nvml.devices,
            "process_vram_baseline_bytes": baseline_vram,
            "process_vram_peak_bytes": peak_vram,
            "process_vram_peak_increment_bytes": (
                None
                if baseline_vram is None or peak_vram is None
                else max(0, peak_vram - baseline_vram)
            ),
            # Device-wide values are explicitly named: other processes can
            # contribute, so this is not presented as process VRAM.
            "device_used_baseline_bytes": self.device_used_baseline_bytes,
            "device_used_peak_bytes": self.device_used_peak_bytes,
        }

