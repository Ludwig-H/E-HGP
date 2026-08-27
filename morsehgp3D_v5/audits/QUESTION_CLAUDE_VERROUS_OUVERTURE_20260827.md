# Questions à l'auditeur — verrous d'ouverture de la v5 (27 août 2026)

Contexte **historique au pin de la question** : `morsehgp3D_v5` reproduisait
l'objet v4 bit à bit sur les campagnes différentielles alors disponibles
(digests canoniques identiques) et le reconstruisait sur une base de code
neuve. Le statut frais et la portée des reçus sont désormais donnés uniquement
par `ETAT_COURANT.md`. Quatre verrous — deux mathématiques, deux
d'implémentation — demandaient alors un arbitrage avant consolidation.

## V1 — Positions dupliquées : refus ou HGP pondéré ?

`MATHEMATIQUES.md` (v4 § 2.0, repris) refuse les positions dupliquées
(`unsupported_degeneracy`) « tant qu'un HGP pondéré n'est pas défini et
prouvé », alors que l'index les bucketise avec multiplicité (les comptes de
témoins comptent déjà les multiplicités). Sur LiDAR quantifié u16 les doublons
sont **fréquents**. Question : (a) le refus reste-t-il la sémantique
normative ; (b) sinon, quelle définition de $\rho(\sigma)$ et de la profondeur
$\vert I_B \vert$ pour un multi-ensemble (un point de multiplicité $m$ compte-t-il
$m$ intérieurs ? est-il son propre support d'arité 2 avec $D = 0$ ?) — et quel
théorème (Th. 2 ou 4 du manuscrit) survit à cette extension ?

## V2 — Plateaux sphériques : borne de coquille et compression

L'expansion d'un plateau énumère les $T \subseteq U_B$ ($2^{\vert U_B \vert}$),
avec plafond `shell_cap = 12` puis `resource_exhausted`. La v4 avait un UB
pour $\vert U_B \vert = 32$ ; la v5 refuse. Sur `uniform n=400` on compte déjà
345 806 points de coquille pour 104 802 événements (grille fine : les
cosphéricités sont la règle). Question : la compression par **supports
minimaux** (MATHEMATIQUES § 5.3bis : « compression par supports minimaux
ensuite ») a-t-elle un énoncé prouvable donnant, pour chaque $K$, l'ensemble
des $\sigma = I_B \cup T$ **sans** énumérer tous les $T$ — par exemple
« les $T$ admissibles sont exactement les sur-ensembles de supports minimaux
dans $U_B$, et les rôles actifs se lisent sur le support minimal contenu » ?
Si oui, quelle borne sur $\vert U_B \vert$ rend le régime exact plutôt que
refusé ?

## V3 — Contrat de sortie à 30 M de points

L'audit v4 du 22 août montre que le flux symbolique complet est impossible
(158,4 Go de `PointId` de facettes nées pour 30 M). La v5 streame par $K$ et
laisse l'appelant choisir ce qu'il garde (`on_forest`). Question : quel
**objet minimal** faut-il rendre pour que « hiérarchie HGP calculée » soit un
énoncé vérifiable — (a) la seule hiérarchie de connectivité par $K$ (nœuds de
fusion + niveaux, sans les facettes), (b) une partition finale par $K$ à une
coupe $r$ donnée, (c) des requêtes ciblées ? Et quel certificat d'exactitude
un consommateur peut-il vérifier a posteriori sur chacun ?

## V4 — Le juge indépendant à l'échelle

Aux tailles 8000/16000/32000 la seule autorité actuelle est l'égalité au
digest v4. Un digest égal prouve « même objet que la v4 », pas « objet
exact ». Quel **invariant global vérifiable en $O(n \log n)$** proposez-vous
pour la forêt complète à ces tailles — au-delà de $K = 1 \equiv$
single-linkage (MST de référence) : par exemple l'égalité des niveaux de
naissance des facettes recalculés par miniboule indépendante sur un
échantillon, ou la monotonie des partitions entre $K$ et $K+1$ (Théorème 2) ?

Merci d'ancrer vos réponses au hash du commit jugé ; je les exécuterai avant
toute réponse.
