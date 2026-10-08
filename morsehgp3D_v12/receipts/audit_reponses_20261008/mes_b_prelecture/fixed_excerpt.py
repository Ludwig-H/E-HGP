# Extraits litteraux du pilote MES-B en construction ; temoin d'audit, pas un pilote de campagne.
import json
import math
import re

STAGES = ('P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R')


C_KEYS = ('parcours', 'feuilles', 'emission', 'fin_etage', 'transferts', 'publication')


G_KEYS = ('tables', 'resolution')


OUT_KEYS = ('validation', 'empreinte')


FULL_KEYS = frozenset(('phase', 'pass', 'trame', 'voie', 'status', 'coord_bits', 'kmax', 'threads', 'sites',
                       'wall_ns', 'etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns', 'pic_octets', 'cpu_ns',
                       'rss_max_octets', 'appareil_octets', 'epinglee_octets', 'pic_appareil_octets', 'full_sha256'))


INT_KEYS = ('pass', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns', 'pic_octets', 'cpu_ns', 'rss_max_octets',
            'appareil_octets', 'epinglee_octets', 'pic_appareil_octets')


OPEN_KEYS = frozenset(('phase', 'status', 'reason', 'wall_ns', 'budget_appareil'))


VOIES = {'appareil': 'device', 'cpu': 'cpu'}


REFUSALS = ('invalid_input', 'unsupported_degeneracy', 'resource_exhausted')  # code 2 de la sonde : refus publies


BITS_EXPECTED = 21


def label_of(name, used):
    """Etiquette de trame de la sonde (au plus 23 octets), unique dans la campagne."""
    base = name[-23:]
    label, i = base, 1
    while label in used:
        prefix = '%d_' % i
        label = prefix + name[-(23 - len(prefix)):]
        i += 1
    used.add(label)
    return label


def is_int(value):
    return type(value) is int and 0 <= value < (1 << 64)


def check_full(row, i, case, sites, label):
    """'' si la ligne full de la passe i est conforme, sinon la raison."""
    if set(row) != FULL_KEYS:
        return 'cles de la passe %d : %s' % (i, sorted(set(row) ^ FULL_KEYS))
    if any(not is_int(row[k]) for k in INT_KEYS):
        return 'entier attendu (passe %d)' % i
    if row['pass'] != i or row['status'] != 'ok' or row['trame'] != label or \
            row['voie'] != VOIES[case['voie']] or row['kmax'] != case['k'] or row['threads'] != case['fils'] or \
            row['coord_bits'] != BITS_EXPECTED or row['sites'] != sites:
        return 'passe %d hors contrat (passe, statut, trame, voie, K, fils, profil ou sites)' % i
    blocks = ((row['etapes_ns'], STAGES), (row['c_ns'], C_KEYS), (row['g_ns'], G_KEYS), (row['hors_mur_ns'], OUT_KEYS))
    for block, keys in blocks:
        if type(block) is not dict or set(block) != set(keys) or any(not is_int(block[k]) for k in keys):
            return 'bloc de durees mal forme (passe %d)' % i
    st, g = row['etapes_ns'], row['g_ns']
    if sum(st[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR')) > row['wall_ns'] or \
            sum(st[k] for k in ('T', 'M', 'V', 'R')) > st['TMVR'] or g['tables'] + g['resolution'] > st['G']:
        return 'etages non inclus dans le mur (passe %d)' % i
    if type(row['full_sha256']) is not str or not re.fullmatch(r'[0-9a-f]{64}', row['full_sha256']):
        return 'empreinte FUL1 mal formee (passe %d)' % i
    if case['voie'] == 'cpu' and (row['appareil_octets'] or row['epinglee_octets'] or row['pic_appareil_octets']):
        return 'memoire de l\'appareil sur la voie CPU (passe %d)' % i
    return ''


def unique_object(pairs):
    row = {}
    for key, value in pairs:
        if key in row:
            raise ValueError('cle JSON repetee')
        row[key] = value
    return row


def reject_constant(value):
    raise ValueError('constante JSON non finie : ' + value)


def parse_output(code, text, case, sites, label):
    """Rend dict(etat, raison, passes) : etat 'ok' ; 'refus' (code 2 de la sonde, ligne de sortie conforme : un
    resultat) ; 'echec' (expiration, signal, invariant viole ou sortie inattendue : un resultat du cas) ; 'illisible'
    (sortie hors schema ou appareil indisponible : un controle manque)."""
    rows = []
    if code == 'expire':
        return dict(etat='echec', raison='expire', passes=[])
    if type(code) is int and code < 0:
        return dict(etat='echec', raison='signal %d' % -code, passes=[])
    for raw in text.splitlines():
        if not raw.isascii():
            return dict(etat='illisible', raison='ligne non ASCII', passes=[])
        try:
            row = json.loads(raw, object_pairs_hook=unique_object, parse_constant=reject_constant)
        except ValueError:
            return dict(etat='illisible', raison='ligne illisible', passes=[])
        if not isinstance(row, dict):
            return dict(etat='illisible', raison='ligne non objet', passes=[])
        rows.append(row)
    if not rows or set(rows[-1]) != {'phase', 'status', 'reason'} or rows[-1]['phase'] != 'exit':
        return dict(etat='illisible', raison='ligne de sortie absente (code %s)' % code, passes=[])
    end, body = rows[-1], rows[:-1]
    open_row = None
    if case['voie'] == 'appareil':
        if not body or body[0].get('phase') != 'open':
            return dict(etat='illisible', raison='ligne open absente', passes=[])
        open_row, body = body[0], body[1:]
        if set(open_row) != OPEN_KEYS or not is_int(open_row['wall_ns']) or open_row['budget_appareil'] != 'separe':
            return dict(etat='illisible', raison='ligne open hors schema', passes=[])
        if open_row['status'] != 'ok' or open_row['reason'] != 'none':
            return dict(etat='illisible', raison='appareil indisponible : %s' % open_row['reason'], passes=[])
    if len(body) % 2 != 0:
        return dict(etat='illisible', raison='passe sans liberation', passes=[])
    passes = []
    for i in range(len(body) // 2):
        full, free = body[2 * i], body[2 * i + 1]
        if full.get('phase') != 'full':
            return dict(etat='illisible', raison='ligne full attendue (passe %d)' % i, passes=[])
        reason = check_full(full, i, case, sites, label)
        if reason:
            return dict(etat='illisible', raison=reason, passes=[])
        if set(free) != {'phase', 'pass', 'liberation_ns'} or free['phase'] != 'liberation' or not is_int(free['pass']) or free['pass'] != i or \
                not is_int(free['liberation_ns']):
            return dict(etat='illisible', raison='liberation hors schema (passe %d)' % i, passes=[])
        passes.append(dict(full, liberation_ns=free['liberation_ns']))
    if type(end['status']) is not str or type(end['reason']) is not str:
        return dict(etat='illisible', raison='ligne de sortie hors schema', passes=[])
    ok_end = end['status'] == 'ok' and end['reason'] == 'none'
    if ok_end and code == 0 and len(passes) == case['passes']:
        state = dict(etat='ok', raison='', passes=passes)
    elif not ok_end and code == 2 and len(passes) < case['passes'] and end['status'] in REFUSALS:
        state = dict(etat='refus', raison='%s/%s' % (end['status'], end['reason']), passes=passes)
    else:
        return dict(etat='echec', raison='sortie %s/%s, code %s, %d passes' % (end['status'], end['reason'], code,
                                                                             len(passes)), passes=passes)
    if open_row is not None:
        state['open_ns'] = open_row['wall_ns']
    return state


def warm_pass(passes):
    """Passe chaude : la derniere jouee si au moins deux, sinon la premiere (froide)."""
    if not passes:
        return None, None
    return (passes[-1], 'chaude') if len(passes) >= 2 else (passes[0], 'froide')


def slope(points):
    """Pente des moindres carres de log(y) contre log(x) ; None sous deux points distincts."""
    xs = [math.log(x) for x, _ in points]
    ys = [math.log(y) for _, y in points]
    if len(set(xs)) < 2:
        return None
    mx, my = sum(xs) / len(xs), sum(ys) / len(ys)
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)


def verdicts(results, series):
    """Criteres B1 a B4 sur les resultats ; rend {critere: dict(etat, detail)}."""
    def played(k, voie):
        return [r for r in results if r['k'] == k and r['voie'] == voie and r['etat'] != 'non_joue']
    out = {}
    k5 = played(5, 'appareil')
    rows = [(r['nom'], warm_pass(r['passes'])[0]['wall_ns'] / 1e9 / (r['sites'] / 1e6)) for r in k5 if r['passes']]
    bad = [(n, v) for n, v in rows if v > 2.0]
    out['B1'] = dict(etat='non evalue' if not rows else 'non tenu' if bad else 'tenu',
                     detail=['%s : %.3f s par million' % (n, v) for n, v in bad] or
                     ['%d scenes, maximum %.3f s par million' % (len(rows), max(v for _, v in rows))] if rows else [])
    small = [r for r in k5 if r['sites'] < 10_000_000]
    failed = [r for r in small if r['etat'] != 'ok']
    out['B2'] = dict(etat='non evalue' if not small else 'non tenu' if failed else 'tenu',
                     detail=['%s : %s (%s)' % (r['nom'], r['etat'], r['raison']) for r in failed] or
                     ['%d scenes jouees sous 10 millions de sites' % len(small)] if small else [])
    fits, bad3 = [], []
    for members in series:
        found = {r['nom']: r for r in k5 if r['nom'] in members and r['passes'] and r['etat'] == 'ok'}
        if len(found) != len(members):
            continue
        s = slope([(found[m]['sites'], warm_pass(found[m]['passes'])[0]['wall_ns']) for m in members])
        if s is None:
            continue
        fits.append('%s : pente %.3f' % (' < '.join(members), s))
        if s > 1.1:
            bad3.append(fits[-1])
    out['B3'] = dict(etat='non evalue' if not fits else 'non tenu' if bad3 else 'tenu', detail=fits)
    k10 = [r for r in played(10, 'appareil') + played(10, 'cpu') if r['passes']]
    rows10 = [(r['nom'], r['voie'], warm_pass(r['passes'])[0]['wall_ns'] / 1e9 / (r['sites'] / 1e6)) for r in k10]
    bad4 = [(n, v, x) for n, v, x in rows10 if x > 10.0]
    out['B4'] = dict(etat='non evalue' if not rows10 else 'non tenu' if bad4 else 'tenu',
                     detail=['%s (%s) : %.3f s par million' % row for row in (bad4 or rows10)])
    for key, cases in (('B1', k5), ('B4', played(10, 'appareil') + played(10, 'cpu'))):
        failures = [r for r in cases if r['etat'] != 'ok']
        if failures:
            out[key]['etat'] = 'non tenu'
            out[key]['detail'] += ['%s : %s (%s)' % (r['nom'], r['etat'], r['raison']) for r in failures]
    return out
