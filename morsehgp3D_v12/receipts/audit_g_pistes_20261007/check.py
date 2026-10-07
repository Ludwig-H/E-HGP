#!/usr/bin/env python3
"""Modeles abstraits de signes : aucun predicat geometrique ou binaire produit."""
import json


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def census(signs, threshold, bounded):
    inner, shell, total_shell, visited = [], [], 0, 0
    for site, sign in enumerate(signs):
        visited += 1
        if sign < 0:
            inner.append(site)
            if len(inner) == threshold:
                return dict(status='saturated', inner=inner, shell=[], visited=visited)
        elif sign == 0:
            total_shell += 1
            if not bounded or len(shell) < 64:
                shell.append(site)
    if total_shell > 64:
        return dict(status='shell_capacity', visited=visited)
    return dict(status='complete', inner=inner, shell=shell, visited=visited)


def certificate(signs, support, skip):
    on, cursor, physical_calls = [], 0, 0
    for site, sign in enumerate(signs):
        if skip and cursor < len(support) and site == support[cursor]:
            on.append(site)
            cursor += 1
            continue
        physical_calls += 1
        if sign > 0:
            return dict(accepted=False, on=on), physical_calls
        if sign == 0:
            on.append(site)
    return dict(accepted=True, on=on), physical_calls


def main():
    streams = {'late_saturation': [0] * 65 + [-1, -1],
               'complete_overflow': [0] * 65 + [-1, 1],
               'complete_at_limit': [0] * 64 + [-1, 1],
               'early_saturation': [-1, -1] + [0] * 65}
    observed = {}
    for name, signs in streams.items():
        a, b = census(signs, 2, False), census(signs, 2, True)
        need(a == b, 'bounded census differs: ' + name)
        observed[name] = dict(status=b['status'], visited=b['visited'],
                              inner_size=len(b.get('inner', [])), shell_size=len(b.get('shell', [])))
    need(observed['late_saturation']['status'] == 'saturated' and
         observed['late_saturation']['visited'] == 67, 'premature rejection forbidden')
    cert_cases = [([0, -1, 0, -1], [0, 2]),
                  ([0, -1, 0, 0, 0], [0, 2, 4]),
                  ([0, 0, -1, 0, 0], [0, 1, 3, 4]),
                  ([1, 0, 0, 0, 0], [1, 2, 3, 4]),
                  ([0, 1, 0, 0, 0], [0, 2, 3, 4])]
    cert_results = []
    for signs, support in cert_cases:
        need(support == sorted(set(support)) and all(signs[i] == 0 for i in support), 'certified support')
        a, before = certificate(signs, support, False)
        b, after = certificate(signs, support, True)
        need(a == b, 'shell order or rejection changes')
        cert_results.append(dict(arity=len(support), accepted=a['accepted'], calls_before=before,
                                 calls_after=after, saved=before-after))
    print(json.dumps(dict(scope='abstract_sign_streams_no_geometry_no_native', census=observed,
                          early_overflow_mutant=dict(status='shell_capacity', visited=65,
                                                     disagrees_with='late_saturation'),
                          support_certificate=cert_results), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
