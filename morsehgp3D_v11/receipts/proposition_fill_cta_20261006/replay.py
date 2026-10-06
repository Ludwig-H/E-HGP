"""Verification du patch textuel et de la partition CTA/positions. Aucun CUDA execute."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path


def check(value, message):
    if not value:
        raise ValueError(message)


def section(text, first, last):
    return text[text.index(first):text.index(last, text.index(first))]


def proof(folder):
    meta = json.loads((folder / 'source_manifest.json').read_text())
    before = (folder / 'leaf_batch_cuda.before.cu').read_text()
    check(hashlib.sha256(before.encode()).hexdigest() == meta['source_sha256'], 'source pin')
    after = before
    for replacement in json.loads((folder / 'replacements.json').read_text()):
        check(after.count(replacement['old']) == 1, 'substitution unique')
        after = after.replace(replacement['old'], replacement['new'])
    check(hashlib.sha256(after.encode()).hexdigest() == meta['proposed_sha256'], 'empreinte candidat')
    patch = ''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                      fromfile='a/'+meta['source_path'], tofile='b/'+meta['source_path']))
    check(patch == (folder / 'fill_one_leaf_cta.patch').read_text(), 'patch exact')
    for first, last in [('__global__ void count_kernel', '__global__ void copy_kernel'),
                        ('__global__ void copy_kernel', '__global__ void fill_kernel')]:
        check(section(before, first, last) == section(after, first, last), 'kernel inchangé '+first)
    check('constexpr int kThreads = 32;' in before and 'constexpr int kThreads = 32;' in after,
          'threads32 attendus')
    matrix = []
    for count in (0, 1, 2, 14, 31, 32, 33, 4196, 65535, 65536):
        for limit in (1, 8, 31, 127, 65535, (1<<31)-1):
            grid = min(count, limit)
            check(count == 0 or 1 <= grid <= limit <= (1<<31)-1, 'grille positive admise')
            emitted = [p for block in range(grid) for p in range(block, count, grid)] if grid else []
            check(sorted(emitted) == list(range(count)), 'chaque position exactement une fois')
            # Liste selectionnee permutee : le noyau consomme la position et garde son identifiant j.
            selected = list(range(count))[::-1]
            check(sorted(selected[p] for p in emitted) == list(range(count)), 'identifiants non omis')
            matrix.append(dict(jobs=count, max_grid_x=limit, grid=grid, visited=len(emitted)))
    large = []
    for count in ((1<<31)-1, 1<<31, (1<<32)-1, 1<<32):
        for limit in (1, 65535, (1<<31)-1):
            grid = min(count, limit)
            q, r = divmod(count, grid)
            check(grid*q+r == count and 0 <= r < grid, 'partition par residues exhaustive')
            last = count-1
            check(last % grid < grid and last // grid < q + (1 if r else 0), 'derniere position atteinte')
            check(last + grid < 1<<64, 'increment position en u64')
            check(grid <= (1<<31)-1 < 1<<32, 'cast unsigned exact')
            check(count-1 <= (1<<32)-1, 'identifiant de feuille u32 existant')
            large.append(dict(jobs=count, max_grid_x=limit, grid=grid,
                              quotient=q, remainder=r, last_position=last))
    mutant = []
    for count in (14, 4196):
        # Erreur causale : garder blockIdx*32 en activant seulement lane0 sauterait 31 positions sur32.
        visits = [b*32 for b in range(count) if b*32 < count]
        check(len(visits) < count, 'mutation stride32 non detectee')
        mutant.append(dict(jobs=count, wrong_index_visited=len(visits), correct_grid=count))
    return dict(source_commit=meta['source_commit'], kind='modele indexation seulement, native0 cuda0 cloud0',
                enumerated_cases=len(matrix), enumerated=matrix, large_cases=large, index_mutant=mutant,
                invalid_device_limit='max_grid_x<1 ou query erreur -> refus explicite sans launch fill',
                disjoint_writes='chaque j selectionne unique ; sinks/prefixes du source gardes, pas prouves par ce modele',
                limits='ni compilation, ni exactitude geometrique CUDA, ni allocation device, ni vitesse qualifiees')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--check', type=Path)
    a = p.parse_args()
    value = proof(Path(__file__).resolve().parent)
    if a.check:
        check(json.loads(a.check.read_text()) == value, 'JSON fige different')
        print('fill CTA mapping conforme :60 cas enumeres,12 bornes larges, kernels count/copy identiques ; native0 cuda0')
    else:
        print(json.dumps(value,ensure_ascii=False,sort_keys=True,indent=2))
