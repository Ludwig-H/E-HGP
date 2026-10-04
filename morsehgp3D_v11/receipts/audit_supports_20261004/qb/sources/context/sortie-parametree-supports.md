---
name: sortie-parametree-supports
description: "4 oct. 2026 soir : sortie parametree v11 (full | points | plat | supports), hierarchie des supports d'ordre K ; decisions utilisateur ; workflow wf_a7dbdf1a-21c ; sorties build/v11-persist/sortie_supports"
metadata:
  node_type: memory
  type: project
  originSessionId: a9eae448-9ef2-4a02-9350-4ba9e133e803
  modified: 2026-10-04T17:42:53.557Z
---

Demande (4 oct. 2026, ~17 h 30 UTC) : le retour de Morse HGP 3D v11 devient un parametre (tour FULL, hierarchie de
points, clustering plat) plus une quatrieme sortie, la **hierarchie des supports** : un polyedre = ses supports q2/q3/q4
(aretes, triangles, tetraedres), « un meme support peut correspondre a de nombreuses liaisons » ; travailler sur
**l'arbre d'ordre K** seulement, pas toute la tour. Reference : presentation Zoltan (P_v = union des primitives) et
Zoltan/FoundationModel/JETON.md (option 2 : Q_b).

Decisions de l'utilisateur (AskUserQuestion) :
1. **Tout natif** : facade C++ `api` (Session) + executable `mhgp11 --sortie=full|points|plat|supports` (modules
   points, head, io, api, cli/ prevus par docs/ARCHITECTURE.md mais absents) ; H^r_{k+1} et la tete plate portees en
   C++ (scores exacts), les versions Python restent oracles differentiels.
2. **Tous les supports minimaux Q_b** de chaque boule (canonique, independant des identifiants ; = S* en position
   generale).
3. **Toutes les boules critiques d'ordre K** de la composante pendant la vie du noeud (naissance, fusions, liaisons
   internes) ; chaque noeud publie ses supports propres, P_v = union sur son sous-arbre.

**Why:** cible = tokenizer du modele de fondation guide par la hierarchie ([[zoltan-modele-fondation]]).

**How to apply:** workflow de conception `wf_a7dbdf1a-21c` (lectures -> deux conceptions -> arbitrage), script copie
dans build/workflows/ ; rapports et SPECIFICATION_FINALE.md dans build/v11-persist/sortie_supports/. Ensuite :
workflow(s) d'implementation par tranches, portes et oracles, puis session G4. Agents : jamais de commit.
