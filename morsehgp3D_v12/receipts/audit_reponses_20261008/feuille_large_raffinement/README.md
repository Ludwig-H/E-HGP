# Feuille large : raffiner le domaine des centres, avec certificat

8 octobre 2026, pin `8dc66863fc8cb89690384d462a2a9b010a87ca7c`. **Proposition sans gain mesuré**,
complément de [T1-c](../feuille_large_proposition/README.md) pour CST-0237. Aucun moteur, GCP ou payload lu.

**Orientation unique :** avant d'énumérer tous les supports d'une feuille large, autoriser un
raffinement dyadique exact de sa **boîte de centres** sous l'unité, puis réappliquer le filtre
des K témoins sur la liste parentale entière. Actuellement `traversal_kernels.hpp:348` ferme
la feuille dès que la largeur maximale est au plus1 ; `traversal_driver.hpp:173` refuse ensuite
au-delà de256 candidats. Le filtre exact existe déjà (`traversal_records.hpp:214`,
`FilterKernel`) ; la proposition étend son domaine de boîtes, pas sa règle de rejet.

Les huit refus MES-C2 `wide_leaf` ne donnent ni le nombre de boules critiques ni leurs coquilles.
La borne par tuples de T1-c borne le travail d'une énumération, **pas une taille minimale de
l'objet** : plusieurs présentations peuvent définir une même boule. Le coût total reste inconnu.

## Lemme de séparation locale

Pour deux sites x,y, poser Δxy(c)=||x−c||²−||y−c||². Cette différence est affine et, si
||c−c₀||∞≤ε,

    Δxy(c) ≥ Δxy(c₀) − 2ε ||x−y||₁.

Si K témoins distincts y donnent chacun une borne **strictement positive**, x peut être
éliminé sur toute la fermeture de la boîte. L'égalité doit être conservée. Ce certificat
ne dépend d'aucun support déjà énuméré et préserve toute liste K-certifiée dans chaque enfant.

Fixons c₀ et notons d_(K) la **K-ième statistique d'ordre des distances des sites**, avec
multiplicités, non la K-ième distance distincte. Soit L={x : ||x−c₀||²≤d_(K)} ; toute la
classe d'égalité contenant le K-ième site appartient à L. Pour x hors L et y dans L,
δxy=Δxy(c₀)>0. Sur une famille finie de sites, choisir

    ε ≤ min_{x hors L, y dans L} δxy / (4 ||x−y||₁)

rend toutes ces dominances strictes. Si L est toute la liste, il n'y a rien à éliminer.
Les 3K premiers sites au milieu de la petite boîte comprennent alors au moins K membres
de L ; ils éliminent tous les sites hors L. Aucun membre de L ne peut être éliminé, car en
c₀ il possède moins de K sites strictement plus proches. **Le résidu est exactement L.**
Un certificat parental valide ne pouvait avoir retiré un membre de L au centre c₀ qu'il contient.

Cette profondeur finie est **locale**, sans borne globale. L peut dépasser256, même si la boule
particulière cherchée a une petite coquille : le seuil porte sur le K-ième site global.
Un groupe massif de contacts égaux à ce seuil persiste à toute profondeur. Le raffinement
seul ne traite donc pas les dégénérescences, et une boîte sans émission peut aussi rester
ambiguë. Aucun O(n log n) ni terminaison globale nouvelle n'en découle.

## Raccord et coût à déclarer avant essai

- Partager les centres entre enfants demi-ouverts, mais filtrer/copier **tous les candidats
  parentaux pertinents** dans chacun. Ne jamais couper les sites en nuages indépendants :
  les supports croisés, le census global, S* et le propriétaire du centre restent requis.
- Des bornes dyadiques de profondeur h ajoutent des bits : anciennes voies i64/i128,
  repère, limite3B et `path[2]` à requalifier. Aucun arrondi vers des entiers. Budget,
  profondeur et refus doivent être explicites, avec repli exact borné.
- Si V boîtes sont visitées avec listes m_v, le filtre actuel teste au plus3K témoins par
  candidat : coût de filtrage O(K Σm_v), **plus sélection des témoins**, fronts, copies et
  feuilles restantes. Listes simultanées :4Σm_v octets de SiteIdx, plus descripteurs et
  arithmétique élargie. V et Σm_v ne sont pas bornés ici ; dupliquer les listes peut coûter.
- Nouvelles feuilles et nouveaux tests changent le ledger de parcours et potentiellement
  celui des feuilles. C'est un changement déclaré d'algorithme, pas une optimisation aux
  compteurs nécessairement identiques. Mesurer C puis FULL, avec objets canoniques comparés.

**Témoin positif borné.** Aux coordonnées relatives (±1000,0,0),(0,±1001,0), K1, la boîte
[0,1]³ conserve trois candidats. Sur [0,1/4]³, le même filtre exact en conserve deux.
Une translation positive entière place les sites dans u21 sans changer ce résultat.
Le carré exact conserve au contraire ses quatre contacts dans toute boîte contenant son
centre : aucune dominance stricte ne peut les séparer. Ce n'est pas une mesure des sphères MES-C2.

`check.py` confronte le filtre à des distances Fraction indépendantes aux coins et sur une
grille bornée ; il vérifie aussi le préfixe incluant toutes les égalités, et refuse deux
mutations (dominance non stricte ; arrêt après K sites). Pins et résultats joints.
Python normal/−O ; produit inchangé.
