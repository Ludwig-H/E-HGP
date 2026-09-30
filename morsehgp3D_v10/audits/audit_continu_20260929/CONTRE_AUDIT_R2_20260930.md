# R2 : progrès vérifiés, collision de sortie et raccord encore distinct

30 septembre 2026. Copie privée `/tmp/mhgp10-r2`, après la réponse
`e9eab2754`. Audit sans modification du moteur, aucun GCP. Les preuves sont
dans [le paquet R2](../../receipts/audit_continu_20260929/r2_followup_20260930/README.md).
Le nouveau [contre-exemple frontière](AUDIT_LAMINARITE_POINTS_20260929.md)
est en section 10 ; il doit entrer dans la porte de conception annoncée.

## Ce qui est maintenant contre-vérifié

| Groupe | Résultat et portée |
| --- | --- |
| Juges | Mon rejeu normal/−O, code 0 : cinq corruptions de listes/ordre auparavant acceptées sont refusées, ainsi que l'attache morte et le plateau ternaire binarisé. Les trois objets valides et les contrôles positifs restent correctement jugés. Aucun nouveau producteur lancé. |
| Tête | Porte native R2 rejouée ici, code 0 en 0,825 s, hashes ciblés stables. Domaine numérique 26/26 fixtures et 25/25 appels invalides sans sortie. La racine zéro est protégée indépendamment de min_cluster_size ; M·λ_max est comparé exactement avant condensation. |
| Coût de tête | Journal développeur observé : témoin temporel code 0, trois retours quadratiques tués par le watchdog de 15 s. Ces mutants ne sont pas relancés ici ; le compteur interne reste un diagnostic. |
| Pool | CAS saturant et chemin série contrôlent les réclamations sans overflow, même avec grain u64 maximal. Les formes alignées new/delete sont testées. Captures terminées observées : ASan 11 commandes, TSan 10, sondes 27, toutes code 0. Cela ne ferme pas toute la campagne ni les différentiels. |

Le premier relevé CLI concluait trop vite à l'absence de build tête R2,
à cause d'une énumération de chemins trop étroite. Cette conclusion est
**corrigée** par le build et la porte maintenant observés. Le relevé brut
reste identique, avec un erratum explicite dans son README de publication.
La source du header a aussi évolué : le reçu de tête donne son SHA actuel.

La campagne R2 des juges est désormais terminale : 33/33 CTests,
`ctest_gate rc=0`, 406,69 s, journal copié sans rejeu. Les 53 changements
de témoin survivant au fuzz sont classés par le développeur comme
représentations équivalentes de même MEB et même composante Γ ; ils ne
sont pas tous des corruptions géométriques. Cette observation ne remplace
pas notre rejeu causal des sept nouveaux refus ci-dessus.

Le mutant de pool `begin<=n` a déjà neuf délais 124 : six portes de 120 s
et trois injections de 300 s, soit 1 620 s de délais programmés pour la
même réclamation vide répétée à n. C'est un défaut du mutant, pas du pool
corrigé. Pour les campagnes suivantes, arrêter cette variante après sa
première non-progression causalement reconnue ; les répétitions ajoutent
peu de preuve. Aucun processus tiers n'a été arrêté par cet audit.

## Nouveau défaut concret : deux sorties vers un même fichier

Sur cinq sites, K2, min_cluster_size=2 et W1, le binaire de la copie CLI
accepte le même chemin pour les étiquettes et `--tree`. Il rend code 0 et
`status=ok`, mais le texte de l'arbre, 126 octets, écrase les 20 octets
d'étiquettes. Le contrôle sans `--tree` écrit les 20 octets attendus.
Les deux appels réels, données, contenus et hashes stables sont conservés.

`OutputSet::open` déduplique le nom sans distinguer les rôles logiques,
puis l'écriture retronque le même fichier. Refuser les destinations
concurrentes **avant réservation**. Documenter aussi les alias de fichier
et les collisions entrée/sortie. Ce défaut d'intégrité de sortie ne prouve
aucune erreur de géométrie.

## Deux conditions de raccord à ne pas perdre

La copie CLI garde l'ancienne tête. Elle n'est donc pas un contrôle de
l'interface après les nouveaux refus numériques. La nouvelle tête rend
des tableaux vides en cas de refus ; le raccord doit conserver sa
propagation de `Outcome` **avant** tout accès aux étiquettes, ainsi que
la validation conjointe avant écriture. Ses tests et ceux de la CLI doivent
être alignés : z=0 et un singleton K1 à niveau zéro ne peuvent pas rester
des acceptations universelles.

Inversement, conserver les écritures vérifiées de la copie CLI lors du
port de cette propagation : la copie tête ne contient pas encore tous
les correctifs d'entrée/sortie. Une fusion de fichiers ne vaut pas un
rejeu sur le binaire commun. Rejouer la collision, les refus numériques,
les mutants des juges et la borne CSR de l'autre prototype après raccord.

Ces gains de fiabilité ne qualifient ni une nouvelle projection frontière,
ni un score ARI/EOM, ni FULL/GPU en 100 ms. Les campagnes encore actives et
les résultats historiques ne sont pas promus.

## Complément : SiteTree, bancs et juge des grands dumps

La [capture suivante](../../receipts/audit_continu_20260929/r2_rounding_bench_stream_20260930/README.md)
ne remplace pas le paquet R2 précédent ; ses hashes restent fermés.

**SiteTree : réserve d'arrondi close à son périmètre.** Rejeu direct de
la porte existante, code 0, 0,989 s : 5 969 requêtes par mode. Le filtre
est désormais limité par `fegetround()` au mode nearest du thread appelant ;
les trois modes dirigés passent au repli exact. G1, bornes et lectures
entre threads sont exercés. Ni FTZ/DAZ ni les autres filtres de FULL ne
sont qualifiés par ce test. Le [rapport SiteTree](catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md)
sépare ce résultat de notre ancien harnais code 1.

**Bancs : ARI hors domaine corrigé, schéma encore incomplet.** Nos
44 appels courts normal/−O confirment les refus R2 puis reproduisent
alpha=2/NaN accepté, paramètres absents ou méthode inconnue passant le
contrôle avant `KeyError`, et colonne `ari_s` dupliquée acceptée.
Alpha=2 peut produire « la tour bat HDBSCAN » sur scores fabriqués,
p corrigé=1/3. Cela ne réfute aucune décision A/C historique : leurs
préenregistrements actuels sont valides. Le [rapport des bancs](timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md)
précise schéma, simulations et vrais signaux locaux.

**Grands dumps : deux contrôles linéaires manquants.** Le lecteur R2
`invariants_echelle.py` se limite explicitement à la structure, pas à
la bijection géométrique de Γ. Sur un petit dump natif K1..2 archivé :

- Retirer tout l'ordre 2 laisse un préfixe K1 cohérent, accepté par le
  lecteur en flux ; le juge exact R2 refuse les ordres manquants.
- Translater de +1000 toutes les coordonnées x des attaches conserve
  leur nombre/unicité ; le lecteur accepte, le juge exact refuse les
  sites inconnus.
- Des tailles annoncées 999/999 passent les deux parseurs : réserve de
  schéma, pas défaut géométrique propre au lecteur en flux.
- Retirer une seule attache est correctement refusé par les deux.

Le [rejeu normal/−O](../../receipts/audit_continu_20260929/r2_rounding_bench_stream_20260930/stream/README.md)
termine code 0 avec résultats identiques. Aucun producteur LiDAR réel n'est
observé émettant un préfixe ou de faux sites. L'interface du lecteur ne
reçoit que n, pas K ni les sites attendus. Passer K effectif et l'ensemble
exact des sites, vérifier les ordres et les tailles annoncées : travail
linéaire en la sortie, sans nouvel oracle combinatoire. Ces gardes ne
qualifieront toujours pas la complétude géométrique des grands dumps.

La [section 11 frontière](AUDIT_LAMINARITE_POINTS_20260929.md) ajoute en
parallèle un résultat positif K2 et de vraies entrées internes K3/K5.
Conserver leurs incidences et continuations est une condition de conception,
pas une autorisation de relancer une grosse optimisation de tête.
Aucun moteur modifié, aucun GCP ni contrat 100 ms nouvellement acquis.

## Clôtures observées et raccord numérique dans la CLI

Observation du 30 septembre, vers 05:03 UTC, distincte des captures
précédentes : le différentiel Pool est terminé, **24/24 comparaisons
identiques**, codes 0, `DONE`. Il inclut catalogue et tour LiDAR00 FULL
K10, référence à quatre fils contre correction à 1/3/8 fils. Son script
compare les 24 premiers hexadécimaux des SHA, soit 96 bits ; il n'impose
pas lui-même un code non nul en cas d'écart. Ici, codes, hashes et marqueur
terminal sont présents : résultat différentiel réel, pas comparaison
octet par octet ni mesure de performance. Le complément SiteTree est clos :
tour 7/7, têtes instrumentées 18/18 et CTest 11/11, codes 0. Ce sont des
campagnes du développeur observées, non relancées par cet audit.

Le lot tête Pool s'est ensuite terminé 24/24, `DONE`, à 05:07:58 UTC,
et ses deux oracles à 05:12:15, code 0. Le premier compare des préfixes
SHA de 64 bits. Les sorties temporaires ont été supprimées par leurs
comparateurs ; aucun digest complet ne peut être reconstruit depuis ces
logs. Notre audit n'a rien effacé ni rejoué.

Deux réserves de juge restent séparées du produit. Le délai ASan de
`contre_pool` est compatible avec sa barrière réutilisée à quatre callbacks
après le départ de l'un d'eux par exception ; il ne prouve pas un deadlock
du Pool. Le juge SiteTree laisse survivre huit mutants, dont le contournement
du mode d'arrondi : la variante instrumentée filtre 84 028 fois en mode
dirigé sans être refusée, contre zéro pour le correctif actuel. Ajouter
une condition explicite sur le chemin exercé ; aucune nouvelle réponse
géométrique fautive du produit n'est démontrée ici.

Notre [nouvelle petite capture](../../receipts/audit_continu_20260929/r2_integration_block_20260930/README.md)
exécute quatre appels sur le binaire tête `50902942…` : K1/mcs1 est refusé
`numeric_domain`, code 2, sans sortie ; une deuxième configuration invalide
est aussi refusée avant toute sortie **dans la même hiérarchie**. Le contrôle
K2/mcs2 produit ses 20 octets. La collision étiquettes/arbre demeure :
code 0, texte de 126 octets remplaçant les étiquettes. Les hashes sont
stables. La copie `entrees_cli-verif` garde l'ancienne tête ; la copie tête
garde les écritures non vérifiées. Ces succès ne qualifient pas leur union.
À 05:19 UTC, la CLI d'intégration inspectée contient encore cinq zones de
conflit de fusion. C'est un état de travail observé, pas une source compilable
ou un nouveau défaut du moteur publié.

## Complément au diagnostic massif

Le [nouvel audit massif indépendant](../AUDIT_MASSIF_LIDAR_20260930.md)
pose correctement la différence entre capacité mémoire et coût de calcul.
Sa certification par K témoins conserve les coquilles admises : p≤K−1
suffit, aucun halo K+1 n'est nécessaire. Le complément scalaire de notre
capture retrouve **un deuxième débordement** de `RankIndex::at_most` :
`lo*64` est calculé en u32 avant le minimum. Pour L=2³²−2, sa borne devient
zéro et la fonction scalaire rend L−61, sans lire de tableau. Élargir le
produit avant le minimum, puis convertir ; corriger aussi le milieu.
Douze valeurs de L passent avec ces formules sûres, normal/−O identiques.
Ce contrôle n'alloue pas des milliards de niveaux et ne qualifie pas une
nouvelle tour.

La [question au développeur](QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md)
précise une piste pour réduire les scans : boîte de centres × bloc du
SiteTree global, rayon supérieur certifié par K témoins et rejet strict
du bloc entier. Les 120 petites configurations rationnelles contrôlent
la formule ; ni un producteur, ni une borne de croissance, ni un gain
LiDAR ne sont mesurés. Les égalités restent présentes, pour FULL et pour
les points frontière. Aucun code du moteur, GCP ou statut public modifié.
