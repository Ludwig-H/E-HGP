# État en cours — point de reprise

> Réécrit le 8 août 2026 au soir. La version précédente de ce fichier annonçait
> `mhgp_oracle2` rouge et les régressions R1 à R6 : c'était vrai avant les
> corrections décrites ici, ce ne l'est plus. `WARNING_AUDIT_PUBLICATION_3.md`
> a relevé cette désynchronisation, entre autres.

## 1. Ce qui est vert

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./build/mhgp_tests        # OK
./build/mhgp_oracle       # OK — 496 cas P2, catalogue identique
./build/mhgp_regressions  # OK — R1 à R8
./build/mhgp_oracle2      # OK — porte par défaut
./build/mhgp_oracle2 300 <graine>   # même porte, campagne longue
```

Campagne longue du 8 août, cinq graines (4242, 90210, 777001, 13, 20260808) :
**1 462 nuages comparés, 89 247 cas O2, zéro désaccord**, et 22 nuages refusés
comme hors domaine avec refus certifié.

## 2. L'échec O2 est expliqué et fermé — deux causes distinctes

### 2.1 Le lecteur du niveau était un `double` (corrigé)

Les trois échecs observés avant l'arrêt étaient **tous** des égalités exactes :

- `k=3, a=1075.304040863833` : le minimum $\lbrace0,1,2\rbrace$ a un niveau
  **exactement égal** à $a$ (comparaison rationnelle : $0$), mais `beta > a` est
  vrai en `double` — la naissance est manquée ;
- `k=2, a=1075.30\ldots` et `k=3, a=1167.24\ldots` : le nœud de multifusion est
  *au* niveau $a$ ; le même écart d'un ULP empêche la remontée, d'où trois
  composantes au lieu d'une.

La forêt décidait déjà tout en exact (`sphere_cmp_beta`) ; c'était **le lecteur**
qui était faux, et il n'avait pas le choix : `ForestNode` ne publiait son niveau
qu'en `double`. Deux corrections :

- toute multifusion porte désormais une `source` — la plus petite (par index)
  des sphères de rang $k+1$ du lot qui participent à cette composante. Le niveau
  exact de **chaque** nœud est donc relisible ; `beta` n'est plus qu'un affichage ;
- `forest_partition` de l'oracle compare en rationnels, plus en `double`.

Régression **R8**. L'oracle vérifie en outre la monotonie exacte des niveaux
vers la racine et l'absence de deux multifusions consécutives de même niveau.

### 2.2 Les cosphéricités étaient jetées en silence (corrigé)

La campagne longue a ensuite exhibé des échecs d'une **autre nature**, à `k=3`,
sur des nuages contenant une configuration cosphérique. Exemple mesuré : la
boule de paire diamétrale $\lbrace4,7\rbrace$ porte $p_1$ exactement sur sa
sphère — coquille $\lbrace1,4,7\rbrace$ pour un support minimal $\lbrace4,7\rbrace$.
$\Gamma$ voit la fusion que cette boule porte à l'ordre 3 ; le catalogue rejette
la sphère, donc la forêt ne l'a pas.

`DESIGN.md` §6.4 déclare déjà ces coquilles **hors modèle**, et promet qu'elles
sont « détectées, jamais supposées silencieusement ». Seul l'**oracle** comptait :
`classify` renvoyait `-2` et l'appelant jetait la sphère sans rien publier. Une
hiérarchie incomplète sortait donc en se disant autoritaire. Corrigé :

- `Catalogue::degenerate_shells` et `Catalogue::shell_anomalies` comptent les
  rejets ;
- `Result::out_of_declared_domain` en découle, et `run` ne publie **aucune**
  forêt dans ce cas ;
- l'oracle croise les deux détections et exige le refus ; régression **R7**.

Note : les fixtures R6 et R7 sont elles-mêmes cosphériques, le garde de domaine
est donc le plus extérieur. R8 a reçu une fixture en position générale.

**Précision de l'utilisateur (8 août)** : les nuages visés sont réels, on a le
droit de **supposer l'absence de cosphéricité**. Le garde reste — c'est une
déclaration de domaine, pas un obstacle au contrat.

## 3. Le mur, mesuré : le voisinage certifié n'élague pas

| $n$ | $K$ | quadruples candidats | temps (2 vCPU) |
| ---: | ---: | ---: | ---: |
| 200 | 10 | 258 739 800 | 26,3 s |
| 500 | 2 | — | > 300 s |

$258\,739\,800 = 200\cdot\binom{199}{3}$ : le voisinage $W_p$ vaut **le nuage
entier**, et cela ne dépend pas de $K$. Le coût est donc en $\Theta(n^4)$, et
l'objectif — 50 000 points, $K=10$, moins d'une seconde — est hors d'atteinte de
plusieurs ordres de grandeur tant que ce point n'est pas compris. C'est le
premier verrou, avant toute question de publication.

## 4. Obligations ouvertes de `WARNING_AUDIT_PUBLICATION_3.md`

Aucune n'est fermée à ce jour :

1. l'arithmétique de l'oracle **déborde** (`i128` signés testés après coup) et
   ne qualifie pas la grille 16 bits déclarée ;
2. la campagne n'est pas fermée (nuages sautés en silence, `mhgp_oracle2 -1 1`
   annonce `OK` avec zéro nuage) ; une forêt censurée neutralise O2 au lieu de
   le faire échouer ;
3. O2 ne compare pas la structure qu'il annonce (ni arité, ni enfants, ni
   racines, ni sources) ;
4. le reçu JSON masque exactement les états qui retirent l'autorité, et le CLI
   rend toujours 0 ;
5. le catalogue n'est ni canonique ni déterministe (membres non triés,
   payload dépendant du nombre de fils) ;
6. l'autorité est contournable par l'API publique (`build_forest` accepte
   n'importe quel catalogue ; options non validées) ;
7. v2 n'est dans aucune CI ; la documentation n'est pas synchronisée.

## 5. Suite immédiate

1. Fermer le §3 : comprendre pourquoi la croissance absorbe tout le nuage, et
   décider si la réparer suffit ou si l'énumération locale exhaustive doit
   céder la place à un générateur sensible à la sortie.
2. Puis les obligations de l'audit 3, dans l'ordre : correction (1, 2, 3, 5, 6)
   avant publication (4, 7).
