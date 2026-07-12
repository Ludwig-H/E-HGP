"""Allow ``python -m benchmarks`` from the perg_hgp project directory."""

from .cli import main


if __name__ == "__main__":
    raise SystemExit(main())
