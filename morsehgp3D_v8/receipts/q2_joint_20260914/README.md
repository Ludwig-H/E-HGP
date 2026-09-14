# Census q2 conjoint — capture avant correction du collecteur

14 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
GCP non utilisé. **Capture historique close, pas l'autorité finale r2.**
Aucun contrat FULL/G4.

## Pourquoi cette capture reste historique

Les [47 CTests Release passent](qualification/release_kuhtgvs4/RESULT.json),
mais la [tentative Clang ASan/UBSan](qualification/sanitize_7dht7d7u/RESULT.json)
échoue : 46/47 CTests passent, `mhgp8_campaign_gate_optimized` échoue
pendant la vérification de collecte à l'interruption. Les empreintes
de fermeture restent stables ; ce n'est pas un changement de source.

Cause reproduite en Python seul : un signal peut lever l'exception
entre `os.read` et l'ajout des octets au tampon de `communicate`.
TERM puis KILL visent correctement le processus possédé et donnent
le bon code final, mais stdout peut être perdu. Ce défaut réel du
collecteur justifie une correction, pas un simple rejeu favorable du test.
Il ne provient pas du nouveau parcours géométrique.

Les 36 mesures à 8k sont closes, toutes réussies, et les
[lecteurs normal/−O](qualification/readers_z0zlkqyb/RESULT.json) donnent
exactement la même sortie. Elles restent rattachées à l'ancien collecteur,
sans promotion vers la révision corrigée. Sources pré-correctif archivées
dans `pre_fix_sources.tgz`, SHA256
`7cd2c9624c915f0bb9cba4383fdf9a7f86380796c34413509e611de6e528a5e7`.
L'archive conserve leurs chemins relatifs depuis la racine ; l'extraire
dans un nouveau répertoire pour consulter les sources sans écraser le
worktree. Les deux builds initiaux sont désormais épinglés.

## Ce qui est comparé

Le [contrat conjoint](../../docs/P0_CENSUS_CONJOINT_Q2.md) conserve A×B
et un préfixe de témoins commun avant le passage à l'ancre individuelle.
Trois modes sur le même front et le même census résiduel : `anchors`
(référence), `joint` (division du facteur le plus large), `joint-a`
(division d'A seulement avant le relais). Compte, curseur, phase et B
original sont transmis ensemble. Les bornes conjointes occupent 96 octets ;
aucun arbre local, tableau global de paires ou liste de frontières ajouté.
Le mode de production par défaut reste Individual/Global/Disabled.

Le temps total est celui de la sonde q2 : génération, propriétaire,
index global, front, comptage, collecte des intérieurs et de toute la
coquille, callback canonique, validations et destructions. Il ne mesure
ni q3/q4, ni catalogue de boules dédupliqué, ni hiérarchies FULL.
Une seule répétition sur hôte partagé, sans warmup ni CPU isolé. La
qualification sanitizer et les audits peuvent coexister avec les
mesures. Ces durées exploratoires ne sont ni p95 ni gains statistiquement
attribués ; les compteurs discrets et sorties sont comparés séparément.

## Provenance et vérification

Builds neufs `build/v8_joint_20260914` (GCC 13.3 Release) et
`build/v8_joint_sanitize_20260914` (Clang 18.1.3 Debug ASan/UBSan,
détection des fuites conservée). Boost 1.83 sert uniquement aux oracles.
Les sources ont été gelées avant la capture et ces builds deviennent
épinglés à sa fermeture. Les commits indépendants d'audit apparus sur
main pendant les tests ne remplacent pas les empreintes des sources.

La gate conjointe passe 5 336 appels sur 18 nuages, avec 1 026 250
contrôles, 124 698 supports complets et 285 376 incidences de coquille.
Oracles scalaires indépendants, K1/2/5/10 et s8/10/12 ; 144 contrôles
du défaut, 4 rejets API, 4 exceptions de callback/reprises, 5 mutants
modèles. Divisions A/B, crédits et 152 relais après crédit sont exercés.
L'admission et la seconde phase conjointes ne le sont pas : la règle
des diagonales les rend inaccessibles avant le relais singleton.
L'[audit A à 7e315009](../../audits/q2_product_20260914/README.md)
prouve cette limite et qualifie un modèle séparé, pas ce binaire C++.

La gate des bornes confronte 3 190 boîtes à des extrema rationnels
indépendants en `cpp_int` : 211 108 valeurs d'oracle, 120 366 points
de grille exhaustive, 48 symétries, 9 contre-modèles arithmétiques,
9 rejets et 3 copies sans alias. Minima stricts, maxima intérieurs,
demi-entiers et extrêmes u16 sont couverts.

Le [pilote capturé](qualification/record.py.snapshot), SHA256
`62bc303b0d0fe7230a0c6d224a4135bf32f557641ff0ee3a63258168beef563e`,
ne construit rien : il vérifie les binaires existants, conserve sorties
brutes et codes attendus, puis contrôle les empreintes de fermeture.
Chaque tentative crée son propre dossier. Les trois opérations sont
`release`, `sanitize` et `readers` ; ce dernier compare aussi Python
normal et `-O`. Pour une autre révision, adapter une copie à des builds
neufs avant de capturer, sans modifier ces témoins.

## Protocole des mesures

`paired_8k` : 36 appels clos, quatre familles × s8/10/12 × trois modes,
Kmax10, seed3, Samples/Shared, Complement/sibling. Le choix de ces deux
dernières options est commun aux trois bras, pas une preuve de leur
optimalité. Aucune croissance 16k/32k n'est lancée sur ce collecteur
avant sa correction. Les autres s ne sont pas mesurés aux grandes tailles.
Le terrain est une nappe synthétique, pas un scan LiDAR.

Les champs `joint_work` et ceux du relais individuel sont de périmètres
distincts. Une borne conjointe teste douze couples de constantes, contre
six à ancre fixe ; leurs nombres ne sont pas des coûts CPU équivalents.
Les tâches conjointes comprennent les relais, eux-mêmes comptés dans
les tâches individuelles : ne pas sommer ces compteurs comme des jobs
disjoints. Rejets/admissions/masses transmises partitionnent le résidu.
Le front, les candidates et le digest canonique doivent rester identiques
entre les trois modes pour une même entrée/K/s.

Résultat exploratoire s8 : amas `anchors` 11,686 s, `joint` 13,146 s,
`joint-a` 11,844 s. Le mode équilibré multiplie les relais à 15 375 375
contre 1 291 731 ancres descriptives. La variante A seul évite cette
explosion mais ne donne pas de gain net ici. La révision r2 effectuera
sa propre qualification et ses mesures ; rien n'est supprimé de cet essai.
