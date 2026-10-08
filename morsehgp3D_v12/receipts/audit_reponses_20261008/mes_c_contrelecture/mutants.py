"""Six mutants Python de notre lecteur, tués par les contre-JSON ; pas de sonde."""
import argparse
import ast
import contextlib
import io
import json
from pathlib import Path
import sys
import types

import reader
import test_reader


MUTANTS = {
    'C3_sans_completude': ("incomplete = not k5 or any(states[s['key']] in {'non_joue', 'illisible'} for s in k5)",
                           'incomplete = False'),
    'froid_conserve': ('warm = mine[1:]', 'warm = mine'),
    'groupes_melanges': ("groups.setdefault(case['groupe'], []).append", "groups.setdefault('reel', []).append"),
    'K_melanges': ("all_hashes.setdefault((spec['k'], case['nom']), set())", "all_hashes.setdefault((0, case['nom']), set())"),
    'empreinte_ignoree': ('if len(hashes) != 1:', 'if False:'),
    'code_sans_type': ("need(type(code) is int or code == 'expire' and type(code) is str, 'type du code')",
                       "need(True, 'type du code')"),
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', required=True)
    args = parser.parse_args()
    source = Path(reader.__file__).read_text()
    original_argv = sys.argv
    results = {}
    for name, (before, after) in MUTANTS.items():
        reader.need(source.count(before) == 1, 'emplacement ' + name)
        mutated = source.replace(before, after)
        ast.parse(mutated)
        module = types.ModuleType('mutant_reader')
        exec(compile(mutated, 'mutant_reader', 'exec'), module.__dict__)
        test_reader.audit = module
        sys.argv = ['test_reader.py', '--repo', args.repo]
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                test_reader.main()
        except module.Invalid as exc:
            results[name] = dict(verdict='tue', diagnostic=str(exc), syntax_valid=True)
        else:
            raise reader.Invalid('mutant vivant ' + name)
        finally:
            sys.argv = original_argv
            test_reader.audit = reader
    print(json.dumps(results, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
