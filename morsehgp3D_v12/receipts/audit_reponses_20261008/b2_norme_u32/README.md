# B2-C : deux fixtures u32 certifiées exigeant une norme i128

8 octobre 2026, sources au commit `a6e3634fcce7f6988a00bdbaacb15488daf4a401`. Réponse constructive au point encore
ouvert de `reponse_audit_b2_census.md`. Aucun défaut du calcul actuel trouvé : les témoins justifient et peuvent
qualifier sa branche i128. Python rationnel uniquement, aucun moteur/compilation/GPU ; fixtures publiques de deux
points, sans données externes ni coefficients forgés.

Un segment suffit. Pour A=(0,0,0), B=(b,0,0), la fabrique publique `CertifiedBall::certify({A,B})` certifie toujours
le milieu dès que A≠B. `Sphere::through` stocke exactement N=(b,0,0), D=2, ancre A. Son centre est (b/2,0,0), son
rayon carré b²/4. L'étendue est `bit_width(b)`, pas log₂(b) arrondi vers le bas. Les deux cas sont :

| Paramètre | Somme i64 insuffisante | Chaque carré i64 insuffisant |
| --- | ---: | ---: |
| b | 536870912 = 2²⁹ | 2147483648 = 2³¹ |
| Étendue s | 30 | 32 |
| Centre x | 268435456 | 1073741824 |
| Rayon carré | 72057594037927936 | 1152921504606846976 |
| X=(q,q,q), q | 2147483647 = 2³¹−1 | 4294967295 = 2³²−1 |
| Norme 3q² | 13835058042397261827 | 55340232195358851075 |
| Puissance locale D‖X‖²−2N·X | 25364273076654571526 | 92233720321303117830 |
| Signe attendu | +1 extérieur | +1 extérieur |

Tous ces points appartiennent au profil 32. Avec M=2ˢ, X reste STRICTEMENT dans le pavé (−M,2M)³ : la garde ne
court-circuite pas le calcul. Le certificat de domaine teste D<2^(123−2t) et |N_j|<2^(124−t). Ici son exposant
maximal est 60 (t=60 passe, t=61 échoue car D=2 n'est pas strictement inférieur à 2). Il couvre donc s+2=32 ou 34.
Les coefficients tiennent en i128 même si leur TYPE de stockage au profil 32 est large. `GuardedSphere` choisit
**Lane::certified**, pas `wide`, et `short_norm_` est faux. Les produits et sommes effectifs tiennent largement
en i128 (puissances positives sur 65 et 67 bits). Le modèle compare cette expression entière à une géométrie
indépendante en `Fraction` : ‖X−c‖²−R² = puissance/D. Il vérifie aussi l'autre ancre du segment, N devenant négatif.

Attention à deux distinctions : le prédicat générique q2 `side(Sphere,Point)` peut compter `native`, suivant sa
propre règle d'arité ; le compteur exigé ici est celui de **GuardedSphere**. Et la branche i128 commence à s=29,
mais ce n'est pas encore un dépassement : |v_j|<2^(s+1) implique ‖v‖²<3·2⁶⁰<2⁶³ pour TOUT support gardé de s≤29.
s=30 est donc la première étendue où un dépassement est possible. Les contrôles s=28 (branche courte) et s=29
(branche longue encore conservatrice) sont inclus ; aucun élargissement du seuil n'est proposé.

## Raccord conseillé à une porte native u32

Construire les objets avec `Point::make`, `CertifiedBall::certify` et `GuardedSphere`, sans exposer ses membres privés.
Vérifier span, centre/coefficient exacts, `power_domain()==60`, `lane()==certified`, puis :

- A et B : signe 0 ; milieu : −1 ; X : +1, via `side_site`, façade `side(Point)` et `side_offset`, avec registres
  frais : un appel arithmétique donne `(native,certified,checked,wide)=(0,1,0,0)`, `outside_sites=0`.
- Boîte `[A,X]` : bornes (−1,+1), deux évaluations certifiées ; boîte ponctuelle `[X,X]` : (+1,+1), une évaluation
  certifiée car le minorant positif arrête la fonction. Comparer également à `local_test::power` dans `tests/num/local_reference.hpp` (384 bits, sans choix de voie).
- Pour s=30, (2³¹,0,0) est sur la frontière OUVERTE du pavé, donc extérieur sans arithmétique et `outside_sites=1`.
  La frontière positive du pavé s=32 est hors du domaine u32 ; ne pas fabriquer ce Point.

Une compilation des portes u21/u24 ne joue pas ces fixtures ; déclarer explicitement leur exécution au profil 32.
La porte `guard_site_lanes` existante couvre les étendues 16,17,19,20,21 ; elle n'atteint pas ces deux cas.

Forcer directement la somme signée i64 introduirait un comportement indéfini C++ : aucune valeur native de ce
mutant n'est prédite. Le lecteur Python fournit seulement un témoin DÉFINI de troncature signée 64, qui retourne
un signe négatif à la place de +1 dans les deux cas. Une future porte UBSan peut détecter le vrai débordement ;
un mutant géométrique portable peut appliquer explicitement cette réduction définie avant la conversion i128.
Aucun mutant compilé n'est prétendu tué ici, ni qualification complète du moteur u32.

```sh
python check.py
python -O check.py
```

Les deux sorties doivent être identiques à `results.json`. Le script est autonome ; `capture.json` conserve les
épingles du raccord C++, dont les constantes numériques ne sont pas exécutées par ce modèle.

Contrelecture mathématique indépendante : admission de ces deux supports, domaine 60 et choix de voie certifiée
confirmés par lecture du même commit ; aucune exécution native supplémentaire.
