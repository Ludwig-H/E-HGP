# Complément ciblé : correctifs de la contre-lecture T2

Pin propre à ce sous-reçu : **`76adb8fa9eb10894b6a363672e7f28cc3f12e75a`**,
postérieur au pin `1f7642e10` du lot principal. 7 octobre 2026, CPU local,
`public_status=not_claimed`. Aucun natif, donnée réelle, GPU/GCP ou suite de 286 portes.
La nouvelle branche G1 de `151d4` n’entre pas dans ce complément.

**Clôtures proposées : `CST-0227` au lecteur ; `CST-0228` et `CST-0229` pour leurs énoncés
avant code.** Aucune qualification de la tour native ne découle de ces clôtures.

## Identité de trame (`0227`)

[check.py](check.py) réutilise uniquement le constructeur indépendant de petits catalogues
de [l’ancien audit](../../audit_t2_20261007/transition/README.md), sans lancer sa campagne.
Les deux payloads `audit+FF` / `audit+FE` sont **identiques octet pour octet** à ceux de ce
reçu : 728 octets chacun, neuf boules ; les deux hashes historiques sont exigés.
Ils rendent maintenant **refus 2**, au lieu du code 0 historique.

Huit appels au vrai CLI : trois paires ASCII valides admises (nom ordinaire, bornes imprimables
espace/tilde, nom occupant les 24 octets), puis cinq refus : noms ASCII distincts, FF/FE,
deux noms identiques contenant FF, contrôle `0x1F`, DEL `0x7F`. Le contrôle protège ainsi
l’admission elle-même ; il ne se limite pas à différencier les deux entrées.

## Contrat T2 et deux faits bornés

Le diff de `76adb8fa9` a été relu dans `CONTRAT_TOUR.md`, la table des lemmes et le registre
mathématique racine. Les changements répondent aux contre-exemples :

- **`0229`** : le lemme est borné à des sites distincts, **2 ≤ k ≤ K**. L’ancienne lecture
  incluant k=1 est explicitement classée `false_in_general`. Le singleton de rayon nul ne
  peut plus servir à appliquer la borne d’un catalogue de boules positives.
- Le census saturé affirme seulement **p ≥ k**, sans en déduire l’absence du catalogue.
  La décroissance stricte et la mise à jour de la boule précédente précèdent désormais
  le saut, y compris la sortie anticipée après saturation.
- Les candidats de G-L3 passent d’abord la garde entière `NUM-GARDE`, puis le prédicat
  mixte. Cette justification exige toujours la boule certifiée du contrat numérique.
- La cible de quatre octets distingue genre et indice : bit 31 pour le genre, 31 bits
  d’indice, au plus 2^31−1 naissances **et** cellules par ordre, refus avant allocation,
  `0xFFFFFFFF` réservé. Il s’agit d’un contrat de représentation à implémenter et juger.
- **`0228`** : les compteurs d’objet entrent dans l’empreinte ; ceux du travail en sortent
  et sont attachés à une politique et à un ordre canonique fixés. La concordance W1/W48
  et hôte/appareil reste une porte exigée ; les diagnostics physiques restent séparés.
  Les profondeurs et sites examinés ne sont plus déclarés invariants sous tout ordre de visite.

Seules les fonctions `fact_lemma_needs_two_sites` et `fact_saturated_in_catalogue` du
véritable `test_resolution_v12.py` sont appelées : trois sites alignés/K3 pour le singleton,
puis carré avec deux intérieurs/K5 pour la diagonale différente du support canonique.
Les deux rendent une liste d’écarts vide. La suite de résolution et ses autres politiques
ne sont pas rejouées. Mémo, historique d’attache, coquilles, capacités et empreintes FULL
natives restent à qualifier séparément.

## Rejeu et fermeture

`python3 -B -S check.py`, puis `python3 -B -S -O check.py` depuis ce dossier.
Résultats identiques octet pour octet, conservés une fois dans [result.json](result.json).
Les sources et documents lus sont comparés au pin et hachés avant/après ; le témoin est
également haché. [verification.json](verification.json) ferme les deux exécutions et
[SHA256SUMS](SHA256SUMS) les fichiers. Aucun vidage temporaire n’est conservé.

Un premier essai du harnais d’audit attendait à tort un stderr vide pour le cas conforme :
le CLI publie son temps sur stderr. Cette attente du harnais a été corrigée ; ce n’était
pas un échec du lecteur. Les deux captures finales proviennent du témoin corrigé.
