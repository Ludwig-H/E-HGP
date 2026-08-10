# Note de Claude — le juge Γ_k est écrit, et la frontière Q1 est mesurée

Date : 10 août 2026, même session que
[`QUESTIONS_CLAUDE_FORET_50K_20260810.md`](QUESTIONS_CLAUDE_FORET_50K_20260810.md).
Suite directe de votre décision Q1 (« NO-GO exact hors position générale » tant
que l'équivalence n'est ni prouvée ni testée) : voici la moitié TEST, avec sa
première mesure. À auditer.

## Ce qui est écrit

Premier snapshot de `oracle/gamma_forest_judge.cpp` (binaire
`mhgp3v_gamma_judge`, cinq CTests à ce palier, six après la correction en fin de
note) :

- **vérité** : Γ_k du théorème 2 du manuscrit, calculée par votre protocole de
  lot Q1.2 — coupe stricte figée, toutes les activations du niveau appliquées,
  composantes du lot entier, classification par racines strictes distinctes
  (0 naissance, 1 continuation, ≥2 multifusion). Arithmétique rationnelle de
  l'oracle, miniboules par `exact_miniball` (géométrie extraite telle quelle
  dans `oracle/exact_geometry.hpp`, partagée avec M1, jamais dupliquée) ;
- **sujet** : `mhgp3v::flat_catalogue` + `mhgp::build_forest`, lus comme des
  données (niveaux exacts recalculés depuis les entiers des sphères) ;
- **comparaison** : partitions d'identifiants couverts à CHAQUE niveau
  d'événement, coupe fermée (la coupe ouverte d'un niveau étant la fermée du
  précédent, les deux coupes sont couvertes). Classification
  structure/couverture ;
- **domaine** : la dégénérescence est exactement la Def. 26 — un point
  surnuméraire SUR une sphère d'événement. Ni le rang (toute face obtuse a des
  intérieurs génériques), ni les niveaux égaux d'effondrement de miniboule, ni
  les multifusions portées par une seule coface (licites jusqu'à l'arité k+1),
  ni les naissances simultanées des singletons au niveau zéro. J'ai d'abord
  sur-déclenché sur ces quatre cas ; les trois retraits sont commentés dans le
  fichier.

## Ce qui est mesuré

- **Hors cosphéricité (coord 40³, 30 nuages, k ≤ 3)** : accord EXACT partout —
  la chaîne rend les partitions de Γ_k à chaque niveau, aux deux coupes. La
  porte permanente l'exige (RC 1 sinon).
- **Grille saturée (4³, 20 nuages, k ≤ 3)** : **36 ordres sur 60 en désaccord
  de STRUCTURE, zéro censure de rang** (s_max=11 suffisait). Le diagnostic
  montre la classe : le sujet SOUS-FUSIONNE. Exemple (nuage 0, ordre 2) :
  vérité `{{0,1,2,3,6,7},{0,6},{4,7},{5,6}}` contre sujet
  `{{0,3,7},{0,6},{1,2,6,7},{2,3},{4,7},{5,6}}`.

**Cause identifiée** : une coquille cosphérique de rang r > k+1 porte, au
niveau de sa sphère, le lot de fusion de ses (k+1)-faces ; la sphère est AU
catalogue (rang ≤ s_max), mais la forêt d'ordre k ne lit que les rangs k et
k+1 — l'événement est invisible. Votre phrase « une coquille de rang supérieur
peut porter des faces de cardinal k ou k+1 » est donc mesurée, et elle mord dès
r = k+2 SANS aucune censure de s_max.

## Question (complément à Q1)

Le théorème manquant se laisse-t-il énoncer ainsi : *pour toute sphère critique
de coquille S et de niveau β, et pour chaque k < |S| (rang fermé), le lot des
(k+1)-sous-ensembles de S fusionne au niveau β exactement les composantes de
Γ_k portées par les k-faces de S ; et la LECTURE ÉLARGIE du fold — l'ordre k
lit toute sphère de rang ≥ k+1 dont la coquille a au moins k+1 points — rend
les partitions de Γ_k à tous les niveaux* ? Si oui, la sémantique v3 du fold
est fixée et le juge Γ_k est déjà sa porte ; sinon, quel est le contre-exemple ?

Deux sous-questions :
1. les k-faces d'une coquille de rang r naissent-elles TOUTES au plus tard à β
   (leur miniboule est incluse dans la boule de S), si bien que la lecture
   élargie n'a besoin d'aucune face implicite ANTÉRIEURE hors catalogue ?
2. l'intérieur strict de la sphère (points de B∖S) participe-t-il aux amas
   couverts d'ordre k au niveau β via des faces mixtes (points de S et de
   l'intérieur), et la couverture les récupère-t-elle par les sphères de rang
   inférieur déjà lues ?

## Limites déclarées

Le juge est exhaustif (C(n, k+1) miniboules rationnelles) : n ≤ 14, K ≤ 6 par
contrat CLI. Il juge la chaîne catalogue→forêt, pas le parcours par flats. Ses
planchers garantissent l'exercice des deux régimes, mais la porte saturée ne
fige pas le compte de divergences : une lecture élargie correcte les ferait
légitimement tomber à zéro.

## CORRECTION, après application de votre audit live — la mesure change de sens

Votre correction n°4 (échec fermé des autorités du sujet) a invalidé ma mesure
« 36/60 ordres en sous-fusion structurelle » : je comparais comme complètes des
forêts que le sujet avait LUI-MÊME censurées (`Forest::authoritative == false`).
Après le fail-closed, la conclusion est différente et meilleure :

- générique (40³) : **78 ordres jugés hors cosphéricité, accord des couvertures
  à toutes les coupes testées**, aux coupes stricte ET fermée, sur l'UNION des
  niveaux vérité+sujet ; 3 ordres refusés par censure du sujet ;
- saturée (4³) : **40/60 ordres refusés par censure DÉCLARÉE — tous les ordres
  $k=2,3$ —, 20/20 ordres $k=1$ jugés en accord de couverture**.

**Aucune divergence silencieuse n'est observée dans ces deux campagnes : les
autres ordres sont censurés et déclarés.** La frontière Q1 mesurée est une
censure déclarée, pas une preuve de sous-fusion silencieuse. L'exemple de
sous-fusion de ma première version provenait d'un
payload censuré — je le retire. La question de la lecture élargie reste posée,
mais reformulée : il ne s'agit pas de corriger une divergence, il s'agit de
REMPLACER la censure par un calcul (votre Ω_{k,c} ou la lecture élargie), jugé
par cet oracle.

Vos quatre autres corrections sont appliquées : partitions de facettes
conservées comme projection séparée (le claim ne couvre que les couvertures),
comparaison aux deux coupes sur l'union des niveaux (mutant
`--force-shift-level` tué par sa porte, sans recevoir séparément la boucle
stricte et l'union : leurs mutants orthogonaux restent verts), classification map<PointId,·> retirée
(étiquettes compte/contenu descriptives), statuts non kOk et parents hors plage
refusés avant comparaison. Six portes permanentes, dont la carte de frontière
qui se déclare CARTE (aucun OK d'exactitude quand aucun ordre n'est jugé hors
dégénérescence).
