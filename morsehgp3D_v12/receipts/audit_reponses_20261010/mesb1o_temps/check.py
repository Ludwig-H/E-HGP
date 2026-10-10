#!/usr/bin/env python3
"""Comparaison MES-B1t/B1o : JSONL, rapports et plans seulement ; pas d'admission de provenance."""
import argparse
import collections
import hashlib
import json
import subprocess
import types
from pathlib import Path

PREFIX = 'morsehgp3D_v12/'
SOURCES = ('caf9585e4a89ba101ba86d2d46e77cb340be15bf', 'ae8f8107cc5a13356da89addf90808b5aaad9d67')
LF_SHA = '15437e5f09408d5fbdc6288e33a2452490489f13bf9a3b3a24c99f5d109b6554'
PILOT_SHA = '9eb044087bfd95443eb5149bd25a12170158769d4bba07ad47681d9f07a25f32'
META_SHA = 'f57f3d0b67924ed8808eb4d69f0b2e521d3b4480b336b63306e8d8e865cec333'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(raw):
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def options(argv):
    need(argv[:2] == ['python3', '{src}/morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py'], 'pilote attendu')
    need(len(argv[2:]) % 2 == 0, 'arguments pairs')
    result = dict(zip(argv[2::2], argv[3::2]))
    need(len(result) * 2 == len(argv[2:]), 'option dupliquée')
    return result


def compact(row):
    if row is None:
        return None
    out = {k: row[k] for k in ('pass', 'wall_ns', 'cpu_ns', 'etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns', 'pic_octets', 'rss_max_octets', 'appareil_octets', 'epinglee_octets', 'pic_appareil_octets', 'memoire_octets')}
    out['liberation_ns'] = row.get('liberation_ns')
    out['unpartitioned_ns'] = row['wall_ns'] - sum(row['etapes_ns'].values())
    out['full_sha256'] = row.get('full_sha256')
    out['last_orders'] = {stage: [i + 1 for i, ends in enumerate(row['fins_par_ordre_ns']) if ends[j] == max(e[j] for e in row['fins_par_ordre_ns'])] for j, stage in enumerate(('G', 'T', 'M', 'V', 'R'))}
    return out


def diagnostic_prefix(raw, lf, expected):
    """Ne convertit pas un préfixe interrompu en prise réussie ; récupère les lignes complètes."""
    rows = []
    for line in raw.splitlines():
        try:
            parsed, why = lf.read_rows(line.decode('ascii'))
        except UnicodeError:
            break
        if why or not parsed:
            break
        rows.append(parsed[0])
    j = 0
    if expected['voie'] == 'appareil':
        if not rows or rows[0].get('phase') != 'open' or rows[0].get('status') != 'ok':
            return []
        j = 1
    out = []
    while j < len(rows) and rows[j].get('phase') == 'full':
        row = rows[j]
        if len(out) >= expected['passes'] or lf.check_full(row, len(out), expected):
            break
        row = dict(row)
        if j + 1 < len(rows) and set(rows[j + 1]) == {'phase', 'pass', 'liberation_ns'} and rows[j + 1]['phase'] == 'liberation' and lf.is_int(rows[j + 1]['pass']) and rows[j + 1]['pass'] == row['pass'] and lf.is_int(rows[j + 1]['liberation_ns']):
            row['liberation_ns'] = rows[j + 1]['liberation_ns']
            j += 2
        else:
            out.append(compact(row))
            break
        out.append(compact(row))
    return out


def campaign(cmdroot, planpath, sites, lf):
    planraw = planpath.read_bytes()
    plan = json.loads(planraw)
    inventory = {'plan.json': pin(planraw)}
    results, all_digests, report_opts = [], {}, []
    for i, command in enumerate(plan['commands']):
        opt = options(command['argv'])
        report_opts.append(opt)
        folder = 'b_identite' if i == 0 else 'b'
        base = cmdroot / ('%03d_%s' % (i, command['name'])) / 'files' / folder
        report_raw = (base / 'rapport_b.json').read_bytes()
        inventory[folder + '/rapport_b.json'] = pin(report_raw)
        report = json.loads(report_raw)
        need(report['provenance']['pilote_sha256'] == PILOT_SHA and report['provenance']['manifeste_sha256'] == META_SHA, 'pilote/manifeste non épinglés')
        params = report['parametres']
        need(params['schema'] == 'recouvert' and params['fils'] == 48 and params['budget_octets'] == 160 << 30 and params['budget_appareil_octets'] == int(opt['--budget-appareil-gio']) << 30, 'paramètres')
        actual_opts = dict(zip(params['argv'][::2], params['argv'][1::2]))
        need(2 * len(actual_opts) == len(params['argv']) and set(actual_opts) == set(opt) and all(actual_opts[k] == v for k, v in opt.items() if k not in ('--src', '--travail', '--donnees', '--sortie')), 'argv rapport')
        specs = [x.split(':') for x in opt['--cas'].split(',')]
        need(len(specs) == len(report['cas']), 'nombre de cas')
        labels, launched, cases, digests = set(), set(), [], {}
        for spec, entry in zip(specs, report['cas']):
            name, k, voie, passes = spec
            label, rank = name[-23:], 1
            while label in labels:
                prefix = str(rank) + '_'
                label, rank = prefix + name[-(23 - len(prefix)):], rank + 1
            labels.add(label)
            expected = dict(voie=voie, k=int(k), fils=48, passes=int(passes), empreinte=sites[name] <= 1600000, trames=[(label, sites[name])], budget_appareil='separe', bits=21, schema='recouvert')
            need(all(entry[key] == value for key, value in [('nom', name), ('k', int(k)), ('voie', voie), ('fils', 48), ('etiquette', label), ('sites', sites[name]), ('empreinte', expected['empreinte'])]), 'identité cas')
            tag = '%s_k%s_%s' % (name, k, voie)
            output = dict(nom=name, k=int(k), voie=voie, sites=sites[name], expected=int(passes), state=entry['etat'], reason=entry['raison'], code=entry.get('code'), smi_peak_mib=entry.get('pic_nvidia_smi_mio'), process_seconds=entry.get('secondes'), full=[], diagnostic_only=[])
            if entry['etat'] == 'non_joue':
                need(not entry['passes'] and entry['raison'].startswith('delai : '), 'non joué incohérent')
            else:
                raw = (base / 'brut' / (tag + '.jsonl')).read_bytes()
                err = (base / 'brut' / (tag + '.err')).read_bytes()
                launched.add(tag)
                inventory[folder + '/' + tag + '.jsonl'] = pin(raw)
                inventory[folder + '/' + tag + '.err'] = pin(err)
                need(not err.strip(), 'stderr non vide : ' + tag)
                decoded = lf.parse_output(entry['code'], raw.decode('ascii'), expected)
                need(all(entry[key] == value for key, value in decoded.items()), 'rapport/JSONL divergent : ' + tag)
                need(('open_ns' in entry) == ('open_ns' in decoded), 'open incohérent')
                output['open_ns'] = decoded.get('open_ns')
                need(decoded['etat'] != 'illisible', 'flux illisible : ' + tag)
                for row in decoded['passes']:
                    need(16 * sites[name] <= row['pic_octets'] <= 160 << 30 and row['epinglee_octets'] <= row['pic_octets'], 'budget hôte')
                    need(row['appareil_octets'] <= row['pic_appareil_octets'] <= int(opt['--budget-appareil-gio']) << 30, 'budget appareil')
                    need(expected['empreinte'] or row['hors_mur_ns']['empreinte'] == 0, 'empreinte hors plan')
                    output['full'].append(compact(row))
                    if expected['empreinte']:
                        key = '%s:K%d' % (name, int(k))
                        digests.setdefault(key, set()).add(row['full_sha256'])
                        all_digests.setdefault(key, set()).add(row['full_sha256'])
                if decoded['etat'] == 'echec' and not decoded['passes']:
                    output['diagnostic_only'] = diagnostic_prefix(raw, lf, expected)
            output['complete_passes'] = len(output['full'])
            output['first'] = output['full'][0] if output['full'] else None
            output['warm'] = output['full'][1] if len(output['full']) > 1 else None
            output['prefix_before_failure'] = entry['etat'] != 'ok' and bool(output['full'])
            need(len(output['full']) <= 2, 'régime plus de deux passes')
            del output['full']
            cases.append(output)
        need({p.name for p in (base / 'brut').iterdir()} == {t + s for t in launched for s in ('.jsonl', '.err')}, 'bruts absents/supplémentaires')
        need(report['empreintes'] == {k: sorted(v) for k, v in digests.items()}, 'table empreintes')
        results.append(dict(folder=folder, device_budget_gib=int(opt['--budget-appareil-gio']), cases=cases, report_criteria={k: v['etat'] for k, v in report['criteres'].items()}, report_verdict=report['verdict']))
    need(all(len(v) == 1 for v in all_digests.values()), 'identités intra/intervoies divergentes')
    all_cases = [c for report in results for c in report['cases']]
    return dict(reports=results, digests={k: sorted(v) for k, v in all_digests.items()}, processes_planned=len(all_cases), processes_run=sum(c['state'] != 'non_joue' for c in all_cases), full_passes=sum(c['complete_passes'] for c in all_cases), warm_passes=sum(c['warm'] is not None for c in all_cases), outcomes={s: sum(c['state'] == s for c in all_cases) for s in ('ok', 'refus', 'echec', 'non_joue')}), inventory, report_opts


def delta(old, new):
    if old is None or new is None:
        return None
    need(old['pass'] == new['pass'], 'rang de passe')
    return dict(old_wall_ns=old['wall_ns'], new_wall_ns=new['wall_ns'], wall_delta_ns=new['wall_ns'] - old['wall_ns'], new_over_old=new['wall_ns'] / old['wall_ns'], cpu_delta_ns=new['cpu_ns'] - old['cpu_ns'], stage_delta_ns={k: new['etapes_ns'][k] - v for k, v in old['etapes_ns'].items()}, c_delta_ns={k: new['c_ns'][k] - v for k, v in old['c_ns'].items()}, host_peak_delta_bytes=new['pic_octets'] - old['pic_octets'], device_peak_delta_bytes=new['pic_appareil_octets'] - old['pic_appareil_octets'], partition_remainder_delta_ns=new['unpartitioned_ns'] - old['unpartitioned_ns'], full_identity_equal=(new['full_sha256'] == old['full_sha256']) if old['full_sha256'] is not None else None)


def run(args):
    lfraw = subprocess.check_output(['git', '-C', str(args.repo), 'show', SOURCES[0] + ':' + PREFIX + 'microbancs/outils/lecteur_full.py'])
    need(pin(lfraw)['sha256'] == LF_SHA, 'lecteur épinglé')
    newer = subprocess.check_output(['git', '-C', str(args.repo), 'show', SOURCES[1] + ':' + PREFIX + 'microbancs/outils/lecteur_full.py'])
    need(newer == lfraw, 'lecteur différent')
    lf = types.ModuleType('mesb_lf'); exec(compile(lfraw, '[lecteur FULL épinglé]', 'exec'), lf.__dict__)
    meta = json.loads((args.repo / PREFIX / 'receipts/audit_reponses_20261010/mesb1o_prelecture/results.json').read_bytes())
    old, old_pins, old_options = campaign(args.old_cmd, args.old_plan, meta['scene_sites'], lf)
    new, new_pins, new_options = campaign(args.new_cmd, args.new_plan, meta['scene_sites'], lf)
    need(old_options == new_options, 'options anciennes/nouvelles différentes')
    comparisons = []
    for op, np in zip(old['reports'], new['reports']):
        for oc, nc in zip(op['cases'], np['cases']):
            keys = ('nom', 'sites', 'k', 'voie', 'expected')
            need(all(oc[k] == nc[k] for k in keys), 'cas non comparable')
            comparisons.append(dict(folder=op['folder'], **{k: nc[k] for k in keys}, old_state=oc['state'], new_state=nc['state'], old_complete_passes=oc['complete_passes'], new_complete_passes=nc['complete_passes'], first=delta(oc['first'], nc['first']), warm=delta(oc['warm'], nc['warm'])))
    shared = set(old['digests']) & set(new['digests'])
    identities = {k: old['digests'][k] == new['digests'][k] for k in sorted(shared)}
    need(all(identities.values()), 'divergence différentielle FUL1')
    paired = [c[r] for c in comparisons for r in ('first', 'warm') if c[r] is not None]
    for d in paired:
        need(d['wall_delta_ns'] == sum(d['stage_delta_ns'].values()) + d['partition_remainder_delta_ns'], 'décomposition de la différence de mur')
    new_cases = [c for report in new['reports'] for c in report['cases']]
    records = {}
    for label, mode, warm in [('gpu_k5_complete', 'appareil', False), ('gpu_k5_warm', 'appareil', True), ('cpu_k5_complete', 'cpu', False)]:
        eligible = [c for c in new_cases if c['state'] == 'ok' and c['voie'] == mode and c['k'] == 5 and (not warm or c['warm'] is not None)]
        if eligible:
            c = max(eligible, key=lambda c: c['sites']); row = c['warm'] if warm else c['first']
            records[label] = {'scene': c['nom'], 'sites': c['sites'], 'pass': row['pass'], 'wall_ns': row['wall_ns']}
    ends = {}
    for name, campaign_result in [('old', old), ('new', new)]:
        rows = [r for report in campaign_result['reports'] for c in report['cases'] for r in (c['first'], c['warm']) if r is not None]
        ends[name] = {stage: dict(collections.Counter(','.join(map(str, r['last_orders'][stage])) for r in rows)) for stage in ('G', 'R')}
    summary = dict(compared_passes=len(paired), full_wall_lower=sum(d['wall_delta_ns'] < 0 for d in paired), host_peak_higher=sum(d['host_peak_delta_bytes'] > 0 for d in paired), device_peak_equal=sum(d['device_peak_delta_bytes'] == 0 for d in paired), c_wall_higher=sum(d['stage_delta_ns']['C'] > 0 for d in paired), last_orders=ends, largest_within_new_cohort=records)
    return dict(schema='audit.mesb1o_timing.v1', provenance_admission=False, old=old, new=new, comparisons=comparisons, summary=summary, shared_identity_keys=identities, old_only_identity_keys=sorted(set(old['digests']) - shared), new_only_identity_keys=sorted(set(new['digests']) - shared), pins={'old': old_pins, 'new': new_pins, 'lecteur_full': pin(lfraw), 'prelecture_result': pin((args.repo / PREFIX / 'receipts/audit_reponses_20261010/mesb1o_prelecture/results.json').read_bytes())})


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('repo', 'old-cmd', 'new-cmd', 'old-plan', 'new-plan'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--write', action='store_true')
    args = p.parse_args()
    result = run(args)
    capture = dict(schema='audit.mesb1o_timing.capture.v1', sources=list(SOURCES), inputs=result.pop('pins'))
    here = Path(__file__).resolve().parent
    for name, obj in [('capture.json', capture), ('results.json', result)]:
        if args.write:
            (here / name).write_text(json.dumps(obj, ensure_ascii=False, separators=(',', ':')) + '\n')
        else:
            need(obj == json.loads((here / name).read_bytes()), name + ' divergent')
    print(json.dumps({'old': {k: result['old'][k] for k in ('processes_run', 'full_passes', 'warm_passes', 'outcomes')}, 'new': {k: result['new'][k] for k in ('processes_run', 'full_passes', 'warm_passes', 'outcomes')}, 'compared_cases': len(result['comparisons']), 'identity_keys': len(result['shared_identity_keys'])}, sort_keys=True))


if __name__ == '__main__':
    main()
