# Transport des intérieurs — prototype du consommateur

26 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`mode=audit_payload_consumer`, `public_status=not_claimed`. GCP non utilisé.
Code produit, CMake produit et sceau de catalogue inchangés.

Le prototype montre que reconstruire une `BallData` régulière depuis un
paquet complet d’IDs est moins coûteux que rechercher de nouveau ses points
dans l’index. Il ne fournit ni producteur GPU de ces paquets, ni gain net de
chaîne, ni qualification FULL. Les paquets de cette expérience proviennent
d’un **premier census global payé et publié séparément**.

## Ce qui est implémenté

[`payload_probe.cpp`](payload_probe.cpp) utilise directement les primitives
u18 de la tour pour les clés, niveaux, positivité et census. Il construit un
propriétaire immuable après copie des entrées et une permutation inverse
ID original → rang Morton. Les IDs couvrent `0..n-1`, comme dans la chaîne,
mais leur affectation aux coordonnées et l’ordre physique d’entrée sont
permutés indépendamment. L’index ne dépend plus du stockage du demandeur.

La seule fabrique normale d’un paquet effectue le census global et conserve
le propriétaire, le support, la clé, les comptes exacts et les IDs intérieurs
par valeur. L’import régulier vérifie le propriétaire, reconstruit clé et
niveau, teste la positivité, la fenêtre K, la cardinalité et l’unicité des
IDs, leur puissance strictement négative, puis les puissances nulles du
support. Il convertit les IDs en rangs de la tour, trie ces petits tableaux
et matérialise la même `BallData` que le témoin. **Les deux bras paient la
reconstruction géométrique et la positivité**, pas seulement un test de
puissance. L’importeur n’effectue aucune recherche globale.

Si la coquille dépasse l’arité du support, l’import renvoie
`needs_global_census` sans modifier la sortie. Cette expérience ne porte pas
le traitement des plateaux : le raccord produit devrait alors appeler le
`census_key` historique, qui établit aussi q_min et Euler.

## Preuve et frontière de confiance

Pour un paquet lié à la même entrée et à la même clé, le producteur certifie
`|I| = d` et `|U| = q`. Si l’importeur reçoit d IDs distincts de puissance
strictement négative, ces IDs constituent nécessairement tout I. Si les q
IDs distincts du support ont puissance nulle, ils constituent nécessairement
toute U. Les opérations restantes reconstruisent l’enregistrement canonique.
Le travail consommateur est O(K log K), mémoire O(K), par paquet régulier ;
cette borne ne porte ni sur le nombre de paquets, ni sur leur production.

La fabrique de ce prototype certifie ses comptes par census global. Un futur
producteur GPU pourrait les certifier par son census déjà complet du cover,
après positivité et propriété de plus longue arête. Pour un support positif
de q points, centre c, poids positifs λ et diamètre L :

$$R^2=\frac{1}{2}\sum_{i,j}\lambda_i\lambda_j\lVert p_i-p_j\rVert^2\leq\frac{q-1}{2q}L^2.$$

Au milieu m d’une arête de longueur L,
`|c−m|² = R²−L²/4`. Ainsi `R+|c−m| < L` pour q=3 et q=4 : le cover fermé
de rayon L contient tous les intérieurs et la coquille. Le filtre L11 doit
continuer à conserver tous les contacts. Ce raisonnement justifie une piste
de port ; ce prototype ne qualifie pas le transport depuis CUDA.

**Une validation locale n’authentifie pas des comptes arbitraires.** La
fixture `drop_and_forge_count` retire un ID intérieur et réduit simultanément
le compte certifié : l’import local accepte alors la fausse liste, tandis
que le recensus global la réfute. Cette contre-fixture est exécutée trois
fois et publiée. Elle démontre pourquoi une simple structure publique
munie d’un booléen `certified` serait insuffisante. `AuditAccess` est un point
d’injection explicite de ce seul prototype ; aucune promesse de sûreté face
aux corruptions arbitraires de mémoire n’est faite. Le futur sceau doit
décrire la provenance de ses comptes et garder un oracle global différentiel.

## Vérifications et mesures

Capture [`results/MANIFEST.json`](results/MANIFEST.json) : base Git,
compilateurs, commandes, codes de sortie, empreintes des dépendances locales
avant/après compilation, binaires et machine. Nouvelle construction dans
`/tmp/mhgp9-census-payload-capture-*`, aucun build épinglé réutilisé.
Release GCC `-O3 -DNDEBUG` et Clang ASan/UBSan passent tous deux :
276 comparaisons exactes `BallData`, 276 contrôles d’IDs/permutations,
34 refus ciblés et une coquille étendue orientée vers le repli. Les cas
couvrent intérieurs vides, profondeur maximale K10, coordonnées u18 larges,
les 6/24 permutations de supports q3/q4, support non positif, IDs dupliqués,
absents ou sur la coquille, clé/compte/propriétaire incohérents et durée de
vie du propriétaire. La contradiction de confiance ci-dessus reste visible.

Les entrées de mesure sont des groupes synthétiques disjoints de six sites :
alternativement un triangle positif et trois intérieurs, puis un tétraèdre
positif et deux intérieurs. Une boule est interrogée par groupe, K5, un fil.
**Ce n’est pas un régime LiDAR ni un générateur global.** Les cinq essais
alternent recensus→import puis import→recensus ; les médianes murales sur
l’hôte local partagé sont :

| Sites | Paquets | Recensus + reconstruction | Import + reconstruction | Rapport |
| ---: | ---: | ---: | ---: | ---: |
| 8 000 | 1 333 | 1,237 ms | 0,352 ms | ×3,52 |
| 16 000 | 2 666 | 2,592 ms | 0,710 ms | ×3,65 |
| 32 000 | 5 333 | 4,959 ms | 1,344 ms | ×3,69 |

La production initiale des paquets prend respectivement
1,805 / 3,376 / 7,042 ms ; la préparation propriétaire/index,
2,979 / 6,748 / 12,928 ms. **Ces coûts sont exclus des deux bras**, puisque
la question est celle du consommateur une fois le paquet disponible.
Additionner production + import ne donne pas ici un nouveau moteur plus
rapide : le gain suppose de récupérer les IDs lors du travail déjà requis
du véritable producteur.

Le recensus visite 45 225 / 95 826 / 203 893 nœuds ; l’import en visite zéro.
Ses temps font ×2,020 puis ×1,893 aux doublements, les visites du témoin
×2,119 puis ×2,128. Cela décrit seulement ce jeu à O(n) paquets ; aucune
conclusion de croissance sur la chaîne LiDAR n’en découle. Les timings
courts et l’hôte partagé ne permettent pas de transférer ces rapports à G4.

## Relecture et suite

[`readback.py`](readback.py) recalcule les tableaux depuis les sorties brutes,
vérifie les sources et binaires locaux ainsi que les limites exercées. C’est
un lecteur **LIVE** dépendant de la construction locale, pas une archive
autonome. Il fonctionne en Python normal et sous `-O`.

```bash
python3 -B morsehgp3D_v9/audits/b_census_payload_20260926/readback.py
python3 -B -O morsehgp3D_v9/audits/b_census_payload_20260926/readback.py
```

`run.py` refuse d’écraser `results/`. Pour une nouvelle capture, utiliser
une copie de ce dossier sans ses résultats ou adapter explicitement le
répertoire de sortie. Le prochain jalon utile est un producteur q3 qui
conserve les IDs des masques intérieurs existants, puis le raccord avec un
oracle global activable. Pour q4, payer et mesurer une collecte sur les
émissions avant d’optimiser sa combinaison lentille + événements. Mesurer
copies, transferts, réordonnancement des records et stockage des paquets ;
conserver toutes les associations pendant le tri q4 et la déduplication.
