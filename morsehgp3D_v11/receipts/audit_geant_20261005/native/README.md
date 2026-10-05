# Contrelecture native du 5 octobre 2026

Source publiee : `238734f1d03ab32e2a722bf036eb8fc5626dfd44`.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Aucun build, test natif, GCP ou benchmark execute par cette contrelecture.

Les fichiers inspectes et leurs empreintes sont dans `source_manifest.json`, separes
entre pin publie et fichiers **WIP non commis** des worktrees S3/S5/S6.
La lecture porte sur les contrats de proprietaires, compteurs, types, refus,
arithmetique et synchronisation ; elle n'est pas une nouvelle qualification.
Les empreintes sont une capture de lecture, jamais une verification LIVE
des worktrees lors du rejeu du modele. La presente tranche inspecte 41 fichiers publies et 13 WIP ; certains grands
fichiers de tower/catalogue sont inspectes par sections. Les cinq fichiers
IO publies ont egalement ete lus, mais leur revue detaillee est confiee a
l'auditeur principal et n'entre pas dans ces 41 empreintes.

## P1 — WIP S5 : le produit et la Session de publication ne sont pas lies

`src/api/api.hpp:117-136` stocke seulement la requete et la tour. La
Session qui a calcule le produit ne laisse aucun jeton d'identite dans
`Product`. `src/api/manifest.cpp:224-251` prend le budget de la Session
fournie puis serialise `product.full()` sans verifier leur provenance
commune. La declaration publique de `publish` n'en fait pas une
precondition explicite. `finish`, lignes261-264, ferme la Session fournie.

Scenario : creer A (budget suffisant), B (budget0), calculer le produit
entier dans A, le garder vivant, appeler `publish(B, produitA, ...)`.
Le WIP peut publier avec pics de B egaux a0, puis `finish(B)` est conforme,
alors que les tableaux du produit sont reserves dans A. `A.close()`
rendrait `budget_not_released` dans le meme etat. Pas de defaut geometrique
ni de chemin inter-Session dans le CLI actuel, qui n'a qu'une Session.
Cela concerne la facade publique et sa comptabilite, avant integration S5.

Recommandation : jeton stable d'identite de Session (survit au deplacement
comme le compte Budget), stocke dans Product ; verifier avant toute
creation/refonte de sortie et tout diagnostic. Refus explicite pour
une autre Session. Porte future sur G4 dans `api_session_identity.cpp`.
Cette fixture n'a pas ete compilee ici.

`~Session() = default` (`api.hpp:59`) ne controle pas non plus
`MemoryBudget::released()`. `close()` ne fait que ce controle explicite
(`session.cpp:27-30`) et ne ferme pas Pool/budget. L'exigence normative de
controle a la destruction (ARCHITECTURE7.1) demande une implementation ou
une clarification explicite du contrat ; aucun UB n'est deduit, car les
Buffer gardent le compte de budget partage en vie.

## P2 — WIP S6 : support_cofaces admet une arite impossible

`src/supports/counts.hpp:117-127` annonce0 pour une arite qui n'est pas
celle d'un support de la coquille, y compris `a > m`. La garde, ligne120,
ne verifie que2..4. La forme publique `make_shape(2,2,2,3)` est valide ;
`support_cofaces(shape,3)` rend `C(1,1)=1`, bien que trois sites ne
puissent former un support sur une coquille de deux sites.

Le modele exact enumere **51** combinaisons du domaine Shape avec `a>m`
et compte non nul. Ajouter `arity > s.shell()` dans la garde des deux
helpers pour garder un contrat commun. Porte native future : formes
`(p,m,q,K)=(1,2,2,2),(2,2,2,3),(1,3,3,3)` et arites3/4.
Pas d'effet sur des supports valides emis par `ball_supports` ; il s'agit
du domaine d'une fonction publique scalaire, pas d'un contre-exemple FULL.

## Constat connu : hook variadique IO toujours a corriger

Le WIP S5 `tests/cli/io_fault_preload.cpp:72-93` lit six `va_arg(long)`.
La source produit `io/directory.cpp:114` passe cinq arguments :
`int,const char*,int,const char*,unsigned`. La lecture ne respecte ni le
nombre ni les types du contrat variadique. Constat deja ouvert dans la
note active : ne pas le compter comme nouveau defaut produit.
Corriger le hook avant ses deux portes injectees sur G4 ; invalider un
numero de syscall non pris en charge. Cela ne bloque pas une tranche
independante L1 qui ne joue pas ce harnais.

## Controles portables

`python3 check_native.py` puis `python3 -O check_native.py` donnent la
meme sortie : **263848 gardes**. Aucune garde n'utilise assert.

- Formule node_count : recurrence binaire independante, n1..1024,
  feuilles1..256 et cinq grands n dont2^32-2, **263424 cas**.
- Majorants entiers des certificats globaux q3/orientation aux trois
  profils, puissance q4 globale et poids barycentriques locaux : les
  produits et sommes absolues restent strictement sous2^127. Ce controle
  algebrique ne remplace pas la factory ni l'execution C++.
- Domaine de la table de binomes : chaque haut0..35 et bas admissible
  <=13 reste sous2^32.
- Temoins exacts de l'arite invalide ci-dessus ; ce modele suit
  l'expression actuelle du helper et l'attendu vient de `a <= m`.

Le budget/pool/index/census et les voies exactes du catalogue/tower
parcourues ne livrent pas de nouveau defaut de resultat FULL en succes.
Cela ne vaut ni preuve exhaustive, ni nouveaux succes ASan/TSan, ni
qualification u18/u21/u24, ni mesure des objectifs200/100ms.
