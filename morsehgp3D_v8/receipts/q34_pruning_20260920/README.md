# Tranche25 : témoins universels partagés avant ouverture des familles

20 septembre2026, après77f659e4, hors registre, `public_status=not_claimed`.
Les158 empreintes de sources identifient le produit mesuré, indépendamment
du commit de départ. L'auditeur a publié4215dd16 pendant cette tranche ;
ses fichiers ne font pas partie du changement constructeur. GCP non utilisé.

## Captures propres

| Capture | Périmètre |
| --- | --- |
| `smoke_n8zdc_in` | Release : quatre portes et18 sondes32, K5/10, pool0/32/64 |
| `smoke_w30cix1c` | Clang ASan/UBSan : mêmes portes et18 sondes |
| `scale_fgs6uc9n` | Release sur CPU0 : quatre portes et60 mesures appariées |
| `regression_mpu74bvw` | 86/86 CTests Release PASS,130s |
| `readers_twjdfdqx` | Dix lectures normal/−O concordantes,28 mutants lecteur |
| [mutants](mutants/README.md) | Trois mutations compilées détectées |
| [différentiel](mutants/README.md#compatibilité-du-chemin-par-défaut) | 20 paires contre binaire tranche24 épinglé |
| [prévision dense](dense_forecast/README.md) | Préfixe seul ; coût aval obligatoire non exécuté |

Total :96 mesures complètes d'une **arête fournie**, dont36 mesures
8k/16k/32k. Les12 configurations denses auxiliaires ne s'y ajoutent pas
comme mesures du parcours complet. Pas de WSPD globale, de tour FULL ni
de mesure G4. Le paramètre s8/10/12 n'existe pas à ce niveau.

La nouvelle porte effectue5 176 contrôles :178 appels de primitive,
63 appels par arête et129 par seed face à192 références,985 complétions
et2 201 visites de sites d'oracle,106 candidats (71q3/35q4), coquille30.
Elle vérifie notamment les deux rejets indépendants, tangence stricte,
arrondi extérieur, u16 extrêmes, permutations, absence de crédit au
repli, exceptions et quatre appels concurrents au même pool immuable.
Les28 mutations du lecteur sont distinctes des trois mutants compilés.

Builds `build/v8_q34_pruning_20260920` et
`build/v8_q34_pruning_sanitize_20260920` désormais épinglés. Le second
couvre quatre portes/18 sondes, **pas86 CTests sous sanitizers**.
Le [préflight](PREFLIGHT.md) conserve les essais antérieurs au gel.

## Ce que paie la mesure

Chaque configuration prépare le nuage, l'index et le cover une seule fois,
puis exécute successivement référence et variante, avec collecte réelle
des supports/coquilles et comparaison physique des sorties. Le pool est
préparé une fois pour toute l'arête, jamais par face.

`baseline_prepare_run_sum_ms` et `pruned_prepare_run_sum_ms` sont des
**sommes de postes**, pas deux temps mur indépendants. Chaque somme inclut
la même génération et les mêmes préparations réellement mesurées ; celle
de la variante ajoute son pool. Elles incluent leurs callbacks, mais pas
les validations indépendantes ni le tri final de comparaison. Ces derniers,
ainsi que la libération, sont inclus dans `paired_total_ms`, le vrai temps
mur de l'expérience appariée. La première sortie reste en mémoire pendant
la seconde ; les capacités publiées ne sont pas le RSS.

Le filtre compte séparément préparation du pool, certificats, dichotomies
entières, propositions, crédits, masques, scans restants, événements, tris,
sorties et capacités. Le lecteur publie la croissance de tous ces postes,
y compris les ratios au-dessus de quatre. Les propositions ne sont pas
gratuites et les crédits ne sont jamais ajoutés au census de repli.

## Petit régime adverse : réduction réelle, pas disparition du résidu

Même arête, m=n sites et S=n−2 faces propriétaires. Lectures du cover :

| n | Sans filtre | K5/C32 | K5/C64 | K10/C32 | K10/C64 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 32 | 960 | 960 | 960 | 960 | 960 |
| 64 | 3 968 | 2 944 | 2 304 | 3 968 | 3 968 |
| 128 | 16 128 | 7 296 | 6 400 | 14 208 | 11 264 |
| 256 | 65 024 | 25 344 | 15 616 | 49 664 | 33 536 |

À n256/C64, K5 ajoute8 974 tests de pool et5 588 itérations de racine ;
K10 ajoute12 270 tests et le même nombre d'itérations. Les comparaisons
de tri passent de620 216 à147 678 (K5) ou316 486 (K10). Les sorties
restent **14 à K5 et54 à K10**, identiques en profondeur et coquille.

Sommes préparation+calcul à n256/C64 :5,293ms à K5 et10,961ms à K10,
soit respectivement×0,243 et×0,488 de leurs références appariées. Une
seule observation par configuration, avec qualification concurrente :
ce ne sont pas des gains stables ni des latences de tour. À n32, aucun
rejet q4, même quand le pool contient tous les sites ; du travail est ajouté.

À K10/C64, les lectures font×4,133 /×2,839 /×2,977 aux doublements ;
les tris×11,152 /×3,248 /×3,427. K5/C64 améliore les lectures à
×2,400 /×2,778 /×2,440, mais le premier ratio des tris dépasse encore six.
Il serait donc faux de ne publier que les ratios favorables.

## Grands fonds : pas de gain de filtrage

À8k/16k/32k, les deux fonds conservent seulement deux faces. Le fond
lointain laisse m=6, soit12 lectures ; le fond dans le cover laisse m=n,
soit16 000/32 000/64 000 lectures. **Aucune famille q4 n'est retirée**
par le nouveau filtre sur ces configurations, à K5 comme à K10.
Les tests ajoutés sont12 par arête dans le premier régime,64 ou128
dans le second. Aucune amélioration générale du temps n'en découle.

Ces séries vérifient la préparation partagée et la croissance du travail
pour S constant ; elles ne testent pas le régime S proportionnel à n.
La [sonde auxiliaire dense](dense_forecast/README.md) traite précisément
ce défaut de couverture, sans lancer aveuglément un milliard de lectures.
Elle exécute les certificats et compte les familles survivantes, puis
calcule m·S4, minorant des lectures obligatoires du futur repli q4. **Ce
n'est ni un chronométrage du repli ni une validation de ses sorties.**

La capture `dense_forecast/forecast_o1_hie63` vérifie12 configurations.
À C64/K10, elle laisse2 035/4 647/14 862 familles q4 aux trois tailles,
soit au moins16,280M/74,352M/475,584M lectures futures. Les rapports
×4,567 et×6,396 dépassent le quadruplement. À C32/K10, le minorant
vaut33,560M/159,040M/730,080M. C'est une alerte concrète contre la
généralisation de ce repli, pas une preuve asymptotique tirée de trois
tailles. Le nuage grandit par préfixes d'une grille3D finie et change
de géométrie avec ses couches ; les choix du pool évoluent aussi.

## Conclusion et reproduction

Le filtre est exact et peut retirer un travail conséquent avant les tris.
Il ne garantit pas un résidu sous-quadratique. La prochaine comparaison
porte sur le minimum collectif du même pool et la corde resserrée proposés
par l'auditeur, avec préparation/tri/résidu payés ; pas de transfert de
ses mesures utilisant un autre pool. Si cela ne suffit pas, partager
des décisions sur des blocs de centres/faces, pas seulement le pool.

```sh
python -B morsehgp3D_v8/bench/run_q34_pruning_checks.py read morsehgp3D_v8/receipts/q34_pruning_20260920/scale_fgs6uc9n --check-live
python -B -O morsehgp3D_v8/bench/run_q34_pruning_checks.py selftest morsehgp3D_v8/receipts/q34_pruning_20260920/scale_fgs6uc9n
```

Le lecteur contrôle commande/reçu, sorties brutes, identités de travail,
empreintes et clôture. Les lectures principales ferment158 sources,
62 artefacts et119 entrées de vérification. La couverture globale WSPD, le catalogue de boules,
les intérieurs après regroupement, FULL, GPU et les contrats restent ouverts.
