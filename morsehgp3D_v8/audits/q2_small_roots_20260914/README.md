# Racines q2 singleton : simplification exacte, gain non établi

14 septembre 2026. Audit indépendant A, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`phase=exploration_v8_hors_registre`, `public_status=not_claimed`.
GCP non utilisé. Sources produit **e3af11a7b2ecba4a71929c7112610ff2618f813e**,
raccord [Pool/paires précédent](../q2_pool_bridge_20260914/README.md)
explicitement épinglé. Aucun fichier produit modifié ; les qualifications
du census conjoint b2106c3c ne sont pas transférées à ce prototype.

**Décision proposée : conserver Complement/sibling pour les petits
rectangles du raccord Pool.** Le remplacement par Global sur les seules
racines initiales singleton est correct, mais les 30 mesures ne montrent
pas de gain stable. Il enlève surtout des étapes structurelles en ajoutant
des tests géométriques. Ce résultat évite de porter une variante de plus
sans bénéfice démontré ; il ne condamne pas une spécialisation ultérieure.

## Pourquoi le changement est exact

Le front oriente A par taille croissante : si B est singleton, A l'est
aussi. Le branchement intervient uniquement à cette **racine initiale**,
dans un rectangle non sélectionné par Pool, avant toute consommation.
Le compte vaut zéro et l'ordre B est encore celui de l'index global.
Les descendants de tâches et les préfixes issus de Pool restent inchangés.

Pour la paire fixée, $H_{ab}(z)=(z-a)\cdot(b-z)$ et $H_{ab}(a)=H_{ab}(b)=0$.
Différer B et exclure l'ancre ne changent donc pas l'ordre relatif des
témoins stricts. Global peut parcourir l'index entier dès le départ.
Les trois bras comptent globalement, saturent à K, puis collectent tous
les intérieurs et toute la coquille avant le même callback de vérification.
`pair_task` démarre lui-même sa racine : aucun `root_start` supplémentaire.

Global peut élaguer une boîte contenant une extrémité et de maximum H nul,
là où Complement descend structurellement. Il paie toutefois des bornes
sur les ancêtres que Complement divisait sans test géométrique.
Le gate conserve la fixture a=(1,1,0), b=(3,1,0), z=(0,0,0), ainsi qu'un
cas à deux sites isolant l'exécution des trois politiques.

Les deux Global diffèrent uniquement par leur mécanique de parcours :
`shared_task<false,false>` itératif ou `pair_task` récursif. Leurs visites,
bornes, tests ponctuels, populations consommées et parcours de collecte
sont strictement identiques dans les configurations mesurées et le gate.
Le certificat frère n'est déjà jamais actif à ces racines ; la préparation
de 48 octets est déjà absente pour une requête singleton. Aucun gain sur
ces deux postes ne peut être attribué à ce changement.

## Temps de toute la chaîne q2

Secondes, un CPU fixé, hôte partagé, sans échauffement. Kmax=10,
front `MidpointSamples`, Pool/paires au seuil 64 dans les trois bras.
Les préparations, destructions, collecte, copie/tri/contrôle et digest
des supports complets sont payés. Ce sont des temps locaux d'audit,
pas des temps de tour FULL sur G4. Les données LiDAR reprennent les
[entrées KITTI08 préparées](../lidar08_20260914/README.md), sans nouvelle
acquisition et sans hypothèse d'alignement exact entre points.

| Entrée | s | Complement | Global itératif | Global récursif |
| --- | ---: | ---: | ---: | ---: |
| Scan 0, 8k | 8 | 1,654 | 1,595 | 1,606 |
| Scan 100, 8k | 8 | 1,395 | 1,382 | 1,372 |
| Scan 200, 8k | 8 | 1,471 | 1,532 | 1,488 |
| Scan 0, 16k | 8 | 3,532 | 3,539 | 3,531 |
| Scan 0, 32k | 8 | 6,796 | 6,819 | 6,961 |
| Scan 0, 50k | 8 | 11,027 | 11,304 | 11,275 |
| Scan 0, 50k, ordre inversé | 8 | 11,209 | 11,335 | 11,373 |
| Scan 0, 8k | 10 | 1,666 | 1,665 | 1,699 |
| Scan 0, 8k | 12 | 1,741 | 1,759 | 1,723 |
| Amas, 8k, graine 3 | 8 | 3,133 | 3,117 | 3,224 |

À 50k, **1 040 458 racines** sont concernées. Les deux Global suppriment
26 178 298 opérations structurelles mais ajoutent 24 105 792 visites
géométriques : 23 707 654 tests de boîte et 398 138 tests ponctuels.
Leurs totaux sont 338 039 900 visites contre 313 934 108 pour Complement.
Ces catégories ne sont pas des unités de coût interchangeables.
La répétition conserve exactement tout le travail discret et ne renverse
pas le constat temporel. Les petits écarts sur cet hôte partagé ne
permettent pas de déclarer un vainqueur universel.

Front, Pool, certificat frère et sorties restent identiques. À 50k,
1 040 133 supports portent 4 619 158 IDs intérieurs et 2 213 035 IDs de
coquille. La collecte **avec callback d'audit** pèse environ 1,3 s dans
cette capture. Le temps restant mélange front et comptage ; les reçus
ne permettent pas de l'appeler « temps du front ».

## Suite constructive : différer les boîtes tangentes

Pour une paire fixée et une boîte Z, poser $c=\frac{a+b}{2}$ et $H_{ab}(z)=\frac{\lVert a-b\rVert^2}{4}-\lVert z-c\rVert^2$.
Cette fonction strictement concave possède un unique maximiseur sur Z,
la projection coordonnée par coordonnée de c dans la boîte. **Si son
maximum vaut zéro, Z contient au plus un site de coquille.** Il faut
vérifier que ce maximiseur est réellement un site : pour a=(0,0,0),
b=(10,0,0), la boîte du nœud {(8,5,0),(9,4,0)} touche la sphère en
(8,4,0), absent du nuage. Les deux sites sont strictement extérieurs.
Cette propriété n'exige aucun alignement exact des points.

Proposition non implémentée : sur une racine de paire Global fraîche,
mémoriser au plus K−1 IDs intérieurs et C emplacements typés, chacun
contenant soit un ID de coquille testé, soit un descripteur de boîte de
maximum nul. **Résoudre ces dernières seulement après admission**, par
recherche de l'unique maximiseur dans leur sous-arbre. Au rejet, jeter
le cache ; au débordement, invalider seulement le cache et conserver
la collecte globale actuelle après admission. Ni émission provisoire
ni coquille tronquée. Cela évite les recherches de tangence sur les paires
rejetées, mais les copies intérieures et descripteurs avant rejet restent
payés. Le cap peut être atteint par des tangences sans site.
Sous Complement, il faudrait aussi traiter les extrémités sautées ; une
continuation héritée doit conserver les obligations du préfixe consommé.

Le [petit gate géométrique](tangent_shell_gate.py) vérifie les tangences
présente/absente et une coquille de 30 sites. À K=1, le premier bloc du
DFS de cette dernière contient 21 sites de coquille avant un intérieur
qui fera rejeter la paire : un tampon de 8 slots ne suffit pas. Ce gate
ne teste pas une implémentation du cache et ne mesure aucune accélération.
Son résultat séparé figure dans [CHECKS.json](CHECKS.json). Mesurer
obligations, débordements, recherches finales, mémoire par tâche, copies
sur admis/rejetés et travail de repli avant de décider d'un portage.

## Preuves et reproduction

[small_bridge.hpp](small_bridge.hpp) et [probe.cpp](probe.cpp) sont des
adaptations déclarées des sources d'audit précédentes ; `node_pool.hpp`
et `support_io.hpp` restent importés sans changement, avec hashes imposés.
Le census produit est inclus une seule fois, sans modification, depuis
un snapshot Git neuf sous ce dossier. Les archives embarquent les sources
produit, adaptations et dépendances exactes.

- [Gate indépendant](gate.cpp) : 10 nuages de 2 à 69 sites,
  K=1/2/5/10, s=8/10/12 ; 360 flux, 120 comparaisons des deux Global,
  32 643 supports dont 3 240 avec coquille supplémentaire. Il compare
  paires, clés, IDs intérieurs et coquilles à un oracle scalaire exhaustif.
- [Release r1](r1_BUILD.json) : gate passé, probe compilé avec
  `-Wall -Wextra -Wpedantic -Werror`, rejet sans arguments vérifié.
- [Clang ASan/UBSan r2](san_r2_BUILD.json) : même gate passé,
  détection des fuites activée. [san_r1](san_r1_BUILD.json) conserve
  l'échec environnemental de LeakSanitizer sous ptrace. Le sanitizer
  couvre le gate et le raccord, pas le lecteur/digest du probe.
- Six campagnes closes, 30 lignes et 27 configurations distinctes.
  Chaque exécution conserve commande, entrée, stdout, code de retour,
  affinité, charge et hashes. Un signal différé ou un timeout reste
  un échec, avec stdout conservé. Les grands nuages vérifient des digests
  et des compteurs ; l'oracle géométrique indépendant est borné au gate.
- [Clôture normale](VALIDATION.json) et [optimisée Python](VALIDATION_OPTIMIZED.json)
  identiques : dix mutations de reçus rejetées, dix comparaisons exactes
  avec le parent Pool/paires et répétition 50k à travail discret constant.

Depuis la racine, utiliser des noms de build neufs ; aucun reçu ni
snapshot existant n'est écrasé :

```bash
python3 -B morsehgp3D_v8/audits/q2_small_roots_20260914/build.py --name replay
python3 -B morsehgp3D_v8/audits/q2_small_roots_20260914/build.py --name san_replay --sanitize
python3 -B morsehgp3D_v8/audits/q2_small_roots_20260914/verify.py
python3 -B -O morsehgp3D_v8/audits/q2_small_roots_20260914/verify.py
python3 -B morsehgp3D_v8/audits/q2_small_roots_20260914/tangent_shell_gate.py
```

Les campagnes initiales utilisent `measure.py --plan <nom>` et le build
r1 ; leur rejeu sans exécution utilise `--validate campaign_<nom>` avec
le chemin complet du dossier. La capture est volontairement immuable.
Le lecteur de clôture vérifie aussi les binaires locaux épinglés sous
`.build` ; une compilation dans un dossier neuf produit son propre reçu,
sans remplacer ces anciennes preuves ni leurs hashes.
P0, q3/q4, FULL, multi-CPU/GPU, tour 50k et plusieurs dizaines de millions
de points restent ouverts.
