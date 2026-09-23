#!/usr/bin/env python3
"""Regenere, ou controle, les entrees des juges depuis les trames sans sol versionnees du recu v8.

Les coupes emboitees 8k (s00, s01, s02) sont produites par `scene_cases` du lanceur LiDAR v9
(`morsehgp3D_v9/bench/run_lidar_scaling.py`), qui controle d'abord chaque trame contre son MANIFEST v8 ;
la trame entiere 08/000000 est copiee telle quelle. Seuls les quatre fichiers epingles ci-dessous sont
publies, et seulement apres verification de leurs empreintes (ecriture dans un dossier temporaire voisin,
puis renommage). Les octets derives de SemanticKITTI ne sont jamais ecrits dans le depot.

Usage :
  python3 regen_inputs.py <dossier_neuf_hors_depot>   regenere ; code 0 si toutes les empreintes concordent
  python3 regen_inputs.py --check <dossier>           controle un dossier existant ; code 0 si conforme
Code 1 : empreinte differente, refus du MANIFEST v8 ou erreur d'entree/sortie ; code 2 : usage.
"""
import hashlib
import importlib.util
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
EXPECTED = {  # empreintes des entrees des campagnes v5 a v7 (PROVENANCE.txt)
    's00_k5_s8_w8_r0_nested_8000.u32le': 'cf84866390ede2e53a20b4166e8dc1a14cb5791eb78b6efba147941b7cd61c54',
    's01_k5_s8_w8_r0_nested_8000.u32le': 'd141e843dcdd3c56d3b396431c9699d94b45b586bc318de70f7aeb77049a6d2a',
    's02_k5_s8_w8_r0_nested_8000.u32le': '255f6433034a6d9e1531dce11cb880725c3bf3e46b6b66078596341dbd7f8d5e',
    'scene_00_full.u32le': '0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf',
}


def check(folder):
    bad = 0
    for name, want in EXPECTED.items():
        path = folder / name
        got = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else 'absent'
        print(got, name, 'ok' if got == want else 'DIFFERENT')
        bad += got != want
    return bad == 0


def regenerate(out):
    out = out.resolve()
    if out.exists():
        raise OSError('output folder already exists: %s' % out)
    if out.is_relative_to(ROOT):
        raise OSError('output folder inside the repository: %s' % out)
    spec = importlib.util.spec_from_file_location('rls', ROOT / 'morsehgp3D_v9/bench/run_lidar_scaling.py')
    rls = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(rls)
    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=out.parent) as work_dir, tempfile.TemporaryDirectory(dir=out.parent) as stage_dir:
        work, stage = Path(work_dir), Path(stage_dir) / 'inputs'
        stage.mkdir()
        try:
            for scene in ('00', '01', '02'):
                name = 's' + scene + '_k5_s8_w8_r0'
                rls.scene_cases(scene, name, work)  # controle MANIFEST v8, ecrit les emboites dans `work`
                shutil.copyfile(work / (name + '_nested_8000.u32le'), stage / (name + '_nested_8000.u32le'))
            raw, _ = rls.read_u32(rls.V8 / 'scene_00_grid' / 'full.u32le', 3)
            (stage / 'scene_00_full.u32le').write_bytes(raw)
        except rls.Refusal as error:
            raise OSError('v8 manifest refusal: %s' % error) from error
        if not check(stage):
            return False
        stage.rename(out)
    return True


def main(argv):
    try:
        if len(argv) == 3 and argv[1] == '--check':
            return 0 if check(Path(argv[2])) else 1
        if len(argv) == 2 and not argv[1].startswith('--'):
            return 0 if regenerate(Path(argv[1])) else 1
    except OSError as error:
        print('regen_inputs: %s' % error, file=sys.stderr)
        return 1
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
