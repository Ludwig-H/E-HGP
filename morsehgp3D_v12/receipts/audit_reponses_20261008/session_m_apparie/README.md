# FULL M — contrelecture appariée des journaux retournés

Audit Codex, 8 octobre 2026. Les **140 journaux / 1 816 passes sont admis** au
schéma épinglé et leurs résumés JSON sont reconstruits. Le critère statistique
annoncé adopte le cache hôte de 8 Gio et rejette la voie séquentielle, avec A/A
dans sa fenêtre. Cette admission ne fournit pas l'empreinte ELF de fermeture
absente du pilote exécuté. Aucun moteur, contrôleur ou appel GCP n'a été lancé
par ce reçu ; aucune coordonnée de scène n'a été lue.

## Source, portée et admission

La session a exécuté `957e9784fb19ccd2d6348e779ed8b7affadc1f3d`, pilote
`fe22a681…`, lecteur FULL `c3e9e0f4…`. [capture.json](capture.json) épingle le
plan, les métadonnées, le rapport, l'inventaire des journaux, les trois sources
Git et leurs versions de relecture. L'enveloppe, ses 518 membres et l'arrêt
certifié sont traités séparément dans [session_m_provenance](../session_m_provenance/README.md).
Le binaire d'ouverture apparié déclaré est `c48961e2…` ; le build MES-FULL
est distinct. Le correctif de terminaison `e78904c49` est postérieur au source
exécuté : sa qualification n'est pas transférée à cette session.

Le rejeu applique hors produit la [composition identité/cohorte](../apparie_livraison/composition.patch)
et les [gardes FULL recouvert](../lf_recouvert_gardes/proposition.patch) à ces
sources précises. Il vérifie indépendamment la cohorte exacte, les noms, les
empreintes des bruts, les métadonnées commandées et tous les résumés par prise,
puis recalcule statistiques et ressources. Le lecteur FULL renforcé contrôle
les types, séquences, schémas et dépendances temporelles ; aucune somme de
fenêtres recouvertes n'est imposée. Les empreintes de fichiers sont revérifiées
après lecture. Il s'agit d'une relecture des JSON, pas du tableau Markdown.

| Bloc | Journaux | Passes | Passes retenues à chaud |
|---|---:|---:|---:|
| Identité, 3 trames × 4 bras | 12 | 24 | identité sur les deux passes |
| Décision, 3 trames × 4 bras × 10 tours | 120 | 1 200 | 1 080 |
| Sessions informatives, 4 bras × 2 processus | 8 | 592 | 296 secondes visites |

Les 12 prises d'identité portent chacune une seule empreinte FULL, identique
entre les quatre bras de la même trame. Les prises chronométrées n'émettent
pas cette empreinte. Les huit Sessions informatives ont 37 trames visitées
deux fois dans l'ordre annoncé. Leur rapport n'archive pas un code ni un hash
par processus : le code 0 utilisé pour parser ces huit flux reste conditionnel
au chemin de succès du pilote épinglé et à son résumé complet sans refus.
Les gardes de cohorte renforcées n'ajoutent aucun seuil statistique.

## Résultats appariés K5, GPU, 48 fils

Médiane des dix médianes de processus ; chaque médiane de processus porte sur
les neuf passes après sa première passe. Valeurs affichées en ms, sans arrondi
pour décider :

| Bras | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| Référence recouverte | 94,493974 | 77,872919 | 96,605795 |
| A/A | 94,248574 | 77,626454 | 95,929630 |
| Cache hôte 8 Gio | 87,459999 | 71,968519 | 88,387420 |
| Séquentiel | 143,494173 | 114,184069 | 147,578284 |

La décision utilise les rapports **appariés par tour**, leur moyenne géométrique
et les 10 000 rééchantillonnages annoncés (graine `20261008`, indices de
quantiles 250/9749). Elle n'utilise pas les rapports du tableau ci-dessus.
Les moyennes géométriques A/A sont 1,00384 / 0,99877 / 0,99073, toutes dans
±1,5 %. Pour le cache, elles sont 0,92892 / 0,92440 / 0,91835 ; les bornes
hautes des IC95 sont 0,93844 / 0,92846 / 0,92412, toutes strictement sous 1.
La voie séquentielle donne 1,51536 / 1,46549 / 1,52880 et est rejetée.
Les valeurs complètes et les décisions sont dans [results.json](results.json).
Cette comparaison du cache porte sur u21, GPU, K5 et 48 fils ; elle ne mesure
pas les configurations CPU ou K10 avec cache, ni un nouveau FULL massif.

Lors de la préparation du rejeu, l'égalité flottante bit à bit avec le pilote
réimporté a refusé deux bornes de l'IC de `seq/ng01` : l'accumulation Python
locale produit un écart de **1 ULP**, alors que le calcul indépendant concorde
avec le rapport retourné. Le rapprochement de ces calculs flottants tolère
désormais au plus 2 ULP et archive chaque différence. Ce rapprochement ne
modifie ni les données, ni les égalités bruts/résumés, ni les seuils, ni les
verdicts calculés indépendamment sans arrondi. Les quatre entrées de diagnostic
correspondent aux mêmes deux bornes comparées dans deux directions.

## Sessions de 37 trames et ressources

Pour chaque trame : médiane de ses deux secondes visites, une par processus.
Puis médiane et maximum de ces 37 valeurs. Ce bloc est informatif ; ce n'est
pas la cohorte de décision appariée ni une preuve du contrat 100 ms.

| Bras | Médiane des 37 (ms) | Maximum des 37 (ms) | RSS maximum par processus (Gio) |
|---|---:|---:|---:|
| Référence | 161,654988 | 319,097054 | 2,565 / 2,626 |
| A/A | 161,500372 | 316,635284 | 2,558 / 2,532 |
| Cache 8 Gio | 152,620111 | 305,894729 | 3,809 / 3,807 |
| Séquentiel | 226,405421 | 429,572706 | 1,283 / 1,268 |

Les pics du budget **actif partagé hôte/appareil** valent respectivement
3,079 / 3,079 / 3,137 / 2,887 Gio. Les capacités appareil et épinglée sont
1 986 801 724 et 16 777 216 octets pour chacun de ces processus. Le lecteur
vérifie chacune contre le pic partagé ; `pic_appareil_octets=0` est ici la
convention du budget partagé, pas une absence d'allocation GPU. Les blocs hôte
inactifs du cache ne sont pas inclus dans ce budget actif. Le RSS est un maximum
cumulé du processus hôte, ni VRAM, ni mesure après libération, ni somme physique
CPU+GPU. Les durées CPU et compteurs exacts sont conservés dans le JSON.

L'environnement « après » du pilote précède les huit Sessions informatives.
Aucune isolation supplémentaire n'est donc certifiée par ce champ pendant
ces Sessions. Aucun hash ELF final n'est archivé : `qualification_campagne_complete`
reste faux au sens de cette clôture de provenance. Ce manque ne prouve pas
une substitution du binaire et n'est pas transformé en nouveau veto statistique
après la campagne.

## Rejeu

Les deux modes normal/`-O` rendent exactement [results.json](results.json).
Les arguments sont les métadonnées et journaux locaux retournés, jamais une
archive de coordonnées. `--repo` fournit les blobs Git et les deux patches
immuables ; seules des copies temporaires sont modifiées.

```sh
python check.py --repo /workspaces/E-HGP --folder CHEMIN_APPARIE \
  --metadata CHEMIN_INPUT_METADATA_JSON --plan CHEMIN_PLAN_JSON --check
python -O check.py --repo /workspaces/E-HGP --folder CHEMIN_APPARIE \
  --metadata CHEMIN_INPUT_METADATA_JSON --plan CHEMIN_PLAN_JSON --check
```
