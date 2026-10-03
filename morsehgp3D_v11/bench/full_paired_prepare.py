#!/usr/bin/env python3
"""Verify previous qualification evidence retained on VM, without importing its binary or running product."""
import argparse
from pathlib import Path
import sys
import full_qualification_context as context


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--archive-sha256', required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        context.capture(args.archive, args.archive_sha256, args.out)
        return 0
    except (OSError, ValueError, KeyError, TypeError) as error:
        print('prior_qualification_refused: ' + str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
