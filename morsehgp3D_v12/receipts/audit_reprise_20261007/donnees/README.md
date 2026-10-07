# Découpes LiDAR : cohérence des tables et de la session E

Contre-lecture au pin `f601b36ac`, correctif documentaire `2b2113264`.
`CST-0218` : l'outil avait déjà été contre-éprouvé sur 32 sélections exactes
([reçu précédent](../../audit_juges_emst_20261007/donnees/README.md)). Ce lot
vérifie les métadonnées publiées ; il ne relit aucune coordonnée LiDAR.

[check.py](check.py) lit les objets Git épinglés, compare les quatre tables de
`DONNEES.md` avant/après le correctif, puis les rapproche du manifeste de transfert
de la session E et des lignes `cloud` de ses onze prises natives.

- Les **69 mêmes découpes** publient désormais séparément taille visée et taille
  réelle. Toutes ont changé d'empreinte ; côtés et bits sont inchangés. Les
  **24 lignes de scènes entières** sont identiques.
- Excédents de sites : IGN **5–317**, ETH3D **109–3 586**, FOR-instance **65–6 287**,
  Boreas **2–308**, conformes au rapport du développeur. Les retours augmentent
  respectivement de 5–317, 109–3 917, 71–6 322 et 2–308.
- Les **neuf découpes distinctes / onze prises MES-E** ont des hashes de coordonnées
  transférées conformes aux tables ; leurs tailles XYZ/PointId correspondent au
  nombre réel de sites. Les onze sorties natives retrouvent ce même nombre.
  La session E porte donc bien sur ces découpes corrigées, et non sur les anciennes
  tailles nominales exactes.

Rejeu normal et `-O` : sortie identique, [result.json](result.json), objets Git
épinglés (documents, reçus, journaux natifs et outils). Aucun
nouveau temps : les mesures restent celles de la v11 gelée, processus complet,
hors contrat résident v12.

**Complément : huit manifestes et journal retrouvés, puis contre-lus.** Le reçu
conserve leurs empreintes et les seules métadonnées nécessaires, sans coordonnées
ni chemins locaux. Les 69 enregistrements de génération du journal correspondent
exactement aux comptes, tailles visées, bits et rayons déclarés. La règle est celle
du carré horizontal fermé, frontière entière ; les hashes et comptes parents
pointent vers les bonnes variantes distinctes. Les 24 scènes et 69 découpes
correspondent aux tables ; les quatre préparateurs et leurs quatorze modules
communs correspondent aux sources Git épinglées.

| Famille | Découpes | Fichiers source vérifiés par `stat` | Payloads du paquet | Octets du paquet |
| --- | ---: | ---: | ---: | ---: |
| IGN | 21 | 93 | 81 | 3 185 391 160 |
| ETH3D | 15 | 65 | 57 | 3 114 613 940 |
| FOR-instance | 16 | 78 | 66 | 1 362 182 340 |
| Boreas | 17 | 85 | 73 | 1 725 451 476 |

Les **277 payloads des paquets** ont chacun le même périphérique et inode que
leur source : ce sont bien des liens physiques vers les fichiers vérifiés par
`stat`, aux tailles attendues. Les listes `SHA256SUMS.txt` reprennent exactement
les hashes des manifestes. Chaque paquet respecte 512 fichiers et 8 Gio ; les
deux petits fichiers de métadonnées s'ajoutent au compte des payloads.
Le journal contient les **huit vérifications à zéro écart**, codes 0, et les
quatre fabrications de paquets réussies. Les neuf découpes transférées en session E
se raccordent aussi aux hashes complets XYZ/IDs de ces manifestes, puis aux onze
prises natives. Les métadonnées lues sont restées inchangées pendant le contrôle.

**Clôture proposée pour `CST-0218`**, dans cette portée : outil contre-éprouvé sur
32 sélections exactes, régénération des 69 découpes tracée et cohérente, tables et
paquets remplacés, neuf découpes corrigées effectivement transférées et utilisées.
**Aucun payload réel n'a été relu ou rehaché ici ; aucun carré n'a été recalculé
géométriquement sur les scènes réelles.** Les zéro écart sont ceux du journal du
développeur, rapprochés indépendamment des manifestes et des liens présents ; cette
clôture ne devient pas une seconde certification géométrique exhaustive des données.

Reproduction depuis la racine, bibliothèque standard seule :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reprise_20261007/donnees/check.py
python3 -B -S -O morsehgp3D_v12/receipts/audit_reprise_20261007/donnees/check.py
```

Ces commandes rejouent la partie Git, sans données locales. Le complément se
rejoue en ajoutant `--external-root DOSSIER_DONNEES`, dossier contenant `data/`,
`bundles/` et `preparer.log` ; il ne lit que JSON, journal et listes d'empreintes,
et fait des `stat` sur les payloads. La capture publiée inclut ce complément.
