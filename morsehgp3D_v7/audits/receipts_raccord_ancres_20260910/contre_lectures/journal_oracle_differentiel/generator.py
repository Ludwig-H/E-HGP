"""Generateur ALEATOIRE seede de journaux structurels valides et de journaux
invalides par mutation unique. Aucun assert ; les refus sont explicites.

Un journal valide couvre, selon les profils : naissances (population > K
possible), continuations (avec contributions, sans noeud), multifusions de
degre >= 2 (jusqu'a 6), contributions redondantes (meme population dans deux
actions ou deux fois dans une action), recouvrements de points entre racines
(populations partageant des points), chaines de successeurs de longueur >= 3
(profil `chain`), K1 (singletons a zero, puis fusions/continuations) et K>1,
masques de coquille jusqu'a 16 bits, niveaux rationnels non reduits et
numerateurs multi-limbs (jusqu'a 2^190) avec denominateurs jusqu'a 2^126.
"""

import random
from fractions import Fraction

from journal_model import (BANK_INVALID, BIRTH, EMPTY_BATCH, EMPTY_CONTINUATION, INVALID_DOMAIN,
                           INVALID_LEVEL, K1_ROOTS, MASK, NONINCREASING, PARENT, POPULATION_REF,
                           POSITIVE_LEVEL, Refusal, all_shell, refuse)


def level_repr(rng, value):
    """Representation [num, den] d'une Fraction, parfois non reduite."""
    scale = rng.choice([1, 1, 1, 2, 3, 7, 1 << 40, (1 << 100) + 7])
    num, den = value.numerator * scale, value.denominator * scale
    # bornes de representation produit : U192 et i128 > 0
    if num >= (1 << 192) or den >= (1 << 127):
        num, den = value.numerator, value.denominator
    return [str(num), str(den)]


def random_levels(rng, count, positive):
    """`count` niveaux rationnels strictement croissants ; le premier est 0 si non positive.

    Mode `big` : denominateur commun D < 2^121 et numerateurs < 2^186 (multi-limbs
    U192 cote produit), sans explosion des denominateurs par sommes."""
    mode = rng.choice(['small_int', 'small_frac', 'big', 'mixed'])
    values = []
    if mode == 'big':
        common = rng.getrandbits(rng.choice([1, 30, 64, 120])) + 1
        nums = set()
        while len(nums) < count:
            nums.add(rng.getrandbits(rng.choice([20, 64, 100, 150, 185])) + 1)
        values = [Fraction(k, common) for k in sorted(nums)]
        if not positive:
            values = [Fraction(0)] + values[:-1]
        return values
    current = Fraction(0)
    for i in range(count):
        if i == 0 and not positive:
            values.append(Fraction(0))
            continue
        if mode == 'small_int' or (mode == 'mixed' and rng.random() < 0.5):
            step = Fraction(rng.randint(1, 9))
        else:
            step = Fraction(rng.randint(1, 50), rng.randint(1, 12))
        current = current + step
        values.append(current)
    return values


class Builder:
    """Simule les identites du contrat : ids denses dans l'ordre des actions,
    continuations sautees, parents consommes par les multifusions."""

    def __init__(self, rng, order, domain, populations):
        self.rng = rng
        self.order = order
        self.domain = domain
        self.populations = populations
        self.live = []          # id -> bool
        self.batches = []
        self.birth_rows = [i for i, r in enumerate(populations)
                           if len(r['interior']) + len(r['shell']) >= order]
        self.singleton_row = {}  # point -> row index (K1)
        for i, r in enumerate(populations):
            pts = r['interior'] + r['shell']
            if len(pts) == 1 and pts[0] not in self.singleton_row:
                self.singleton_row[pts[0]] = i

    def live_ids(self):
        return [i for i, alive in enumerate(self.live) if alive]

    def full_ref(self, row_index):
        row = self.populations[row_index]
        return dict(population=row_index, shell_mask=all_shell(row), include_interior=bool(row['interior']))

    def random_ref(self, row_index=None):
        rng = self.rng
        if row_index is None:
            row_index = rng.randrange(len(self.populations))
        row = self.populations[row_index]
        include = bool(row['interior']) and rng.random() < 0.6
        mask = rng.getrandbits(len(row['shell'])) if row['shell'] else 0
        if rng.random() < 0.3:
            mask = all_shell(row)
        if not include and mask == 0:
            if row['shell']:
                mask = 1 << rng.randrange(len(row['shell']))
            else:
                include = True
        return dict(population=row_index, shell_mask=mask, include_interior=include)

    def random_contributions(self, minimum):
        rng = self.rng
        count = rng.randint(minimum, 3)
        refs = [self.random_ref() for _ in range(count)]
        if refs and rng.random() < 0.3:
            refs.append(dict(refs[0]))            # contribution redondante dans l'action
        return refs

    def birth_action(self):
        row_index = self.rng.choice(self.birth_rows)
        return dict(parents=[], contributions=[self.full_ref(row_index)])

    def add_batch(self, level, actions):
        for action in actions:
            if len(action['parents']) == 1:
                continue
            for p in action['parents']:
                self.live[p] = False
            self.live.append(True)
        self.batches.append(dict(level=level, actions=actions))

    def random_batch_actions(self, max_actions):
        rng = self.rng
        available = self.live_ids()
        rng.shuffle(available)
        actions = []
        for _ in range(rng.randint(1, max_actions)):
            choices = []
            if self.order > 1 and self.birth_rows:
                choices.append('birth')
            if available:
                choices += ['continuation'] * 2
            if len(available) >= 2:
                choices += ['merge'] * 3
            if not choices:
                break
            kind = rng.choice(choices)
            if kind == 'birth':
                actions.append(self.birth_action())
            elif kind == 'continuation':
                parent = available.pop()
                actions.append(dict(parents=[parent], contributions=self.random_contributions(1)))
            else:
                degree = min(len(available), rng.choice([2, 2, 2, 3, 3, 4, 5, 6]))
                parents = sorted(available.pop() for _ in range(degree))
                actions.append(dict(parents=parents, contributions=self.random_contributions(0)))
        if not actions:
            actions.append(self.birth_action())
        return actions


def random_domain(rng, n):
    space = rng.choice([n, n + 5, 1000, 1 << 20, 1 << 32])
    return sorted(rng.sample(range(space), n))


def random_population(rng, domain, order, force_birth=False, force_wide=False):
    n = len(domain)
    while True:
        u = 16 if force_wide else rng.choice([0, 1, 1, 2, 2, 3, 4, 5, 8, 12, 16])
        u = min(u, n)
        p = rng.choice([0, 0, 1, 2, 3, 5, 10])
        p = min(p, n - u)
        if force_birth and p + u < order:
            p = min(n - u, order - u)
            if p + u < order:
                u = min(n, order)
                p = 0
        if p + u == 0:
            continue
        chosen = rng.sample(domain, p + u)
        interior, shell = sorted(chosen[:p]), sorted(chosen[p:])
        return dict(interior=interior, shell=shell)


def make_valid(seed):
    """Un journal valide, deterministe par graine."""
    rng = random.Random(seed)
    profile = rng.choice(['random', 'random', 'random', 'chain', 'overlap', 'k1', 'wide_mask'])
    order = 1 if profile == 'k1' else rng.choice([1, 2, 2, 3, 3, 4, 5, 6, 8, 10])
    low = max(order, 2)
    n = rng.randint(low, max(low, rng.choice([6, 10, 20, 40])))
    domain = random_domain(rng, n)
    populations = []
    if order == 1:
        for point in domain:      # singletons obligatoires du niveau zero
            if rng.random() < 0.5:
                populations.append(dict(interior=[point], shell=[]))
            else:
                populations.append(dict(interior=[], shell=[point]))
    extra = rng.randint(1, 8)
    for i in range(extra):
        populations.append(random_population(rng, domain, order, force_birth=(i == 0 and order > 1),
                                             force_wide=(profile == 'wide_mask' and i == 0 and n >= 16)))
    if profile == 'overlap':
        # populations partageant des points : recouvrements entre racines
        base = populations[-1]
        pts = base['interior'] + base['shell']
        for _ in range(3):
            shared = rng.sample(pts, max(1, len(pts) // 2)) if pts else []
            others = rng.sample(domain, min(len(domain), 3))
            all_pts = sorted(set(shared) | set(others))
            cut = rng.randint(0, len(all_pts))
            populations.append(dict(interior=all_pts[:cut], shell=all_pts[cut:][:16]))
    builder = Builder(rng, order, domain, populations)
    batch_count = rng.randint(4, 9) if profile == 'chain' else rng.randint(1, 8)
    levels = random_levels(rng, batch_count, positive=(order > 1))
    reprs = [level_repr(rng, v) for v in levels]
    for b in range(batch_count):
        if order == 1 and b == 0:
            actions = [dict(parents=[], contributions=[builder.full_ref(builder.singleton_row[point])])
                       for point in domain]
        elif profile == 'chain':
            if b == 0:
                actions = [builder.birth_action(), builder.birth_action()]
            else:
                live = builder.live_ids()
                if len(live) >= 2:
                    merge = dict(parents=sorted(live[-2:]), contributions=builder.random_contributions(0))
                else:   # une seule racine vivante : continuation (jamais vide)
                    merge = dict(parents=[live[-1]], contributions=builder.random_contributions(1))
                actions = [merge]
                if order > 1 and builder.birth_rows:
                    actions.insert(rng.randrange(2), builder.birth_action())
                if b == batch_count - 1 and len(live) > 2 and rng.random() < 0.5:
                    actions.append(dict(parents=[live[0]], contributions=builder.random_contributions(1)))
        else:
            actions = builder.random_batch_actions(5)
        # verrou : aucun parent partage dans le lot (invariant de construction)
        used = [p for a in actions for p in a['parents']]
        refuse(len(used) == len(set(used)), 'generator_parent_reuse')
        builder.add_batch(reprs[b], actions)
    cuts = []
    for i in range(batch_count):
        low = levels[i - 1] if i else levels[0] - 1
        cuts.append(level_repr(rng, (low + levels[i]) / 2))            # intermediaire (ou sous le premier)
        cuts.append([str(levels[i].numerator * 5), str(levels[i].denominator * 5)])  # meme niveau, autre representation
    cuts.append(level_repr(rng, levels[-1] + Fraction(1, 3)))
    if rng.random() < 0.4:
        cuts.append(['1', '0'])          # coupe invalide (den nul)
    if rng.random() < 0.3:
        cuts.append(['3', '-2'])         # coupe invalide (den negatif)
    if cuts[0][0].startswith('-'):
        cuts[0] = ['0', '1']
    return dict(name='valid_%04d' % seed, seed=seed, profile=profile, order=order, domain=domain,
                populations=populations, batches=builder.batches, cuts=cuts)


# ---------------------------------------------------------------------------
# Mutations uniques. Chaque fonction rend (journal mute, raison primaire attendue)
# ou None si elle ne s'applique pas au journal de base.

def deep(journal):
    import json
    return json.loads(json.dumps(journal))


def find_action(journal, predicate, rng):
    found = [(b, a) for b, batch in enumerate(journal['batches'])
             for a, action in enumerate(batch['actions']) if predicate(b, action)]
    return rng.choice(found) if found else None


def is_birth(action):
    return not action['parents']


def is_continuation(action):
    return len(action['parents']) == 1


def is_merge(action):
    return len(action['parents']) >= 2


def m_domain_empty(j, rng):
    j['domain'] = []
    return j, BANK_INVALID


def m_domain_unsorted(j, rng):
    if len(j['domain']) < 2:
        return None
    i = rng.randrange(len(j['domain']) - 1)
    j['domain'][i], j['domain'][i + 1] = j['domain'][i + 1], j['domain'][i]
    return j, BANK_INVALID


def m_domain_duplicate(j, rng):
    if len(j['domain']) < 2:
        return None
    i = rng.randrange(1, len(j['domain']))
    j['domain'][i] = j['domain'][i - 1]
    return j, BANK_INVALID


def m_rows_empty(j, rng):
    j['populations'] = []
    return j, BANK_INVALID


def m_row_empty(j, rng):
    i = rng.randrange(len(j['populations']))
    j['populations'][i] = dict(interior=[], shell=[])
    return j, BANK_INVALID


def m_row_shell_17(j, rng):
    if len(j['domain']) < 17:
        return None
    i = rng.randrange(len(j['populations']))
    j['populations'][i] = dict(interior=[], shell=sorted(rng.sample(j['domain'], 17)))
    return j, BANK_INVALID


def m_row_interior_unsorted(j, rng):
    rows = [i for i, r in enumerate(j['populations']) if len(r['interior']) >= 2]
    if not rows:
        return None
    r = j['populations'][rng.choice(rows)]
    r['interior'][0], r['interior'][1] = r['interior'][1], r['interior'][0]
    return j, BANK_INVALID


def m_row_shell_unsorted(j, rng):
    rows = [i for i, r in enumerate(j['populations']) if len(r['shell']) >= 2]
    if not rows:
        return None
    r = j['populations'][rng.choice(rows)]
    r['shell'][-1], r['shell'][-2] = r['shell'][-2], r['shell'][-1]
    return j, BANK_INVALID


def m_row_point_outside_domain(j, rng):
    i = rng.randrange(len(j['populations']))
    r = j['populations'][i]
    outside = max(j['domain']) + 1 if max(j['domain']) < (1 << 32) - 1 else 0
    if outside in j['domain']:
        return None
    key = 'shell' if r['shell'] else 'interior'
    r[key][-1] = outside
    return j, BANK_INVALID


def m_row_shell_in_interior(j, rng):
    rows = [i for i, r in enumerate(j['populations']) if r['interior'] and len(r['shell']) < 16]
    if not rows:
        return None
    r = j['populations'][rng.choice(rows)]
    r['shell'] = sorted(set(r['shell']) | {r['interior'][0]})
    return j, BANK_INVALID


def m_order_zero(j, rng):
    j['order'] = 0
    return j, INVALID_DOMAIN


def m_order_eleven(j, rng):
    j['order'] = 11
    return j, INVALID_DOMAIN


def m_batches_empty(j, rng):
    j['batches'] = []
    return j, INVALID_DOMAIN


def m_order_gt_domain(j, rng):
    if len(j['domain']) >= 10:
        return None
    j['order'] = len(j['domain']) + 1
    return j, INVALID_DOMAIN


def m_level_den_zero(j, rng):
    b = rng.randrange(len(j['batches']))
    j['batches'][b]['level'][1] = '0'
    return j, INVALID_LEVEL


def m_level_den_negative(j, rng):
    b = rng.randrange(len(j['batches']))
    j['batches'][b]['level'][1] = '-' + j['batches'][b]['level'][1]
    return j, INVALID_LEVEL


def m_level_equal_previous(j, rng):
    if len(j['batches']) < 2:
        return None
    b = rng.randrange(1, len(j['batches']))
    j['batches'][b]['level'] = list(j['batches'][b - 1]['level'])
    return j, NONINCREASING


def m_level_equal_other_representation(j, rng):
    if len(j['batches']) < 2:
        return None
    b = rng.randrange(1, len(j['batches']))
    num, den = j['batches'][b - 1]['level']
    j['batches'][b]['level'] = [str(int(num) * 3), str(int(den) * 3)]
    if int(num) * 3 >= (1 << 192) or int(den) * 3 >= (1 << 127):
        return None
    return j, NONINCREASING


def m_level_decreasing(j, rng):
    if len(j['batches']) < 2:
        return None
    b = rng.randrange(1, len(j['batches']))
    num, den = j['batches'][b - 1]['level']
    j['batches'][b]['level'] = [str(int(num)), str(int(den) * 2 if int(num) else 1)]
    if int(num) == 0:
        return None
    return j, NONINCREASING


def m_batch_empty_actions(j, rng):
    b = rng.randrange(len(j['batches']))
    j['batches'][b]['actions'] = []
    return j, EMPTY_BATCH


def m_level_zero_k_gt_1(j, rng):
    if j['order'] == 1:
        return None
    j['batches'][0]['level'] = ['0', j['batches'][0]['level'][1]]
    return j, POSITIVE_LEVEL


def m_k1_first_level_positive(j, rng):
    if j['order'] != 1:
        return None
    j['batches'][0]['level'] = ['1', '7']
    if len(j['batches']) > 1:
        nxt = Fraction(int(j['batches'][1]['level'][0]), int(j['batches'][1]['level'][1]))
        if nxt <= Fraction(1, 7):
            j['batches'][0]['level'] = ['1', str(max(8, nxt.denominator * 1000))]
    return j, K1_ROOTS


def m_k1_missing_birth(j, rng):
    if j['order'] != 1:
        return None
    j['batches'][0]['actions'].pop()
    return j, K1_ROOTS


def m_k1_birth_order_swapped(j, rng):
    if j['order'] != 1 or len(j['domain']) < 2:
        return None
    acts = j['batches'][0]['actions']
    acts[0], acts[1] = acts[1], acts[0]
    return j, K1_ROOTS


def m_k1_late_birth(j, rng):
    if j['order'] != 1 or len(j['batches']) < 2:
        return None
    b = rng.randrange(1, len(j['batches']))
    row = rng.randrange(len(j['populations']))
    r = j['populations'][row]
    j['batches'][b]['actions'].append(dict(parents=[], contributions=[
        dict(population=row, shell_mask=all_shell(r), include_interior=bool(r['interior']))]))
    return j, K1_ROOTS


def m_k1_non_singleton_birth(j, rng):
    if j['order'] != 1:
        return None
    rows = [i for i, r in enumerate(j['populations']) if len(r['interior']) + len(r['shell']) >= 2]
    if not rows:
        return None
    row = rng.choice(rows)
    r = j['populations'][row]
    a = rng.randrange(len(j['batches'][0]['actions']))
    j['batches'][0]['actions'][a]['contributions'] = [
        dict(population=row, shell_mask=all_shell(r), include_interior=bool(r['interior']))]
    return j, K1_ROOTS


def m_k1_wrong_point(j, rng):
    if j['order'] != 1 or len(j['domain']) < 2:
        return None
    acts = j['batches'][0]['actions']
    a = rng.randrange(len(acts))
    other = (a + 1) % len(acts)
    acts[a]['contributions'] = [dict(acts[other]['contributions'][0])]
    return j, K1_ROOTS


def m_parents_unsorted(j, rng):
    hit = find_action(j, lambda b, a: is_merge(a), rng)
    if not hit:
        return None
    b, a = hit
    j['batches'][b]['actions'][a]['parents'].reverse()
    return j, PARENT


def m_parents_duplicate(j, rng):
    hit = find_action(j, lambda b, a: is_merge(a), rng)
    if not hit:
        return None
    b, a = hit
    p = j['batches'][b]['actions'][a]['parents']
    p[1] = p[0]
    return j, PARENT


def m_parent_future(j, rng):
    # un parent cree dans le meme lot (id >= prior_count)
    counts = []
    ids = 0
    for batch in j['batches']:
        counts.append(ids)
        ids += sum(1 for a in batch['actions'] if not is_continuation(a))
    candidates = [(b, a) for b, batch in enumerate(j['batches']) for a, act in enumerate(batch['actions'])
                  if not is_birth(act) and any(not is_continuation(x) for x in batch['actions'])]
    if not candidates:
        return None
    b, a = rng.choice(candidates)
    act = j['batches'][b]['actions'][a]
    act['parents'] = sorted(set(act['parents'][:-1]) | {counts[b]}) if len(act['parents']) > 1 else [counts[b]]
    if len(act['parents']) == 1 and not act['contributions']:
        act['contributions'] = [dict(population=0, shell_mask=all_shell(j['populations'][0]),
                                     include_interior=bool(j['populations'][0]['interior']))]
    return j, PARENT


def m_parent_out_of_range(j, rng):
    hit = find_action(j, lambda b, a: not is_birth(a), rng)
    if not hit:
        return None
    b, a = hit
    j['batches'][b]['actions'][a]['parents'][-1] = (1 << 64) - 2
    return j, PARENT


def m_parent_dead(j, rng):
    # un parent deja consomme par une multifusion d'un lot anterieur
    dead_after = {}
    ids = 0
    for b, batch in enumerate(j['batches']):
        for act in batch['actions']:
            if is_merge(act):
                for p in act['parents']:
                    dead_after.setdefault(p, b)
            if not is_continuation(act):
                ids += 1
    candidates = [(b, a, p) for p, dead_b in dead_after.items()
                  for b, batch in enumerate(j['batches']) if b > dead_b
                  for a, act in enumerate(batch['actions']) if not is_birth(act)]
    if not candidates:
        return None
    b, a, p = rng.choice(candidates)
    act = j['batches'][b]['actions'][a]
    act['parents'] = sorted(set(act['parents'][:-1]) | {p}) if len(act['parents']) > 1 else [p]
    if len(act['parents']) == 1 and not act['contributions']:
        act['contributions'] = [dict(population=0, shell_mask=all_shell(j['populations'][0]),
                                     include_interior=bool(j['populations'][0]['interior']))]
    return j, PARENT


def m_parent_shared_in_batch(j, rng):
    candidates = [(b, a) for b, batch in enumerate(j['batches']) for a, act in enumerate(batch['actions'])
                  if not is_birth(act)]
    if not candidates:
        return None
    b, a = rng.choice(candidates)
    act = j['batches'][b]['actions'][a]
    j['batches'][b]['actions'].append(dict(parents=[act['parents'][0]], contributions=[
        dict(population=0, shell_mask=all_shell(j['populations'][0]),
             include_interior=bool(j['populations'][0]['interior']))]))
    return j, PARENT


def m_continuation_empty(j, rng):
    hit = find_action(j, lambda b, a: is_continuation(a), rng)
    if not hit:
        return None
    b, a = hit
    j['batches'][b]['actions'][a]['contributions'] = []
    return j, EMPTY_CONTINUATION


def pick_ref(j, rng, predicate):
    found = [(b, a, c) for b, batch in enumerate(j['batches']) for a, act in enumerate(batch['actions'])
             for c, ref in enumerate(act['contributions']) if predicate(act, ref)]
    return rng.choice(found) if found else None


def m_population_out_of_range(j, rng):
    hit = pick_ref(j, rng, lambda act, ref: True)
    if not hit:
        return None
    b, a, c = hit
    j['batches'][b]['actions'][a]['contributions'][c]['population'] = len(j['populations'])
    return j, POPULATION_REF


def m_mask_outside_shell(j, rng):
    hit = pick_ref(j, rng, lambda act, ref: len(j['populations'][ref['population']]['shell']) < 16)
    if not hit:
        return None
    b, a, c = hit
    ref = j['batches'][b]['actions'][a]['contributions'][c]
    ref['shell_mask'] |= 1 << len(j['populations'][ref['population']]['shell'])
    return j, MASK


def m_include_interior_empty_interior(j, rng):
    hit = pick_ref(j, rng, lambda act, ref: not j['populations'][ref['population']]['interior'])
    if not hit:
        return None
    b, a, c = hit
    j['batches'][b]['actions'][a]['contributions'][c]['include_interior'] = True
    return j, MASK


def m_mask_zero_no_interior(j, rng):
    hit = pick_ref(j, rng, lambda act, ref: not is_birth(act))
    if not hit:
        return None
    b, a, c = hit
    ref = j['batches'][b]['actions'][a]['contributions'][c]
    ref['shell_mask'] = 0
    ref['include_interior'] = False
    return j, MASK


def m_birth_two_contributions(j, rng):
    hit = find_action(j, lambda b, a: is_birth(a), rng)
    if not hit:
        return None
    b, a = hit
    act = j['batches'][b]['actions'][a]
    act['contributions'].append(dict(act['contributions'][0]))
    return j, BIRTH


def m_birth_zero_contributions(j, rng):
    hit = find_action(j, lambda b, a: is_birth(a), rng)
    if not hit:
        return None
    b, a = hit
    j['batches'][b]['actions'][a]['contributions'] = []
    return j, BIRTH


def m_birth_partial_mask(j, rng):
    hit = find_action(j, lambda b, a: is_birth(a) and
                      len(j['populations'][a['contributions'][0]['population']]['shell']) >= 1 and
                      (a['contributions'][0]['include_interior'] or
                       len(j['populations'][a['contributions'][0]['population']]['shell']) >= 2), rng)
    if not hit:
        return None
    b, a = hit
    ref = j['batches'][b]['actions'][a]['contributions'][0]
    ref['shell_mask'] &= ~1
    return j, BIRTH


def m_birth_missing_interior_flag(j, rng):
    hit = find_action(j, lambda b, a: is_birth(a) and a['contributions'][0]['include_interior'] and
                      a['contributions'][0]['shell_mask'] != 0, rng)
    if not hit:
        return None
    b, a = hit
    j['batches'][b]['actions'][a]['contributions'][0]['include_interior'] = False
    return j, BIRTH


def m_birth_population_smaller_than_order(j, rng):
    if j['order'] == 1:
        return None
    rows = [i for i, r in enumerate(j['populations']) if len(r['interior']) + len(r['shell']) < j['order']]
    hit = find_action(j, lambda b, a: is_birth(a), rng)
    if not rows or not hit:
        return None
    b, a = hit
    boundary = [i for i in rows if len(j['populations'][i]['interior']) + len(j['populations'][i]['shell']) == j['order'] - 1]
    row = rng.choice(boundary) if boundary else rng.choice(rows)
    r = j['populations'][row]
    j['batches'][b]['actions'][a]['contributions'] = [
        dict(population=row, shell_mask=all_shell(r), include_interior=bool(r['interior']))]
    return j, BIRTH


MUTATIONS = [
    m_domain_empty, m_domain_unsorted, m_domain_duplicate, m_rows_empty, m_row_empty, m_row_shell_17,
    m_row_interior_unsorted, m_row_shell_unsorted, m_row_point_outside_domain, m_row_shell_in_interior,
    m_order_zero, m_order_eleven, m_batches_empty, m_order_gt_domain,
    m_level_den_zero, m_level_den_negative, m_level_equal_previous, m_level_equal_other_representation,
    m_level_decreasing, m_batch_empty_actions, m_level_zero_k_gt_1,
    m_k1_first_level_positive, m_k1_missing_birth, m_k1_birth_order_swapped, m_k1_late_birth,
    m_k1_non_singleton_birth, m_k1_wrong_point,
    m_parents_unsorted, m_parents_duplicate, m_parent_future, m_parent_out_of_range, m_parent_dead,
    m_parent_shared_in_batch, m_continuation_empty, m_population_out_of_range, m_mask_outside_shell,
    m_include_interior_empty_interior, m_mask_zero_no_interior, m_birth_two_contributions,
    m_birth_zero_contributions, m_birth_partial_mask, m_birth_missing_interior_flag,
    m_birth_population_smaller_than_order,
]


def make_invalid(seed, base):
    """Applique UNE mutation (choisie par la graine) a un journal valide."""
    rng = random.Random(seed * 7919 + 13)
    order = list(MUTATIONS)
    rng.shuffle(order)
    for mutation in order:
        result = mutation(deep(base), rng)
        if result is None:
            continue
        journal, expected = result
        journal['name'] = 'invalid_%04d_%s' % (seed, mutation.__name__[2:])
        journal['mutation'] = mutation.__name__[2:]
        journal['expected_reason'] = expected
        journal['base'] = base['name']
        return journal
    raise Refusal('no_applicable_mutation')
