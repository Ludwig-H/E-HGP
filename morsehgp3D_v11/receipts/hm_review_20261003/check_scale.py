#!/usr/bin/env python3
"""H_m : plafond en niveaux carrés, contre-garde et correction constructive.

Source math relue : WIP e26b48055d8e8a8ef665e1c6bab296db82168903,
docs/HIERARCHIE_POINTS.md, SHA256
088dd06718367203b4a20497c2f234e29cecbc692f6e63d23fbf2a343af8b9eb.
H3 lignes94--106 affirme 3*delta pour les entrées et 5*delta pour les
réunions, puis convertit P5 avec delta=2*epsilon*sqrt(Lambda)+epsilon^2,
Lambda décrit comme « niveau de la racine ». Il faut distinguer une
naissance de racine FULL des dates de couverture et de qualification.
La copie math WIP est seulement identifiée par son empreinte ; aucun
code développeur ni natif n'est importé/exécuté par cette preuve.

Petit témoin : quatre sites sur le cercle de centre(5,5), rayon5 :
x=(0,5), a=(10,5), b=(8,9), c=(8,1). À k=3, abc entre à16. Les trois
parties contenant x entrent à25 et rejoignent ce même composant à la
coupe fermée. FULL3 possède un seul nœud, racine née à16. La couverture
contient trois sites entre16 et25, puis quatre à25, sans nouveau nœud.
Pour m=4, chaque première qualification est25 ; R_i^(4) est la même
branche racine à partir de25. Donc chaque retard D_i=0, chaque entrée
H_4 vaut25, et toutes les réunions de paires valent25, au-delà de16.

Contre-garde ENTIER u21 apparié : cercle de rayon85, ancien triangle de
rayon13 ; homothétie86/85 autour du centre, puis multiplication commune
par85 et translation(10000,10000,0). Les nuages X/Y ci-dessous ont
rayons7225/7310 et anciens rayons1105/1118. Chaque site bouge de85,
car 84^2+13^2=85^2. Les racines FULL3 naissent à1221025/1249924 ;
toutes les entrées et réunions H_4 valent52200625/53436100, D_i=0.
Avec Lambda=max des deux naissances racine=1118^2, delta=197285.
Le changement1235475 dépasse 3*delta=591855 et 5*delta=986425.
Cela réfute la conversion avec CE plafond, pas H3 sous l'hypothèse
d'un entrelacement de niveau delta transportant les couvertures.

Correction suffisante : Lambda=max(beta(MEB(X)), beta(MEB(Y))) pour
les nuages appariés complets, ou tout plafond commun certifié au moins
aussi grand. À ce niveau toutes les k-parties et cofaces sont actives,
Gamma_k est connexe, sa couverture est tout X. Pour m<=n, t_i<=Lambda.
Tout rival de hauteur h<=Lambda a rencontre<=Lambda, donc
D_i<=Lambda-t_i ; les points sur la branche racine après Lambda ont
terme de retard nul. Ainsi e_i=t_i+D_i<=Lambda et u(i,j)<=Lambda.
P5 donne le décalage constant 2*epsilon*sqrt(Lambda)+epsilon^2 sur ce
préfixe ; au-delà, on prolonge les cartes sur l'unique branche racine
qualifiée couvrant tous les sites. Ce plafond inclut les continuations.
Ici Lambda=53436100=7310^2 et delta=1249925 : les bornes corrigées
contiennent bien la variation. Aucun calcul natif de MEB globale n'est
proposé/qualifié par ce reçu ; une borne géométrique supérieure certifiée
peut remplacer sa valeur exacte dans une future API.

H3 abstrait, preuve courte. Les cartes phi/psi de niveau delta préservent
R_i^(m), par inclusion des mêmes IDs couverts, et leurs compositions sont
les remontées à2*delta. D'où |t_X-t_Y|<=delta. Pour q dans R_i^Y,
h(q)>=t_Y, l'ultramétrie des rencontres via p_X donne
m_X(psi(p_Y),psi(q)) <= D_X+h(q)+delta.
Puis m_Y(p_Y,q)<=m_X(psi(p_Y),psi(q))+delta, donc
D_Y<=D_X+2*delta ; symétrie et e=t+D donnent |Delta e|<=3*delta.

Amélioration : |Delta u|<=3*delta également, sous un VRAI entrelacement.
En effet, psi(p_Y) appartient à R_X et a hauteur t_Y+delta. Donc
m_X(p_X,psi(p_Y))<=D_X+t_Y+delta<=e_X+2*delta.
Appliquer phi donne m_Y(phi(p_X),phi(psi(p_Y)))<=e_X+3*delta.
La composition est EXACTEMENT Up_Y(p_Y,t_Y+2*delta). Remonter un point
ne diminue pas le niveau ABSOLU de rencontre, donc
m_Y(phi(p_X),p_Y)<=e_X+3*delta.
Pour les propriétaires finaux g_X=Up_X(p_X,e_X), g_Y=Up_Y(p_Y,e_Y),
la commutation aux remontées implique
m_Y(phi(g_X),g_Y)=max(e_X+delta,e_Y,m_Y(phi(p_X),p_Y))<=e_X+3*delta.
La chaîne triangulaire entre g_Yi, phi(g_Xi), phi(g_Xj), g_Yj donne
u_Y(i,j)<=max(e_Xi+3*delta,u_X(i,j)+delta,e_Xj+3*delta)
          <=u_X(i,j)+3*delta,
car u_X(i,j)>=e_Xi,e_Xj pour i!=j. Symétrie. La diagonale0 séparée est
triviale. Ce résultat utilise les identités phi*psi=Up(2*delta),
psi*phi=Up(2*delta), pas seulement deux inclusions ou un seul transport.
Il ne démontre ni constante sharp ni stabilité d'IoU/topologie sélectionnée.
Les transports préservent la qualification, pas nécessairement l'égalité
des cardinalités de couverture. m>n rend R_i^(m) vide : il exige une
politique inactive/refus, hors de cette preuve de pendaison finie.

H4 : la branche racine infinie force kappa>=1 pour la famille proposée.
Les constantes (1+2*kappa) et (1+4*kappa) sont des BORNES obtenues par
une preuve ; kappa1 minimise ces bornes dans cette famille. Cela ne
prouve pas une optimalité statistique, une constante optimale, ni que
H_1 serait le meilleur extracteur ou le plus stable parmi toutes les
règles fidèles. Cette nuance ne modifie aucun résultat mesuré.

Contrôle autonome borné : réutilise notre Gram/Fraction/Gamma indépendant
de full_points_20261003/experiment/test_qualified.py. Ni ses main/imports
optionnels de références ni les bancs développeur ne sont exécutés.
Diagonale de coassociation fixée à0 ; entrées publiées séparément.
"""

from fractions import Fraction as Q
from hashlib import sha256
import json
from math import isqrt
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
EXPERIMENT = HERE.parent / "full_points_20261003/experiment"
sys.path.insert(0, str(EXPERIMENT))
from test_qualified import Oracle  # noqa: E402; independent bounded audit model

CHECKS = 0


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def root_and_h4(points):
    oracle = Oracle(points, 3)
    tree = oracle.trees[3]
    require(len(tree["nodes"]) == 1, "one FULL3 node; later parts are continuations")
    root = oracle.levels[tree["nodes"][0]["rank"]]
    entries = []
    for site in range(len(points)):
        entries.append(next(beta for beta in oracle.levels
                            if any(site in coverage and len(coverage) >= 4
                                   for coverage in ({i for part in component for i in part}
                                                    for component in oracle.gamma(3, beta)))))
    require(len(set(entries)) == 1, "common first qualification of all four sites")
    require(entries[0] > root, "first qualified coverage later than root birth")
    require(entries[0] == oracle.meb(tuple(range(4)))[0], "global enclosure exactly qualification date")
    # One root and common qualification: all qualified R_i have only this
    # lineage, so their meet-h(q) is identically0, without sampling infinity.
    pair_heights = [[Q(0) if i == j else max(entries[i], entries[j]) for j in range(4)]
                    for i in range(4)]
    for beta in oracle.levels:
        components = oracle.gamma(3, beta)
        require(len(components) <= 1, "at most one continuous FULL3 component at every closed cut")
        coverage = set(i for component in components for part in component for i in part)
        require(len(coverage) == (0 if beta < root else 3 if beta < entries[0] else 4),
                "exact coverage0/3/4 on the sole branch")
    return dict(points=points, root_birth=root, qualified_first=entries, entries=entries,
                delay=[Q(0)]*4, pair_heights=pair_heights,
                meb_triples={str(part): oracle.meb(part)[0] for part in oracle.parts[3]},
                global_meb=oracle.meb(tuple(range(4)))[0])


def sqrt_exact(value):
    result = Q(isqrt(value.numerator), isqrt(value.denominator))
    require(result*result == value, "perfect-square rational radius")
    return result


def serial(value):
    if isinstance(value, Q):
        return str(value)
    if isinstance(value, dict):
        return {key: serial(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [serial(item) for item in value]
    return value


def run():
    small = root_and_h4([(0, 5, 0), (10, 5, 0), (8, 9, 0), (8, 1, 0)])
    require(small["root_birth"] == 16 and small["entries"] == [Q(25)]*4, "R5 small witness")
    x = [(2775, 10000, 0), (17225, 10000, 0), (17140, 11105, 0), (17140, 8895, 0)]
    y = [(2690, 10000, 0), (17310, 10000, 0), (17224, 11118, 0), (17224, 8882, 0)]
    original, moved = root_and_h4(x), root_and_h4(y)
    epsilon = Q(85)
    require(len(set(x)) == len(set(y)) == 4 and all(0 <= coordinate < 2**21
                                                  for p in x+y for coordinate in p), "unit distinct u21 sites")
    require(all(sum((a-b)**2 for a, b in zip(p, q)) == epsilon**2 for p, q in zip(x, y)),
            "all four matched point displacements exactly85")
    root_plafond = max(original["root_birth"], moved["root_birth"])
    delta_root = 2*epsilon*sqrt_exact(root_plafond)+epsilon**2
    entry_changes = [abs(a-b) for a, b in zip(original["entries"], moved["entries"])]
    pair_changes = [abs(original["pair_heights"][i][j]-moved["pair_heights"][i][j])
                    for i in range(4) for j in range(i+1, 4)]
    require(root_plafond == 1249924 and delta_root == 197285, "exact mistaken root plafond")
    require(entry_changes == [Q(1235475)]*4 and all(change > 3*delta_root for change in entry_changes),
            "all four entries violate the claimed root-based3delta")
    require(pair_changes == [Q(1235475)]*6 and all(change > 5*delta_root for change in pair_changes),
            "all six pairs violate the claimed root-based5delta")
    global_plafond = max(original["global_meb"], moved["global_meb"])
    delta_global = 2*epsilon*sqrt_exact(global_plafond)+epsilon**2
    require(global_plafond == 53436100 and delta_global == 1249925, "exact corrected common global plafond")
    require(all(change <= 3*delta_global for change in entry_changes)
            and all(change <= 5*delta_global for change in pair_changes), "corrected conversion contains all changes")
    require(all(change <= 3*delta_global for change in pair_changes), "stronger abstract3delta pair bound contains all changes")
    dependencies = {"check_scale.py": sha256(Path(__file__).read_bytes()).hexdigest(),
                    "../full_points_20261003/experiment/test_qualified.py": sha256((EXPERIMENT / "test_qualified.py").read_bytes()).hexdigest(),
                    "../full_points_20261003/experiment/qualified.py": sha256((EXPERIMENT / "qualified.py").read_bytes()).hexdigest()}
    return serial(dict(status="pass", schema="ehgp.v11.hm_scale_exact_check.v1", checks=CHECKS,
                       scope="WIP mathematical conversion only; not native H_m qualification",
                       k=3, m=4, height_units="squared_grid_radius", epsilon=epsilon, small_R5=small,
                       X=original, Y=moved, entry_changes=entry_changes, pair_changes=pair_changes,
                       mistaken_root_plafond=root_plafond, mistaken_delta=delta_root,
                       mistaken_entry_bound=3*delta_root, mistaken_pair_bound=5*delta_root,
                       corrected_global_plafond=global_plafond, corrected_delta=delta_global,
                       corrected_entry_bound=3*delta_global, corrected_pair_bound=3*delta_global,
                       entry_violations=4, pair_violations=6, abstract_H3="not refuted",
                       stronger_abstract_pair_bound="3delta; true interleaving identities and ascent commutation required; not claimed sharp",
                       document_wip=dict(head="e26b48055d8e8a8ef665e1c6bab296db82168903",
                                         path="morsehgp3D_v11/docs/HIERARCHIE_POINTS.md",
                                         sha256="088dd06718367203b4a20497c2f234e29cecbc692f6e63d23fbf2a343af8b9eb"),
                       dependencies=dependencies, product_or_reference_imports=False, native_runs=0, fits=0))


if __name__ == "__main__":
    print(json.dumps(run(), ensure_ascii=False, sort_keys=True, separators=(",", ":")))
