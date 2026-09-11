# MEB hybride : refus valide à K=7 et repli de confinement

11 septembre 2026. Audit indépendant du prototype du second auditeur, base
`99b4d3b1`. CPU u16 hors registre, `public_status=not_claimed`. GCP non utilisé.
Aucune source active ou source du second auditeur modifiée.

## Résultat

La fixture ordonnée suivante possède sept sites distincts u16 :

```text
(2,3,2), (2,0,0), (0,2,2), (1,0,0), (2,2,0), (3,0,1), (0,2,3)
```

`anchor_meb` renvoie `kOk`, support canonique `{0,1,5,6}`, coquille de taille 4.
L’hybride figé renvoie `kInvariantViolated`, raison `canon_fail`. Sa proposition
Welzl renvoie pourtant succès avec une puissance positive `176` au site 6.
Les puissances entières de cette proposition sont `0,-16,0,0,-96,0,176`.
La canonicalisation recontrôle le confinement et refuse ; il s’agit donc d’un
refus d’entrée valide, sans faux succès géométrique démontré.

La récursion borne `R` à quatre sites mais son cas de base calcule la MEB de
`R`, pouvant placer certains de ces sites strictement à l’intérieur. Cela ne
réalise pas la contrainte de frontière de la récursion de Welzl. Le théorème
« quatre points suffisent à définir la MEB finale » ne justifie pas ce remplacement.

Une vérification rationnelle indépendante fournit la vraie boule : support
`{0,1,5,6}`, centre `(29/22,23/22,37/22)`, rayon carré `193/44`, coefficients
barycentriques `(1/22,3/11,5/22,5/11)`. Les puissances normalisées des sept sites
sont `0,0,-18/11,-4/11,-2/11,0,0`. Les coefficients sont positifs, leur somme
vaut un, et tous les sites sont confinés : cette boule est la MEB unique.
La proposition erronée a pour support `{0,2,3,5}`, centre
`(29/18,23/18,23/18)`, rayon carré `131/36`, coefficients
`(5/18,2/9,2/9,5/18)` ; le dernier site a puissance `22/9 > 0`.

## Réparation constructive testée

`guarded_run.hpp` est une copie locale déclarée de `run`, avec repli vers
`anchor_meb` si Welzl échoue, si une puissance positive apparaît pendant le
balayage existant de coquille, ou si la canonicalisation ne trouve pas de support.
Le cas valide ne paie pas un second balayage de confinement. Ce garde ne répare
pas profondément Welzl ; il empêche cette proposition incomplète de restreindre
la recherche canonique à une mauvaise coquille.

Le travail de proposition reste conservé séparément du travail de repli : sur
la fixture, 167 candidats et 415 puissances avant un appel nominal supplémentaire
(71 puissances). Aucun travail déjà effectué n’est effacé. Le contrôle positif
collinéaire de sept sites ne se replie pas. Les six champs exacts comparés sont
`status`, `key`, `level`, `support_size`, `support_slots`, `selected_shell_count`.
Le garde rend PASS en O2 et sous ASan/UBSan avec détection des fuites ; les deux
sorties sont identiques. Une seconde compilation indépendante depuis l’archive
figée reproduit ces deux résultats dans [root_replay.json](root_replay.json),
avec 56 headers et les quatre sources locales inchangés avant/après. La TU CUDA
archivée est distincte et n’est pas compilée. Les branches `welzl=false` et dernier recours de
canonicalisation sont présentes, mais seules les deux fixtures décrites sont
exercées ; aucune couverture générale des branches n’est revendiquée.

## Captures et reproduction

`probe.stdout` conserve le premier échec (K=10, 307 985e essai). `fixture.stdout`
conserve sa réduction à K=7 et coordonnées divisées par deux. Les deux commandes
ont rendu 1 ; `exploration.json` conserve leurs commandes et empreintes. Ces sondes exploratoires ont repris les options déclarées du banc
original : `g++ -std=c++20 -O2 -w`. Ce résultat négatif est conservé.

La gate finale `guard.cpp` emploie `-Wall -Wextra -Wpedantic -Werror`, en O2 puis
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`. Le diagnostic
`-Wmisleading-indentation` est neutralisé uniquement autour des deux inclusions
issues du prototype figé ; il reste actif pour le harnais indépendant. Aucun
`-w` global dans cette gate. `commands.json` contient les quatre commandes,
leurs sorties et codes ; les chemins historiques désignent l’espace isolé
utilisé avant publication. Les options SAN sont `detect_leaks=1:halt_on_error=1`
et `halt_on_error=1:print_stacktrace=1` pour UBSan.

Lecture sans compilation :

```bash
python3 -B verify.py
python3 -B -O verify.py
```

Reproduction des seules deux fixtures finales, dans un répertoire nouveau situé
sous `morsehgp3D_v7/audits/` :

```bash
python3 -B reproduce.py --work ../.meb_boundary_replay_new
```

Le lecteur recalcule aussi le certificat rationnel ci-dessus. La positivité
du support et le confinement donnent l’optimalité par l’identité de variance
pondérée : toute autre boule contenant ces quatre supports a un rayon au moins
aussi grand. Cette vérification ne dépend pas du code C++ du proposeur.

L’archive source provient de `git archive 99b4d3b1 morsehgp3D_v7/src`.
Ses 57 fichiers sont confrontés octet pour octet à ce commit, puis scellés
dans [source_manifest.json](source_manifest.json).
Le prototype figé est conservé octet pour octet. Aucune dépendance système,
ELF, mesure de tour, de GPU ou de performance corrigée n’est distribuée.
