#!/usr/bin/env python3
"""Validate links and repository-specific Markdown conventions."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[1]
MARKDOWN_LINK = re.compile(r"!?\[[^\]]*\]\(([^)\s]+)")
INLINE_CODE = re.compile(r"`[^`]*`")
EXPLICIT_BRACES = re.compile(r"\\(?:mathbb|mathbf|frac|sqrt)(?!\{)")
BANNED_LATEX = (
    r"\operatorname",
    r"\left\|",
    r"\right\|",
    r"\left\{",
    r"\right\}",
)
AMPUTATED_LATEX = (
    re.compile(r"(?<!\\)\bqquad\b"),
    re.compile(r"(?<!\\)\bpi_0\b"),
    re.compile(r"(?<!\\)\bmathrm\b"),
)


def active_markdown() -> list[Path]:
    """Return the maintained Markdown corpus, excluding the historical code."""

    paths = [
        ROOT / "README.md",
        ROOT / "AGENTS.md",
        ROOT / "gcp-migration" / "README.md",
    ]
    paths.extend(sorted((ROOT / "docs").rglob("*.md")))
    # Le chantier v4 porte aujourd'hui toute la mathematique active, et la
    # regle « une equation par ligne physique, accolades explicites, pas de
    # \operatorname » n'y etait verifiee par personne : le controleur ne
    # regardait que `docs/`. Une regle que rien ne verifie n'est pas une
    # regle.
    #
    # PERIMETRE : la passation, les recus et les notes ECRITES ICI. Les
    # `AUDIT_*` / `CONTRE_AUDIT_*` / `HARMONISATION_*` et les lectures en
    # sont exclus a dessein — ce sont les mots des auditeurs, un registre
    # que l'on ne reformate pas pour faire passer un linter. (Ils portent
    # d'ailleurs des `\operatorname` et des `\frac` sans accolades : c'est
    # leur affaire, pas la ligne de production.)
    v4 = ROOT / "morsehgp3D_v4"
    paths.extend(p for p in (v4 / "PASSATION.md", v4 / "README.md") if p.is_file())
    paths.extend(sorted((v4 / "receipts").rglob("*.md")))
    paths.extend(
        p
        for p in sorted((v4 / "audits").rglob("*.md"))
        if p.name.startswith(("REPONSE_CLAUDE_", "NOTE_CLAUDE_", "REPONSE_APRES_"))
    )
    return list(dict.fromkeys(paths))


def local_target(source: Path, raw_target: str) -> Path | None:
    target = unquote(raw_target).split("#", 1)[0].split("?", 1)[0]
    if not target or target.startswith(("http://", "https://", "mailto:")):
        return None
    return (source.parent / target).resolve()


def validate(path: Path) -> list[str]:
    errors: list[str] = []
    text = path.read_text(encoding="utf-8")
    relative = path.relative_to(ROOT)
    in_fence = False

    for number, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if stripped.startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence:
            continue

        if line.endswith((" ", "\t")):
            errors.append(f"{relative}:{number}: trailing whitespace")

        lint_line = INLINE_CODE.sub("", line)
        dollar_blocks = lint_line.count("$$")
        if dollar_blocks not in (0, 2):
            errors.append(
                f"{relative}:{number}: a $$ equation must open and close on one line"
            )

        for token in BANNED_LATEX:
            if token in lint_line:
                errors.append(f"{relative}:{number}: banned LaTeX token {token}")
        for pattern in AMPUTATED_LATEX:
            match = pattern.search(lint_line)
            if match:
                errors.append(
                    f"{relative}:{number}: probable missing LaTeX backslash before {match.group(0)}"
                )
        if EXPLICIT_BRACES.search(lint_line):
            errors.append(
                f"{relative}:{number}: mathbb, mathbf, frac and sqrt require braces"
            )

        for match in MARKDOWN_LINK.finditer(line):
            target = local_target(path, match.group(1))
            if target is not None and not target.exists():
                errors.append(
                    f"{relative}:{number}: missing local link {match.group(1)!r}"
                )

    if in_fence:
        errors.append(f"{relative}: unclosed fenced code block")
    return errors


def main() -> int:
    paths = active_markdown()
    missing = [path for path in paths if not path.is_file()]
    errors = [f"missing required document: {path.relative_to(ROOT)}" for path in missing]
    for path in paths:
        if path.is_file():
            errors.extend(validate(path))

    if errors:
        print("Documentation validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print(f"Validated {len(paths)} active Markdown files.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
