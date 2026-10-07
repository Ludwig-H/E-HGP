# Proposition de garde du pilote CUDA — 7 octobre 2026

`phase=exploration_v12_hors_registre`, `public_status=not_claimed`.
Proposition textuelle hors produit, sur le prototype local `3b445b763` observé
avec main `136e07628`. Aucun fichier du prototype vivant modifié. Elle répond
aux admissions indues reproduites dans `../cuda_juge_prepublication/` ; elle
ne qualifie ni une campagne GPU future ni des mesures historiques.

`proposed.patch` ajoute une validation commune du rapport, conserve les données
natives de **chaque** passe et renforce le test C++ `device_open`. Il s'applique
à trois fichiers existants et crée `bench/g4_catalogue_schema.py`. Épingles
avant/après dans `capture.json` ; aucune copie de source ancienne incluse.

La validation exige les cohortes et identifiants uniques, les types exacts
(entiers non booléens dans le domaine u64), les métadonnées de chaque passe,
les 20 compteurs logiques, 20 diagnostics, 16 champs appareil et neuf durées
élémentaires. Elle vérifie `balls=ledger.emitted`, `incidences=ledger.incidences`,
la correspondance des durées avec leurs sources et leur somme ≤ durée totale.
Les sorties CPU conservent les 16 champs appareil nuls ; des allocations nulles
à chaud restent valides. Les reprises wide/span peuvent se chevaucher.

Les champs `status/reason` d'ouverture et de sortie sont ceux de l'émetteur
(pas de champ `code` inventé). L'ordre des empreintes est celui des passes :
aucun numéro de passe n'est inventé dans les lignes `digest`. Identité CPU :
une passe ; identité appareil : trois. Temps CPU : dix empreintes ; temps
appareil K5/K10 : zéro empreinte conformément aux commandes actuelles. Le
parseur vise ces commandes sans `--out` ; une réutilisation avec export
nécessiterait d'admettre explicitement la phase `export`. Toute ligne non vide
doit être un objet JSON : `null`, tableau et texte libre sont refusés. Les
clés dupliquées, constantes non standard (`NaN`, infinies) et caractères non
ASCII, y compris remplacement UTF-8 ou caractères Unicode échappés, sont
refusés. Une structure diagnostics/device invalide reste conservée pour
validation, sans exception dans `summary`.

L'identité examine toutes les passes physiques et logiques. Les mesures
appareil sont aussi comparées aux comptes et diagnostics CPU du même cas
(k, feuille24). Le manifeste lie chaque mutant à son critère : une preuve de
délai ne remplace plus une divergence d'identité. Une preuve absente ou
incohérente donne `refuse` ; une preuve complète d'échec de condition donne
`rejete`. Un arrêt précoce du pilote reste refusable sans exception Python.

Deux précisions de règle doivent être déclarées **avant** la campagne :

- l'isolation GPU est exigée avant **et après** les temps ; l'ancien texte
  n'exigeait que l'avant, donc l'ajout n'est pas une invalidation rétroactive ;
- la cohorte K5 est celle configurée et figée d'avance, avec au moins 5
  processus et 10 passes : Nproc×(P−1) valeurs, soit 45 pour le plan5×10.
  Le juge accepte aussi6×11 (60 valeurs). K10 reste3×5. La médiane globale
  et le maximum des médianes par processus doivent chacun être ≤45000000ns,
  avant arrondi. Le plan, les options et les commandes archivées doivent
  être concordants ; ce juge de rapport n'authentifie pas un environnement
  ou des journaux externes.

Le test `device_open` proposé passe un diagnostic à chacun des deux appels
résidents et les confronte au CPU. Le marqueur positif complet reste requis :
le succès du test de refus « appareil indisponible » ne qualifie pas CUDA.
Cette modification C++ est seulement appliquée textuellement ici ; elle reste
à compiler et à jouer sur appareil avant toute adoption.

## Rejeu et portée

```sh
python3 -B -S check.py --source-dir RACINE_V12_EPINGLEE
python3 -O -B -S check.py --source-dir RACINE_V12_EPINGLEE
```

Le lecteur contrôle les sources avant/après, reconstruit un répertoire
temporaire, applique le patch et vérifie ses empreintes. **18 auto-injections
et 31 cas complémentaires** passent avec sorties normal/−O identiques.
Les témoins incluent étapes absentes, processus dupliqués, identité effacée
des deux côtés, compteurs perdus dans une première passe ou une mesure,
durées négatives/booléennes, métadonnées fausses et mutants reclassés.
Les dix compléments de contrelecture root altèrent uniquement l'archive CPU
intacte : huit corruptions textuelles et deux structures diagnostics/device
invalides, toutes refusées sans exception.
Une première version du témoin `summary` conservait un alias Python de son
attendu : il a été corrigé par une copie profonde ; aucun changement produit
n'a été fait pour cet échec du témoin.

`cpu_format.jsonl` est une vraie sortie fournie par le contrôle indépendant
root : deux sites synthétiques, un fil CPU, trois passes, code0, binaire
`b845fbae…` haché avant/après. Elle passe `parse_probe → summary → valid_run`
sans modifier ses lignes. La provenance de construction du binaire n'est pas
contre-certifiée. `capture.json` porte commande, hashes et périmètre exact ;
ce contrôle de format n'est pas un benchmark. L'adaptation en lignes appareil
dans le lecteur est explicitement **synthétique**. La contrelecture du schéma
C++ par un autre auditeur n'a pas relevé d'incompatibilité avec les commandes
saines prévues ; aucune compatibilité native GPU n'en découle.

Aucun moteur n'est exécuté par `check.py` ; aucune compilation, campagne LiDAR,
exécution GPU ou GCP dans ce reçu. Avant mesure : intégrer le patch et le
nouveau module, requalifier les portes Python et C++, jouer `device_open` sur
l'appareil réel puis archiver la cohorte configurée et les preuves complètes.
