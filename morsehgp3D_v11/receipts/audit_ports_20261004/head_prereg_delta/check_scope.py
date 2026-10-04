#!/usr/bin/env python3
"""Only frozen JSON and Markdown are read. No product or statistical test is run."""
import hashlib
import json
from pathlib import Path

here = Path(__file__).resolve().parent
base = here / 'source/morsehgp3D_v11'
plan_path = base / 'plans/e1_prereg_synthetique_20261004.json'
doc_path = base / 'docs/SORTIE_PLATE.md'
plan = json.loads(plan_path.read_bytes())
doc = doc_path.read_text()
checks = 0


def need(value, message):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(message)


need("n'est pas significativement > 0" in plan['decision_rule']['attribution'], 'old inference condition remains')
need('gain vient de la selection, pas de la hierarchie' in plan['decision_rule']['attribution'], 'old categorical attribution remains')
need('pas de la hierarchie' in plan['predictions']['PS2'], 'PS2 retains old categorical attribution prediction')
need("n'est pas établie ; il ne prouve pas sa nullité" in doc, 'doc explicitly rejects null contribution inference')
need(len(plan['decision_rule']['conditions']) == 5, 'five quantitative/gate conditions retained')
need(plan['decision_rule']['conditions'][0] == 'p de Holm < 0,05', 'Holm threshold retained')
print(json.dumps({'verdict': 'confirmed_scope_mismatch', 'checks': checks,
    'source_pin': '723cf6e436f6cb6f524501cbd9a51bf0e17fe6fe',
    'decision_rule_attribution': plan['decision_rule']['attribution'], 'PS2': plan['predictions']['PS2'],
    'quantitative_conditions': plan['decision_rule']['conditions'],
    'sources_sha256': {str(p.relative_to(here)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (plan_path, doc_path)},
    'hypothesis_tests_or_fit_run': False,
    'action': 'Clarify attribution interpretation; preserve original predictions and thresholds. Non-significant T-A establishes no null hierarchy contribution.'},
    indent=2, sort_keys=True))
