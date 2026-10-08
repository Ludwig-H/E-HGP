#!/usr/bin/env python3
"""Lecteur strict des sorties de la sonde FULL residente (bench/full_probe.cpp), partage par les pilotes MES-B
(microbancs/mes_b_scenes) et MES-FULL (microbancs/mes_full).

Une sortie est lue entiere, ligne a ligne : objets JSON ASCII, sans cle repetee ni constante non finie ; voie
appareil : une ligne "open" reussie d'abord, avec le budget de l'appareil attendu ("separe" ou "partage") ; puis, pour
chaque passe jouee, une ligne "full" et une ligne "liberation" de meme rang ; la ligne de sortie en dernier. Une ligne
"full" a exactement les cles du schema (plus full_sha256 si l'empreinte est demandee), des entiers u64 non booleens, un
mur et des sites non nuls, la trame et les sites attendus pour sa passe (la passe p joue la trame p modulo le nombre de
trames), la voie, K, les fils et le profil demandes, des etages inclus dans le mur, une memoire par etage dont le plus
haut pic vaut pic_octets, et aucune memoire de l'appareil sur la voie CPU.

Issues : 'ok' ; 'refus' (code 2 de la sonde et ligne de sortie conforme d'un refus publie : un resultat) ; 'echec'
(expiration, signal, invariant viole ou sortie inattendue : un resultat du cas) ; 'illisible' (sortie hors schema ou
appareil indisponible : un controle manque).

attendu : dict(voie='appareil'|'cpu', k, fils, passes, empreinte (bool), trames=[(etiquette, sites), ...],
budget_appareil='separe'|'partage' (voie appareil), bits=21 par defaut).
Python 3.10 nu, aucun assert. Bibliotheque seulement (aucun point d'entree).
"""
import json
import re

STAGES = ('P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R')
C_KEYS = ('parcours', 'feuilles', 'emission', 'fin_etage', 'transferts', 'publication')
G_KEYS = ('tables', 'resolution')
OUT_KEYS = ('validation', 'empreinte')
MEM_STAGES = ('P', 'C', 'G', 'raccord', 'TMVR')  # memoire_octets : [usage a la fin de l'etage, pic de l'etage]
FULL_KEYS = frozenset(('phase', 'pass', 'trame', 'voie', 'status', 'coord_bits', 'kmax', 'threads', 'sites',
                       'wall_ns', 'etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns', 'pic_octets', 'cpu_ns',
                       'rss_max_octets', 'appareil_octets', 'epinglee_octets', 'pic_appareil_octets',
                       'memoire_octets'))
INT_KEYS = ('pass', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns', 'pic_octets', 'cpu_ns', 'rss_max_octets',
            'appareil_octets', 'epinglee_octets', 'pic_appareil_octets')
OPEN_KEYS = frozenset(('phase', 'status', 'reason', 'wall_ns', 'budget_appareil'))
VOIES = {'appareil': 'device', 'cpu': 'cpu'}
REFUSALS = ('invalid_input', 'unsupported_degeneracy', 'resource_exhausted')  # code 2 de la sonde : refus publies


def is_int(value):
    return type(value) is int and 0 <= value < (1 << 64)


def unique_object(pairs):
    """Objet JSON sans cle repetee (contrelecture de l'auditeur Codex, mes_b_prelecture)."""
    row = {}
    for key, value in pairs:
        if key in row:
            raise ValueError('cle JSON repetee')
        row[key] = value
    return row


def reject_constant(value):
    raise ValueError('constante JSON non finie : ' + value)


def check_full(row, i, attendu):
    """'' si la ligne full de la passe i est conforme, sinon la raison."""
    label, sites = attendu['trames'][i % len(attendu['trames'])]
    keys = FULL_KEYS | {'full_sha256'} if attendu['empreinte'] else FULL_KEYS
    if set(row) != keys:
        return 'cles de la passe %d : %s' % (i, sorted(set(row) ^ keys))
    if any(not is_int(row[k]) for k in INT_KEYS):
        return 'entier attendu (passe %d)' % i
    if row['wall_ns'] == 0 or row['sites'] == 0:
        return 'mur ou nombre de sites nul (passe %d)' % i  # mur nul : refuse avant toute statistique (auditeur)
    if row['pass'] != i or row['status'] != 'ok' or row['trame'] != label or row['voie'] != VOIES[attendu['voie']] or \
            row['kmax'] != attendu['k'] or row['threads'] != attendu['fils'] or \
            row['coord_bits'] != attendu.get('bits', 21) or row['sites'] != sites:
        return 'passe %d hors contrat (passe, statut, trame, voie, K, fils, profil ou sites)' % i
    blocks = ((row['etapes_ns'], STAGES), (row['c_ns'], C_KEYS), (row['g_ns'], G_KEYS), (row['hors_mur_ns'], OUT_KEYS))
    for block, names in blocks:
        if type(block) is not dict or set(block) != set(names) or any(not is_int(block[k]) for k in names):
            return 'bloc de durees mal forme (passe %d)' % i
    mem = row['memoire_octets']
    if type(mem) is not dict or set(mem) != set(MEM_STAGES) or \
            any(type(mem[k]) is not list or len(mem[k]) != 2 or not all(is_int(v) for v in mem[k]) or
                mem[k][0] > mem[k][1] for k in MEM_STAGES) or max(mem[k][1] for k in MEM_STAGES) != row['pic_octets']:
        return 'memoire par etage mal formee ou incoherente avec pic_octets (passe %d)' % i
    st, g = row['etapes_ns'], row['g_ns']
    if sum(st[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR')) > row['wall_ns'] or \
            sum(st[k] for k in ('T', 'M', 'V', 'R')) > st['TMVR'] or g['tables'] + g['resolution'] > st['G']:
        return 'etages non inclus dans le mur (passe %d)' % i
    if attendu['empreinte'] and (type(row['full_sha256']) is not str or
                                 not re.fullmatch(r'[0-9a-f]{64}', row['full_sha256'])):
        return 'empreinte FUL1 mal formee (passe %d)' % i
    if attendu['voie'] == 'cpu' and (row['appareil_octets'] or row['epinglee_octets'] or row['pic_appareil_octets']):
        return 'memoire de l\'appareil sur la voie CPU (passe %d)' % i
    return ''


def read_rows(text):
    """Objets JSON de la sortie, ou (None, raison)."""
    rows = []
    for raw in text.splitlines():
        if not raw.isascii():
            return None, 'ligne non ASCII'
        try:
            row = json.loads(raw, object_pairs_hook=unique_object, parse_constant=reject_constant)
        except ValueError:
            return None, 'ligne illisible'
        if not isinstance(row, dict):
            return None, 'ligne non objet'
        rows.append(row)
    return rows, ''


def parse_output(code, text, attendu):
    """Rend dict(etat, raison, passes[, open_ns]) ; voir l'en-tete pour les issues."""
    if code == 'expire':
        return dict(etat='echec', raison='expire', passes=[])
    if type(code) is int and code < 0:
        return dict(etat='echec', raison='signal %d' % -code, passes=[])
    rows, why = read_rows(text)
    if rows is None:
        return dict(etat='illisible', raison=why, passes=[])
    if not rows or set(rows[-1]) != {'phase', 'status', 'reason'} or rows[-1]['phase'] != 'exit':
        return dict(etat='illisible', raison='ligne de sortie absente (code %s)' % code, passes=[])
    end, body = rows[-1], rows[:-1]
    open_row = None
    if attendu['voie'] == 'appareil':
        if not body or body[0].get('phase') != 'open':
            return dict(etat='illisible', raison='ligne open absente', passes=[])
        open_row, body = body[0], body[1:]
        if set(open_row) != OPEN_KEYS or not is_int(open_row['wall_ns']) or \
                open_row['budget_appareil'] != attendu['budget_appareil']:
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
        reason = check_full(full, i, attendu)
        if reason:
            return dict(etat='illisible', raison=reason, passes=[])
        if set(free) != {'phase', 'pass', 'liberation_ns'} or free['phase'] != 'liberation' or \
                not is_int(free['pass']) or free['pass'] != i or not is_int(free['liberation_ns']):
            return dict(etat='illisible', raison='liberation hors schema (passe %d)' % i, passes=[])
        passes.append(dict(full, liberation_ns=free['liberation_ns']))
    if type(end['status']) is not str or type(end['reason']) is not str:
        return dict(etat='illisible', raison='ligne de sortie hors schema', passes=[])
    ok_end = end['status'] == 'ok' and end['reason'] == 'none'
    if ok_end and code == 0 and len(passes) == attendu['passes']:
        state = dict(etat='ok', raison='', passes=passes)
    elif not ok_end and code == 2 and len(passes) < attendu['passes'] and end['status'] in REFUSALS:
        state = dict(etat='refus', raison='%s/%s' % (end['status'], end['reason']), passes=passes)
    else:
        return dict(etat='echec', raison='sortie %s/%s, code %s, %d passes' % (end['status'], end['reason'], code,
                                                                             len(passes)), passes=passes)
    if open_row is not None:
        state['open_ns'] = open_row['wall_ns']
    return state
