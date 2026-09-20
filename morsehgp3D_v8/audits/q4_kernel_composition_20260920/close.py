#!/usr/bin/env python3
"""Read-only final closure of this audit; no product execution or mutation."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from campaign import BASE, ROOT, load, require, sha, source_pins

# campaign imports historical fixture helpers from another audit directory.
sys.path.insert(0, str(BASE))
from positive_campaign import fixture_checks


def replay(command, expected):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=180)
    require(result.returncode == 0 and not result.stderr, 'Reader/model failed: ' + str(command))
    require(json.loads(result.stdout) == expected, 'Reader/model differs: ' + str(command))
    return dict(command=command, returncode=result.returncode,
                stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest())


def main():
    normal, optimized = load(BASE/'READ_normal.json.gz'), load(BASE/'READ_optimized.json.gz')
    positive = load(BASE/'POSITIVE_READ_normal.json.gz')
    require(normal == optimized, 'Main readback modes differ')
    require(positive == load(BASE/'POSITIVE_READ_optimized.json.gz'), 'Positive readback modes differ')
    checks = []
    for flags in (['-B'], ['-B', '-O']):
        checks.append(replay(['python3', *flags, str(BASE/'read.py')], normal))
        checks.append(replay(['python3', *flags, str(BASE/'positive_campaign.py'), 'read'], positive))

    qualification = load(BASE/'qualification/MANIFEST.json')
    require(qualification['counts'] == dict(calls=192, max_shell=30, outputs=60,
                                           point_tests=1041, tetrahedra=5769), 'Qualification differs')
    require(normal['qualification_records'] == 200 and normal['measurement_records'] == 90,
            'Main campaign cardinal differs')
    require(positive['records'] == 36, 'Supplement cardinal differs')
    require(sum(r['emitted'] for r in normal['rows']) == 0, 'First sample no longer rejection only')
    require(sum(r['emitted'] for r in positive['rows']) == 54, 'Productive emissions differ')
    require(sum(r['shell_ids'] for r in positive['rows']) == 216, 'Productive shell incidences differ')
    require(sum(r['required_present'] for r in positive['rows']) == 20, 'Productive target count differs')
    require(len(source_pins()) == 65, 'Compilation dependency count differs')

    window = load(BASE/'WINDOW_CHECKS.json')
    require(window['source_sha256'] == sha(BASE/'window_gate.py'), 'Window model changed')
    require(len(window['records']) == 2, 'Window checks missing')
    models = []
    for record in window['records']:
        require(record['returncode'] == 0 and not record['stderr'], 'Window model failed')
        models.append(json.loads(record['stdout']))
        checks.append(replay(record['command'], models[-1]))
    require(models[0] == models[1] and models[0]['cases'] == 508, 'Window modes/cardinal differ')

    discovery = fixture_checks()
    require(discovery['all_targets_met'], 'Discovery incomplete')
    preflight = load(BASE/'SUPPLEMENT_PREFLIGHT.json')
    require(preflight['status'] == 'failed_before_any_engine_call' and preflight['returncode'] == 1,
            'Supplement initial failure missing')
    require(hashlib.sha256(preflight['source'].encode()).hexdigest() == preflight['source_sha256'],
            'Initial runner snapshot changed')

    review = load(BASE/'PRODUCT_PINS.json')
    for group in ('source_sha256', 'document_sha256'):
        for filename, digest in review[group].items():
            require(sha(ROOT/filename) == digest, 'Reviewed file changed: ' + filename)
    for filename, digest in review['source_sha256'].items():
        result = subprocess.run(['git', 'show', review['source_commit'] + ':' + filename],
                                cwd=ROOT, capture_output=True, check=True)
        require(hashlib.sha256(result.stdout).hexdigest() == digest, 'Review commit differs')
    for capture in review['captures']:
        for filename, field in (('MANIFEST.json', 'manifest_sha256'), ('COMPLETION.json', 'completion_sha256')):
            require(sha(ROOT/capture['path']/filename) == capture[field], 'Reviewed product capture changed')

    links = 0
    documents = list(BASE.glob('*.md')) + [BASE.parent/'DIALOGUE_COURANT.md']
    for doc in documents:
        for link in re.findall(r'\]\(([^)]+)\)', doc.read_text()):
            target = link.split('#', 1)[0]
            if target and '://' not in target:
                path = (doc.parent/target).resolve()
                require(path.exists() or path == BASE/'CHECKS.json', 'Missing document link: ' + target)
                links += 1
    inventory = {}
    for path in sorted(BASE.rglob('*')):
        relative = path.relative_to(BASE)
        if not path.is_file() or path.name == 'CHECKS.json':
            continue
        if any(part.startswith('.') or part == '__pycache__' for part in relative.parts) and relative != Path('.gitignore'):
            continue
        inventory[str(relative)] = sha(path)
    print(json.dumps(dict(schema='mhgp8_audit_kernel_composition_closure_v1', status='passed',
        source_commit=review['source_commit'], source_pins=len(source_pins()),
        qualification=qualification['counts'], qualification_commands=200,
        paired_lidar_measurements=126, productive_presentations=54, shell_id_occurrences=216,
        selected_target_occurrences=dict(accepted=20, rejected=16), readers_equal=True,
        command_checks=checks, composition_model=normal['composition_model'],
        degeneracy_model=normal['degeneracy_model'], window_model=models[0],
        aggregates_rejection_sample=normal['aggregates'], aggregates_productive_sample=positive['aggregates'],
        preserved_preflight_failure=True, reviewed_product_sources_match_commit=True,
        local_document_links=links, dialogue_sha256=sha(BASE.parent/'DIALOGUE_COURANT.md'), files=inventory,
        scope='Independent audit of ports28/29 and rational models; no port30, composition or full tower qualification',
        gcp_used=False), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
