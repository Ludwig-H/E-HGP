# Lecteur des captures QUAL / PAIRED

`verify_full_captures.py` est autonome (bibliothèque standard Python), sans
import du moteur, lancement de processus, compilation ni appel cloud. Il lit
les archives en place et vérifie tous les payloads de `MANIFEST.sha256`, y
compris les inventaires imbriqués. Seul le manifeste racine est exclu de
l'inventaire qu'il définit. Aucun fichier de capture n'est écrit ou extrait.

Le commit attendu par défaut est `6503c95abeb7822a5efd61e25babc2c0b60b79cc`.
Le lecteur sera publié après les campagnes ; il ne change pas leur source.
Les deux sessions définitives doivent avoir le même paquet Git, mais leurs
générations, builds, binaires et plans sont distincts. La baseline comparative
reste `895680ff866fbe41c450c87b2498ebff2ac7408b`, mode2047 exclusivement.

Chaque répertoire de session passé au lecteur contient les copies closes :

```text
receipt.json
target_running_minimal.json
package/plan.json
package/plan.sh
results/results.tar.gz
```

Conserver deux paquets sources partagés nécessaires, par exemple
`captures/sources/672afb71c/package.tar.gz` (prise échouée initiale) et
`captures/sources/6503c95ab/package.tar.gz` (commun aux sessions définitives) ;
passer le paquet correspondant par `--source-package`. Le snapshot minimal matériel vient du describe
effectué par le contrôleur gardé : machineType G4-48, SPOT/STOP/maxRun4200,
cible et génération, plus SHA/origine du relevé. Il ne contient pas de données
OSLogin. `receipt.json` certifie l'arrêt TERMINATED de cette même génération.
Une clôture externe ou inconnue échoue au lecteur.

```bash
python3 verify_full_captures.py --qualification captures/qualification \
  --source-package captures/sources/6503c95ab/package.tar.gz --out captures/review_qualification.json
python3 verify_full_captures.py --qualification captures/qualification \
  --paired captures/paired --source-package captures/sources/6503c95ab/package.tar.gz \
  --out captures/review_paired.json
```

Rejouer aussi avec `python3 -O` et conserver les sorties/codes. Le code0
signifie que les conditions de protocole recoupées sont conformes ; toute
rupture ou qualification incomplète donne code1. Les contrôles synthétiques
du lecteur sont dans `tests/support/full_capture_reader_test.py` : archives
minuscules, fixtures JSON, contre-exemples d'intégrité et de verdict ; aucun
produit natif exécuté.

La première prise auditfix1, source672, est un échec préservé : un mutant
catalogue est INVALIDE pour compilation, ce qui ne qualifie pas la suite
complète. Le lecteur peut l'inspecter avec `--inspect-failed` et
`--expected-commit 672afb71c12ec4d323f5cd7e7457a0c04fdf0999`, en passant
**son paquet672 original**. Il retourne toujours code1/conforming=false,
même si l'intégrité, les autres portes et l'arrêt sont corrects. Son paquet
ne peut pas être remplacé par celui de650. Sa copie portable est conservée
distinctement dans `sources/672afb71c/`. L'option n'autorise
pas une prise interrompue ou partielle à devenir une qualification.

Le rapport recoupe les configurations réellement présentes, les inventaires
CTest/JUnit, les profils/options dans les caches capturés et les causes des
mutants. Clang absent reste absent. Les gardes tri/FENV/pannes catalogue
doivent passer dans les configurations complètes et le rebuild ciblé ; le
supplément ASan18 ne joue que num/index/tower et ses gardes correspondantes.
Les compteurs de mutants viennent des manifestes du paquet et des blocs de
leur propre CTest, jamais d'un total historique.

Pour PAIRED, le lecteur exige les81 invocations ordonnées : trois trames
entières sans sol, K5/u21, W1/W8/W48, trois prises, baseline2047/courant2047/
courant16379 avec rotation des producteurs. Il recoupe pour chaque invocation
l'entrée XYZ/IDs, l'argv, les pins avant/après, le nouveau binaire ciblé, les
traces, les hashes bruts/sémantiques, les comparaisons avant suppression et
l'origine de réemploi des résumés. Les six hashes XYZ/IDs sont aussi ancrés
dans le lecteur sur reuse1 ; un autre jeu de mêmes cardinalités est refusé.
Les temps FULL sont ceux du natif ; ni
CPU cumulé ni intervalles des ordres concurrents ne sont ajoutés au mur.

Les limites restent explicites : CPU sur hôteG4, pas GPU ni tête/points ;
trois trames d'une même séquence ; masses unitaires et coordonnées1mm ;
processus/propriétaires neufs, caches OS non vidés ; préparation sol/grille,
IO/Cloud/Pool, projection/tête et décodage Python hors chrono FULL. Les
hashes de binaires absents des archives restent des relevés du worker.
Les dumps réussis supprimés ne peuvent pas être redécodés hors ligne ; le
lecteur recoupe les comparaisons et hashes enregistrés. Pour la baseline,
le cache a un hash ; ses options sont recoupées via les commandes capturées
et les gardes du producteur, sans prétendre disposer du texte de son cache.
Le commentaire Git du paquet enregistre le commit déclaré ; son SHA protège
les octets exacts. Un manifeste de blobs Git indépendant peut compléter
cette provenance, sans être reconstruit fictivement par le lecteur.

## Texte à remplir après clôture

> Source exécutée :650… ; publication du lecteur :[commit]. Qualification :
> [état], configurations [inventaire, profils, portes sélectionnées/passées],
> ASan18 [comptes, portée], mutants [juges / compilations attendues / invalides],
> Clang [présent/absent]. Prise auditfix1 source672 conservée en échec.
> PAIRED :[état,81 ou omissions exactes], sorties [statut], timings [table des
> prises/médianes, phases/CPU/réservations distincts]. Les binaires PAIRED sont
> reconstruits et soumis aux portes ciblées ; aucune identité avec les
> binaires de qualification n'est revendiquée. Arrêts :[cibles/générations].
> Lectures Python normal/−O :[commandes, codes, hashes]. Limites :[ci-dessus].

Après toute dérivation, fermer une enveloppe exhaustive SHA256/ledger qui
inclut chaque inventaire imbriqué, script et rapport. Exclure uniquement son
propre `SHA256SUMS` racine, pas tous les fichiers de ce basename. N'altérer
aucune pièce close. L'exclusion export-ignore des futurs paquets sources
bruts peut être ajoutée **après les deux sessions**, pour éviter leur copie
récursive dans une campagne ultérieure ; elle ne réécrit pas les paquets
déjà exécutés. Ligne ciblée dans `morsehgp3D_v11/.gitattributes` :

```text
receipts/qualification_performance_20261003/captures/sources/*/package.tar.gz export-ignore
```

La petite archive comparative `baseline_source_895680ff8.tar.gz` doit rester
incluse dans les futurs paquets qui construisent ce comparateur.
