"""Modele INDEPENDANT du journal FULL date (schema full_dated_coverage_forest_v2).

Semantique rejouee depuis le contrat de l'auditeur (LOCAL_DIAGNOSTICS §1) et
le contrat constructeur (CONTRAT_COUVERTURES_DATEES.md), sans reprendre le
code C++ :
  - coupe ouverte a t : seuls les niveaux STRICTEMENT inferieurs a t sont admis ;
    coupe fermee : jusqu'a t inclus ;
  - la racine d'un segment a une coupe suit les successeurs admis A CETTE COUPE,
    jamais la racine finale ;
  - une continuation conserve l'identite (aucun noeud) ;
  - la couverture d'une racine est l'UNION ensembliste des contributions admises
    de tous les segments dont la racine a la coupe est cette racine ; aucun
    dedoublonnage entre racines (recouvrements conserves).
Aucun assert : tout refus est explicite (liste de codes de violation).
"""

from fractions import Fraction

# Codes de refus, nommes comme les raisons du produit pour la comparaison ;
# ils sont calcules ici a partir du contrat, pas copies du code.
BANK_INVALID = 'coverage_invalid_population'
INVALID_DOMAIN = 'coverage_invalid_domain'
INVALID_LEVEL = 'coverage_invalid_level'
NONINCREASING = 'coverage_nonincreasing_batch'
EMPTY_BATCH = 'coverage_empty_batch'
POSITIVE_LEVEL = 'coverage_positive_level_required'
K1_ROOTS = 'coverage_k1_roots'
PARENT = 'coverage_parent_not_unique_prebatch_root'
EMPTY_CONTINUATION = 'coverage_empty_continuation'
POPULATION_REF = 'coverage_population_reference'
MASK = 'coverage_empty_or_invalid_mask'
BIRTH = 'coverage_birth_population'

MAX_ORDER = 10
MAX_SHELL_BITS = 16
U64_MAX = (1 << 64) - 1
U16_MAX = (1 << 16) - 1
U32_MAX = (1 << 32) - 1
U192_MAX = (1 << 192) - 1
I128_MAX = (1 << 127) - 1
ABSENT = None


class Refusal(Exception):
    """Refus explicite d'une entree que le modele ne represente pas."""


def refuse(condition, reason):
    if not condition:
        raise Refusal(reason)


def parse_level(raw):
    """[num_str, den_str] -> (Fraction | None, representable).

    None signifie den <= 0 (niveau invalide au sens du produit). La
    representation est bornee par le format produit (U192 / i128 > 0)."""
    refuse(isinstance(raw, list) and len(raw) == 2, 'level_shape')
    num, den = int(raw[0]), int(raw[1])
    refuse(0 <= num <= U192_MAX, 'level_num_unrepresentable')
    refuse(-I128_MAX - 1 <= den <= I128_MAX, 'level_den_unrepresentable')
    if den <= 0:
        return None
    return Fraction(num, den)


def strictly_increasing(values):
    return all(values[i - 1] < values[i] for i in range(1, len(values)))


def bank_violations(domain, populations):
    """Codes fins de la banque ; tout code non vide -> BANK_INVALID cote produit."""
    codes = []
    if not domain:
        codes.append('bank:domain_empty')
    if not populations:
        codes.append('bank:rows_empty')
    if not strictly_increasing(domain):
        codes.append('bank:domain_not_strictly_increasing')
    domain_set = set(domain)
    for row in populations:
        interior, shell = row['interior'], row['shell']
        if len(shell) > MAX_SHELL_BITS:
            codes.append('bank:shell_wider_than_mask')
        if not interior and not shell:
            codes.append('bank:row_empty')
        if not strictly_increasing(interior):
            codes.append('bank:interior_not_strictly_increasing')
        if not strictly_increasing(shell):
            codes.append('bank:shell_not_strictly_increasing')
        if any(p not in domain_set for p in interior) or any(p not in domain_set for p in shell):
            codes.append('bank:point_outside_domain')
        if set(interior) & set(shell):
            codes.append('bank:shell_meets_interior')
    return codes


def all_shell(row):
    return (1 << len(row['shell'])) - 1


def journal_violations(journal):
    """Ensemble (liste ordonnee, sans doublon) des codes de refus du journal."""
    codes = []

    def add(code):
        if code not in codes:
            codes.append(code)

    order = journal['order']
    domain = journal['domain']
    populations = journal['populations']
    batches = journal['batches']
    bank_codes = bank_violations(domain, populations)
    bank_ok = not bank_codes
    if not (1 <= order <= MAX_ORDER) or not bank_ok or not populations or len(domain) < order or not batches:
        add(INVALID_DOMAIN)
        return codes, bank_codes
    previous = None
    live = []            # identite -> vivante avant le lot courant
    for b, batch in enumerate(batches):
        level = parse_level(batch['level'])
        if level is None:
            add(INVALID_LEVEL)
        elif previous is not None and not (previous < level):
            add(NONINCREASING)
        if level is not None:
            previous = level
        actions = batch['actions']
        if not actions:
            add(EMPTY_BATCH)
        if order > 1 and level is not None and level == 0:
            add(POSITIVE_LEVEL)
        if order == 1 and b == 0 and ((level is not None and level != 0) or len(actions) != len(domain)):
            add(K1_ROOTS)
        prior_count = len(live)
        consumed = set()
        for a, action in enumerate(actions):
            parents = action['parents']
            contributions = action['contributions']
            parents_ok = (strictly_increasing(parents) and
                          all(0 <= p < prior_count and live[p] and p not in consumed for p in parents))
            if not parents_ok:
                add(PARENT)
            consumed.update(p for p in parents if 0 <= p < prior_count)
            if len(parents) == 1 and not contributions:
                add(EMPTY_CONTINUATION)
            for ref in contributions:
                pop = ref['population']
                if not (0 <= pop < len(populations)):
                    add(POPULATION_REF)
                    continue
                row = populations[pop]
                if (ref['shell_mask'] & ~all_shell(row)) or (ref['include_interior'] and not row['interior']) \
                        or (not ref['include_interior'] and ref['shell_mask'] == 0):
                    add(MASK)
            if not parents:
                birth_ok = len(contributions) == 1
                if birth_ok:
                    ref = contributions[0]
                    if 0 <= ref['population'] < len(populations):
                        row = populations[ref['population']]
                        size = len(row['interior']) + len(row['shell'])
                        birth_ok = (ref['include_interior'] == bool(row['interior']) and
                                    ref['shell_mask'] == all_shell(row) and size >= order)
                        if birth_ok and order == 1:
                            point = row['interior'][0] if row['interior'] else row['shell'][0]
                            if b != 0 or size != 1 or a >= len(domain) or point != domain[a]:
                                add(K1_ROOTS)
                    else:
                        birth_ok = False
                if not birth_ok:
                    add(BIRTH)
        if codes:
            # L'etat des racines n'est plus defini apres un refus ; on ne
            # poursuit pas la simulation des lots suivants.
            break
        # commit du lot : identites dans l'ordre des actions, continuations sautees
        for action in actions:
            parents = action['parents']
            if len(parents) == 1:
                continue
            for p in parents:
                live[p] = False
            live.append(True)
    return codes, bank_codes


def build(journal):
    """Construit la foret (noeuds, successeurs, contributions datees).

    Precondition : journal_violations(journal) vide."""
    nodes = []          # dict(level=Fraction, raw=[num,den], parents=[ids])
    successor = []      # id -> id | None
    contributions = []  # dict(level=Fraction, raw, segment, population, shell_mask, include_interior)
    for batch in journal['batches']:
        level = parse_level(batch['level'])
        for action in batch['actions']:
            parents = action['parents']
            if len(parents) == 1:
                segment = parents[0]
            else:
                segment = len(nodes)
                nodes.append(dict(level=level, raw=list(batch['level']), parents=list(parents)))
                successor.append(ABSENT)
                for p in parents:
                    successor[p] = segment
            for ref in action['contributions']:
                contributions.append(dict(level=level, raw=list(batch['level']), segment=segment,
                                          population=ref['population'], shell_mask=ref['shell_mask'],
                                          include_interior=ref['include_interior']))
    return dict(nodes=nodes, successor=successor, contributions=contributions,
                populations=journal['populations'])


def admitted(level, cut, closed):
    return level < cut or (closed and level == cut)


def root_at(forest, segment, cut, closed):
    """Racine du segment a la coupe (historique, jamais la racine finale)."""
    nodes, successor = forest['nodes'], forest['successor']
    if cut is None or segment is None or not (0 <= segment < len(nodes)):
        return ABSENT
    if not admitted(nodes[segment]['level'], cut, closed):
        return ABSENT
    while successor[segment] is not ABSENT and admitted(nodes[successor[segment]]['level'], cut, closed):
        segment = successor[segment]
    return segment


def points_of(row, ref):
    values = set()
    if ref['include_interior']:
        values.update(row['interior'])
    for bit, point in enumerate(row['shell']):
        if ref['shell_mask'] & (1 << bit):
            values.add(point)
    return values


def coverage_at(forest, root, cut, closed):
    """Union des contributions admises dont le segment a pour racine `root`."""
    if root is ABSENT or root_at(forest, root, cut, closed) != root:
        return dict(status='invalid_input', reason='full_invalid_read', values=[])
    values = set()
    for record in forest['contributions']:
        if not admitted(record['level'], cut, closed):
            continue
        if root_at(forest, record['segment'], cut, closed) != root:
            continue
        values |= points_of(forest['populations'][record['population']], record)
    return dict(status='ok', reason='structural_only', values=sorted(values))


def coverage_table(forest, cut, closed):
    """Toutes les couvertures d'une coupe : racine de chaque noeud par marche
    naive (sans memo ni tableau inverse), puis une passe sur les contributions."""
    roots = [root_at(forest, i, cut, closed) for i in range(len(forest['nodes']))]
    table = {}
    if cut is None:          # coupe invalide (den <= 0) : aucune racine, aucune lecture
        return roots, table
    for record in forest['contributions']:
        if not admitted(record['level'], cut, closed):
            continue
        root = roots[record['segment']]
        if root is ABSENT:
            continue
        table.setdefault(root, set()).update(points_of(forest['populations'][record['population']], record))
    return roots, table


def raw_cut(raw):
    """Une coupe [num, den] : Fraction, ou None si den <= 0."""
    return parse_level(raw)


def evaluate_cut(forest, raw, closed, origin):
    cut = raw_cut(raw)
    n = len(forest['nodes'])
    roots, table = coverage_table(forest, cut, closed)
    rows = []
    for probe in list(range(n)) + [n, ABSENT]:
        root = roots[probe] if (probe is not ABSENT and probe < n) else ABSENT
        live = root is not ABSENT and root == probe
        if live:
            read = dict(status='ok', reason='structural_only', values=sorted(table.get(probe, set())))
        else:
            read = dict(status='invalid_input', reason='full_invalid_read', values=[])
        rows.append(dict(id=probe, root=root, live=live, read=read))
    return dict(level=[str(int(raw[0])), str(int(raw[1]))], closed=closed, origin=origin, nodes=rows)


def evaluate(journal):
    """Sortie attendue, de meme forme que celle du pont C++."""
    codes, bank_codes = journal_violations(journal)
    out = dict(name=journal.get('name', ''), order=journal['order'])
    if bank_codes:
        out['bank'] = dict(status='invalid_input', reason=BANK_INVALID, value=False)
    else:
        out['bank'] = dict(status='ok', reason='structural_only', value=True)
    if codes:
        out['build'] = dict(status='invalid_input', reasons=codes, empty=True, order=0)
        out['bank_codes'] = bank_codes
        return out
    out['build'] = dict(status='ok', reason='structural_only', order=journal['order'], empty=False)
    forest = build(journal)
    out['nodes'] = [dict(id=i, level=[str(int(n['raw'][0])), str(int(n['raw'][1]))], parents=n['parents'],
                         successor=forest['successor'][i]) for i, n in enumerate(forest['nodes'])]
    out['contributions'] = [dict(level=[str(int(c['raw'][0])), str(int(c['raw'][1]))], segment=c['segment'],
                                 population=c['population'], shell_mask=c['shell_mask'],
                                 include_interior=c['include_interior']) for c in forest['contributions']]
    cuts = []
    if journal.get('batch_cuts', True):
        for batch in journal['batches']:
            for closed in (False, True):
                cuts.append(evaluate_cut(forest, batch['level'], closed, 'batch'))
    for raw in journal.get('cuts', []):
        for closed in (False, True):
            cuts.append(evaluate_cut(forest, raw, closed, 'extra'))
    out['cuts'] = cuts
    return out
