"""Contre-tests reproductibles de check.py, sans construction, CTest ou GCP.

Deux captures LIVE servent de temoins : reprise2 reste en echec, reprise3 est conforme. Quatorze corruptions
sont refusees ; les changements restent en memoire, y compris les quatre essais du vrai lecteur complet.
Aucun recu ni journal n'est copie ou modifie. Usage : python3 [-O] check_selftest.py.
"""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
import tarfile
from unittest import mock
import xml.etree.ElementTree as ET


def archive_data(path):
    with tarfile.open(path) as archive:
        return {item.name: archive.extractfile(item).read() for item in archive if item.isfile()}


def main():
    root = Path(__file__).resolve().parent
    spec = importlib.util.spec_from_file_location('receipt_reader', root / 'check.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    for name, failed in [('reprise2', True), ('reprise3', False)]:
        with contextlib.redirect_stdout(io.StringIO()):
            got = reader.check(root / name)
        if got is not failed:
            raise RuntimeError('temoin invalide : ' + name)
    data = archive_data(root / 'reprise3/results.tar.gz')
    prefix = reader.BASE + 'style/'
    baseline = reader.js(data[prefix + 'result.json'])
    refused = []

    def refuse(label, action):
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                action()
        except reader.Refusal:
            refused.append(label)
        else:
            raise RuntimeError('corruption acceptee : ' + label)

    def mutate_case(label, mutate):
        changed, config = dict(data), copy.deepcopy(baseline)
        mutate(changed, config)
        changed[prefix + 'result.json'] = json.dumps(config).encode()
        refuse(label, lambda: reader.judge_config(config, changed))

    def selection(changed, kind):
        selected = reader.js(changed[prefix + 'tests.json'])
        if kind == 'dup':
            selected.append(selected[0])
        elif kind == 'missing':
            selected.pop()
        else:
            selected = []
        changed[prefix + 'tests.json'] = json.dumps(selected).encode()

    for kind in ['dup', 'missing', 'empty']:
        mutate_case('selection_' + kind, lambda d, c, k=kind: selection(d, k))

    def xml_change(changed, kind):
        tree = ET.fromstring(changed[prefix + 'junit.xml'])
        child = copy.deepcopy(next(tree.iter('testcase')))
        if kind == 'extra':
            child.set('name', 'mhgp11_unexpected')
        tree.append(child)
        changed[prefix + 'junit.xml'] = ET.tostring(tree)

    for kind in ['dup', 'extra']:
        mutate_case('JUnit_' + kind, lambda d, c, k=kind: xml_change(d, k))
    mutate_case('compteur', lambda d, c: c['tests'].__setitem__('passed', 1))
    mutate_case('compteur_booleen', lambda d, c: c['tests'].__setitem__('not_run', False))
    mutate_case('etapes_vides', lambda d, c: c.__setitem__('steps', []))
    mutate_case('labels', lambda d, c: c.__setitem__('passed_labels', {}))
    historical = archive_data(root / 'reprise2/results.tar.gz')
    config = reader.js(historical[reader.BASE + 'mutants/result.json'])
    config.update(status='ok', conforming=True)
    historical[reader.BASE + 'mutants/result.json'] = json.dumps(config).encode()
    refuse('echec_verdi', lambda: reader.judge_config(config, historical))

    virtual = root / '__selftest_in_memory'
    originals = {virtual / name: (root / 'reprise3' / name).read_bytes()
                 for name in ['receipt.json', 'matrix.json', 'results.tar.gz']}
    read_bytes = Path.read_bytes
    for case in ['archive_hash', 'matrix_copy', 'raw_hash', 'closure']:
        changed = dict(originals)
        if case == 'archive_hash':
            changed[virtual / 'results.tar.gz'] += b'altered'
        elif case == 'matrix_copy':
            changed[virtual / 'matrix.json'] += b' '
        else:
            receipt = reader.js(changed[virtual / 'receipt.json'])
            key = 'original_receipt_sha256' if case == 'raw_hash' else 'closure'
            receipt[key] = '0' * 64 if case == 'raw_hash' else 'running'
            changed[virtual / 'receipt.json'] = json.dumps(receipt).encode()

        def read_in_memory(path):
            return changed[path] if path in changed else read_bytes(path)

        with mock.patch.object(Path, 'read_bytes', read_in_memory):
            refuse(case, lambda: reader.check(virtual))
    if len(refused) != 14 or len(set(refused)) != 14:
        raise RuntimeError('plancher ou identifiants des corruptions')
    print('check_selftest_ok positifs=2 corruptions_refusees=14')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
