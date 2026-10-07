# Proposition ciblée : admission et rejeu des journaux T2-c

7 octobre 2026. Suite corrective du [contre-exemple publié](../t2c_pilote_admission/README.md),
sans modification du prototype ni du produit. [proposition.patch](proposition.patch)
s'applique au pilote non publié SHA-256
`323068ab824e5dcb8920de9ae3429e62c980e7d2608df62cccf5dd99d8c9c0e1`.
Ce pin diffère du précédent `fa1b7…` seulement par la publication de chiffres
sous quota dès deux tours (le refus demeure) et l'alignement du tableau.
Le [delta de préimage](prototype_delta.patch) conserve cette évolution ;
aucune duplication de la snapshot historique.

Le patch impose le schéma exact de chaque bras : `avant`/`avant_bis`
gardent les sept diagnostics scalaires historiques et `order_ns` ; les
autres bras prennent les treize scalaires et quatre tableaux de repo2.
Les profils supplémentaires sont déclarés uniquement pour le bras `profil`,
après chacune des passes. Il contrôle types et bornes, configuration
K/W/u21, indices 0..P−1, ordre des phases, compteurs, digest complet et
`exit` final (`order` entier nul). Les booléens, clés JSON répétées,
NaN/Infinity, texte annexe et UTF-8 invalide sont refusés. Les valeurs
`wall_ns` doivent être strictement positives pour permettre les rapports.
Un succès avec stderr annexé est également refusé : le journal admis est
entièrement JSONL. Les octets non UTF-8 restent conservés lors de la capture.

`juger()` revalide les 120 journaux d'une campagne minimale de huit tours,
vérifie leurs hashes avant et après lecture, recalcule murs et médiane
chaude, puis compare les résumés à ces valeurs. La cohorte doit comprendre
les trois trames et cinq bras, le nombre demandé de tours exactement,
des journaux distincts et un nombre de sites constant par trame. Toute
divergence est un **refus explicite** ; les résumés recalculés alimentent
ensuite les mêmes statistiques. Le seuil, le bootstrap, les leviers,
l'A/A et les minima de tours/passes de `REGLE_T2C` sont inchangés.

Les références ng00 et uniformes sont corrigées depuis le CMake actif
épinglé dans [capture.json](capture.json), et non depuis une chaîne
supposée : `e5a81154fb1b15f1`, puis
`a40f1b2ef8547269/cf7c7745fcb4ae6e/d1f08fd0dbdf48eb`.
Les deux producteurs C++ et leur exportateur sont également hachés.

## Validation bornée

[check.py](check.py) applique les deux patches dans un répertoire temporaire,
vérifie les préimages et le hash de la proposition, puis exerce :

- 21 prises Python : trois formes positives (avant, après, profil) et
  18 refus, dont les cinq mutations admises par le pilote initial ;
- trois jugements : campagne cohérente sans gain rejetée, résumé `g_ns`
  divergent refusé, journal invalide mais réhaché refusé ;
- les cinq auto-tests statistiques existants, toujours passants ;
- les deux sorties **réelles inchangées** [avant](before.jsonl) et
  [après](after.jsonl) fournies par l'auditeur principal : huit sites
  synthétiques, K5/W1/P2, code 0, stderr vide, deux exécutables dont les
  hashes étaient stables avant/après. Toutes deux sont admises ;
- deux variantes synthétiques de forme K5/sites2, avec deux ordres et
  tableaux de diagnostics de longueur K. Elles vérifient la distinction
  entre K demandé et nombre d'ordres produit.

Les microcaptures natives ont été exécutées **par l'auditeur principal**,
avant intégration ici, uniquement pour le format. Ce rejeu n'exécute aucun
moteur natif. Leurs métadonnées conservées ne certifient ni la provenance
du build ni la géométrie, et leurs durées ne sont pas des benchmarks.
La contrelecture indépendante des deux schémas et de leur admission est
favorable. Normal et `-O` donnent les mêmes [résultats](results.json).

## Limites conservées

C'est une validation **structurelle et de concordance**, pas un oracle
géométrique. Le producteur émet ordres et digest après la dernière passe
seulement : aucune identité par passe n'est prouvée. Les compteurs de
travail peuvent différer entre bras ; ils ne sont pas comparés entre
algorithmes. Les bilans internes des diagnostics et des compteurs restent
hors de cette correction ciblée. Le journal ne porte pas leaf/frame/path ;
le patch n'invente pas ces métadonnées ni une preuve de provenance du
processus à partir de son seul stdout.

Le mode `rapport` seul n'exécute toujours pas `etape_auto_test()` ; cette
garde annoncée par le pilote constitue un point séparé, **hors patch**.
La voie `verifier_journaux=False` reste réservée aux auto-tests synthétiques
et n'est pas appelée par l'action CLI `rapport`. Les règles statistiques,
l'ordre des bras et la mesure catalogue + G font l'objet d'une revue
distincte. Aucun gain ni qualification G4/FULL n'est acquis ici.

## Relecture et proposition

Depuis ce dossier :

```sh
python3 -S check.py --check
python3 -S -O check.py --check
sha256sum -c SHA256SUMS
```

Pour examen dans un checkout contenant exactement le pilote `323068ab…` :
`git apply --check <chemin>/proposition.patch` depuis la racine du dépôt.
Le développeur décide de l'intégration ; aucune application vivante n'a
été effectuée. Toute évolution de schéma ou de préimage impose une nouvelle
contrelecture, sans transfert automatique de cette validation.
