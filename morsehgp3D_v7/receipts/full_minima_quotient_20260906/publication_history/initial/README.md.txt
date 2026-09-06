# Minima Gabriel : preuves et modèles rationnels autonomes

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Aucune qualification moteur C++, CPU industriel,
GPU, GCP ou contrat de latence n'est revendiquée par ce paquet.

Trois preuves sont conservées avec leurs sources exactes et captures historiques :

- **Quotient sur minima.** Les deux restrictions naïves du graphe aux minima
  échouent sur quatre points u16 réguliers. Le graphe minimax transférant les
  chemins, ou sa forêt couvrante avec naissances et lots atomiques, conserve
  H0 filtré, les identités et couvertures de points : 17 modèles, 640 coupes,
  68 histoires, dix mutants. Ce n'est pas une énumération produit de Γ.
- **Descentes.** Une descente de facettes de cardinal K est correcte, mais son
  coût n'est pas monotone face au raccourci J=1 actuel. E5 donne 2→1 MEB,
  une autre fixture régulière n5 donne 1→2. Le gate conserve 17 modèles × trois
  capacités et cette contre-fixture supplémentaire. Il compte des calendriers
  conditionnels P=0 et ordinaux F, jamais des temps. Aucun cache hit dans ce
  corpus ne permet de revendiquer une économie de cache.
- **Borne de sortie.** Une famille rationnelle 3D porte au moins N²/4 minima
  de cardinal deux après une perturbation régulière d'existence. Les trois
  modèles finis m=2/5/10 vérifient 129 paires et 2 008 identités, avec huit
  mutants. La perturbation n'est pas exécutée. Cela borne l'émission FULL
  explicite, pas tout encodage implicite ni une asymptotique infinie u16.

Les explications détaillées sont archivées sous [payload/proofs](payload/proofs).
Ces copies conservent leurs références contextuelles au dépôt original ; leurs
liens historiques ne sont pas des dépendances exécutables de ce paquet.
Les sources rejouables se trouvent sous `payload/sources/morsehgp3D_v7/tests/`.
Cette arborescence conserve sans modification l'import explicite du petit oracle
quotient par le test de comparaison. Aucun module du produit n'est importé.

## Vérification et rejeu après déplacement du paquet

Depuis la racine de ce paquet, avec Python 3.10 ou ultérieur :

```bash
sha256sum -c SHA256SUMS
python3 -B payload/replay.py
python3 -B payload/replay.py --execute --output ../replay_math_neuf
python3 -B -O payload/replay.py --execute --output ../replay_math_optimise_neuf
```

Sans `--execute`, le lecteur vérifie les fichiers et reçus sans lancer de test.
Chaque rejeu exécute uniquement les trois gates Python, chacun avec `--selftest`
et `--unknown` en mode normal puis `-O` : douze codes attendus 0/2, sorties
exactement comparées à celles de leur mode historique. Le répertoire de sortie
doit être neuf et extérieur à `payload/`. Aucun chemin `/workspaces/E-HGP`,
aucun interpréteur historique et aucune affinité CPU particulière ne sont requis.
Le flag `python_optimization` du modèle de borne reste 0 ou 1 : ses deux stdout
historiques ne sont donc pas réétiquetés comme byte-identiques entre modes.

`payload/history/` conserve byte pour byte les trois captures closes, leurs
stdout/stderr, commandes, reçus, sources et scripts de capture lorsqu'ils en
faisaient partie. `payload/provenance/` ajoute le driver historique de borne et
ses contrôleurs importés, **pour provenance seulement : ne pas les exécuter**.
Leur ancien snapshot contient des pins C++ sans prétendre redistribuer ni
requalifier tout ce moteur. Les quelques headers de la capture de descentes
sont également des témoins lus, jamais compilés dans ce paquet.

`qualification/` conserve un unique nouveau rejeu fermé depuis une copie
déplacée hors du dépôt, avec ses douze commandes brutes normales et optimisées.
Ce rejeu qualifie la portabilité des modèles, pas une
nouvelle expérience moteur. Les chemins absolus dans les reçus sont uniquement
la provenance datée de leurs commandes, jamais réécrite ou imposée au rejeu.

Le manifeste racine `SHA256SUMS` utilise exclusivement des chemins relatifs à
cette racine. Les manifestes historiques imbriqués gardent leurs bases originales.
Le manifeste `payload/PAYLOAD_SHA256SUMS` ferme le contenu portable, inchangé
entre le rejeu et la publication. Aucun fichier de l'auditeur n'est copié.
