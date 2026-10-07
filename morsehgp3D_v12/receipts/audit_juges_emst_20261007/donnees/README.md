# Données et cache : contre-épreuves des correctifs

7 octobre 2026. Pin `1f7642e105aebd76632c58c63fdfd5b5c0824779`, correctifs
`1b40c0411`. Audit CPU local, `public_status=not_claimed`. Aucun appel réseau/GCP,
aucune donnée réelle, compilation native, matrice ni sanitizer.

**Clôtures proposées : `CST-0216`, `CST-0217`, `CST-0220`, dans les portées ci-dessous.**
**`CST-0218` reste en cours : outil corrigé sur les témoins, 69 découpes réelles à régénérer.**

## Causalité : mêmes petits témoins, ancien puis nouveau code

[check.py](check.py) extrait uniquement six sources historiques dans un dossier temporaire,
depuis `4147c546000b198b5239646063bfb1e3ed6d28fc`. Leurs hashes sont publiés ; aucune
archive d’audit n’est modifiée. Les fichiers u32/JSON sont fabriqués indépendamment des
préparateurs du produit. Les vrais programmes sont exécutés, avec les attentes historiques
et actuelles distinctes.

| Constat | Ancien comportement reproduit | Comportement du pin audité |
| --- | --- | --- |
| `0216`, pilote déplacé | `verify` rend 2 : recherche sous `$ROOT/scripts/verify_inputs.py` | depuis un autre répertoire courant, deux manifestes synthétiques vérifiés, code 0 ; `outils` fonctionne sans ROOT ; arbre sans manifeste et étape inconnue refusés code 2 |
| `0217`, vérification vide | manifeste sans cas et hash nul acceptés code 0 ; témoin au hash faux refusé code 1 | manifeste vide et hash nul refusés code 2 avant lecture ; nominal admis 0, hash faux refusé 1 |
| `0218`, colonne tranchée | quatre sites de mêmes XY, taille visée 2 : seuls deux retenus, rayon déclaré 0 | rayon 0 contenant toute la colonne : aucune découpe partielle publiée, car le carré couvre toute la scène |
| `0220`, éviction de lien dur | vrai objet de six octets lié au build : entrée du cache retirée avant refus ; lien du build conservé | même demande et espace libre simulé nul : refus avant toute éviction, entrée et lien conservés |

L’espace libre est simulé uniquement pour éviter de remplir le disque. Les liens durs sont
réels et les octets du build sont vérifiés. Aucune exécution du contrôleur cloud n’intervient.

## Contrôles supplémentaires indépendants

- **Admission : 14 scénarios**, dont compte booléen/nul, empreinte majuscule, chemin parent,
  liste de découpes mal formée, doublon de nom contradictoire, réemploi cohérent d’un fichier,
  variante distincte sans multiplicité et multiplicité sans hash. Une dernière découpe
  invalide est refusée avant **tout** appel à `Path.is_file` ou au hachage du payload :
  l’ordre de validation est contrôlé par doubles qui lèvent une exception, pas seulement
  par le texte du refus. Variante distincte complète admise, multiplicité altérée refusée ;
  mesure nominale conforme et étendue géométrique fausse détectée.
- **Carré horizontal : sept nuages**, dont coordonnées aux bornes u32 et cinq nuages déterministes
  de 23 sites. L’oracle Python utilise des entiers et trie les distances de Tchebychev ; il ne
  réutilise ni `crop_distances` ni `crop_selection`. **32 sélections** publiées confrontées
  exactement : rayon, nombre réel, tous les PointId et octets des coordonnées translatées.
  Emboîtement vérifié ; demandes trop grandes et carrés couvrant toute la scène sautés.
  Les manifestes obtenus passent aussi `verify_inputs.py --measure`.
- **Cache : trois cas nouveaux** : objet libérable protégé, partiel protégé, et contraintes de
  plancher/plafond simultanées. La fonction d’espace libre simulée dépend de l’existence des
  fichiers, indépendamment des compteurs d’éviction du produit. Les refus préservent le cache ;
  le cas réalisable retire l’objet sans autre lien et préserve l’objet lié.
- **Portes du développeur rejouées** : `bench/donnees_test.py`, 37 cas ;
  `bench/data_cache_test.py`, huit cas. Elles couvrent aussi l’épingle des outils,
  le retrait d’une ancienne entrée de découpe devenue inutile, le premier essai après échéance
  et l’ancêtre symbolique vers les résultats. Toutes passent. Le résultat conserve le hash de
  leurs lignes JSON après retrait du seul champ informatif `optimise`, puis comparaison
  normal/`-O` ; aucun verdict ni compteur de cas n’est supprimé.

## Portée des clôtures et suite

`0216` porte sur le raccord du pilote aux outils versionnés ; aucune chaîne de téléchargement,
préparation LiDAR ou fabrication de paquets réels n’est requalifiée. `0217` ferme les preuves
vides et incomplètes reproduites ; la vérification de hashes ne prouve pas la provenance
physique des données ni leur pertinence. `0220` ferme l’estimation erronée liée aux liens durs
observés avant éviction. Les changements extérieurs d’espace libre et fichiers encore ouverts
restent les limites déclarées par le produit. `freed_disk_bytes` est calculé à partir de la
taille et des liens, sans mesure indépendante des blocs réellement libérés : ce reçu n’en
fait pas un compteur physique certifié.

Pour `0218`, la règle et les petits résultats sont corrigés ; **aucune des 69 découpes
historiques n’a été régénérée ici**. Les quatre manifestes multi-millions, les paquets touchés
et les tables de `DONNEES.md` restent à remplacer après rejeu. L’observation antérieure
sur `prepare_outputs` (premier rang au lieu de l’ID source minimal quand les IDs sont désordonnés)
est distincte ; elle n’est ni corrigée ni requalifiée par cette tranche. Aucun nouveau constat.

## Reproduction et fermeture

Depuis la racine :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_juges_emst_20261007/donnees/check.py
python3 -B -S -O morsehgp3D_v12/receipts/audit_juges_emst_20261007/donnees/check.py
```

Le script principal utilise la bibliothèque standard ; les véritables préparateurs lancés
en sous-processus isolé exigent NumPy. Les sorties agrégées sont identiques octet pour octet,
conservées une seule fois dans [result.json](result.json). Trente sources courantes comparées
au pin et hachées avant/après, six sources historiques épinglées, témoin lui-même haché.
[verification.json](verification.json) ferme la comparaison normal/`-O` et
[SHA256SUMS](SHA256SUMS) les fichiers du reçu. Aucun payload temporaire n’est conservé.
