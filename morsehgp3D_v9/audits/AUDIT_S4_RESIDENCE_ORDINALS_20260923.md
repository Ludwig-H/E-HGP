# S4 : rendre explicite le passage GPU–CPU et l'identité des survivants

23 septembre 2026 — contrelecture de conception, sans port ni chrono. Cadre : `exploration_v9_hors_registre`, `public_status=not_claimed`.

## Point de raccord

Le [plan S4](../docs/s4_conception_20260923/PLAN_S4.md) prévoit en S4a que le GPU traite q3 pendant que le CPU calcule l'atlas et q4 (§0 et §2), puis indique que les survivants et masques S2/S3 restent sur l'appareil (§3). L'[analyse GPU](../docs/s4_conception_20260923/ANALYSE_ARCHITECTURE_GPU.md) va jusqu'à dire que l'hôte ne reçoit que les attentes et le ledger (§2). Ces deux descriptions ne peuvent pas être simultanément vraies **pendant S4a** : le CPU a besoin, avant son calcul q4, des arêtes q4 ouvertes, de leurs rangs/IDs d'origine et de leurs masques S3. Le `Q34CertificateBatch` actuel porte justement un masque et un statut par survivant ([interface](../src/gen/pipeline/wspd_q34.hpp), lignes 330–348) ; la phase 3 lit ces champs à l'ordinal `j` du survivant ([pipeline](../src/gen/pipeline/wspd_q34.cpp), lignes 1409–1455). Le gain attribué à la résidence ne doit donc pas supposer la suppression de cette copie dans le jalon S4a.

Solution minimale : après S3, former une liste compacte hôte `(source_ordinal, a_rank, b_rank, q4_mask, status)` des seules arêtes à calculer en q4 sur CPU ; conserver `source_ordinal` jusqu'à la réunion des enregistrements GPU q3 et CPU q4. Lancer la copie et le noyau q3 sur des flux compatibles, puis déclencher les workers q4 seulement quand la copie est complète. Compter dans le **mur de chaîne** la compaction, les octets et le temps du transfert, l'attente, les arêtes CPU calculées en parallèle et la traîne des débordements GPU. Une session résidente peut alors supprimer les copies inutiles sans cacher le raccord réellement nécessaire. Le jalon S4b, si q4 passe aussi sur GPU, pourra garder les survivants sur l'appareil hors attentes et juges.

## Interface utile au certificat de groupe pré-cœur

Le [shadow de tuiles d'arêtes](paired_guard_group_bvh_20260923/SUMMARY.json) trouve des fermetures exactes potentielles **après S2 et avant le cœur S3**, sur un groupe spatial d'une trame brute ; son coût net intégré n'est pas établi. Le [shadow par rectangles](rect_pair_shadow_b_20260923/README.md) montre au contraire qu'un certificat uniforme par rectangle ne ferme que 0,904 % de `F` sur cette même trame. S4 ne doit donc pas présupposer ce gain, mais son interface devrait pouvoir accueillir une décision exacte avant S3 sans changer l'identité des arêtes.

Conserver l'ordinal original S2 comme identité immuable et transporter séparément le masque ouvert :

```text
S2.source_ordinal, S2.mask
       ↓ certificat de groupe éventuel : mask_group ⊆ S2.mask
       ↓ S3 : mask_s3 ⊆ mask_group
       ↓ S4 : mask_s4 ⊆ mask_s3
```

Une arête totalement close saute S3 et S4 ; une fermeture de seule voie laisse l'autre voie ouverte. Une mise en attente de S3 ou S4 revient au CPU avec le **dernier masque effectivement certifié**, jamais avec un masque partiellement écrit. La liste GPU compacte porte toujours `source_ordinal`, même si les indices de travail changent ; chaque enregistrement émis et chaque repli CPU se rattache une seule fois à cet ordinal. Les compteurs des étapes sautées et ceux du certificat de groupe restent distincts du ledger S2/S3 historique. La porte vérifie, par ordinal, unicité, inclusion monotone des masques et multiensemble exact des émissions ON/OFF, puis les condensés de catalogue et de tour. Elle mesure aussi le coût du groupage, des preuves, des transferts et des retours CPU.

Un BVH pré-cœur **CPU** intercalé entre deux étages résidents imposerait S2 GPU→CPU puis des masques CPU→S3 GPU, avec une synchronisation sur le chemin critique. Le shadow CPU est utile pour mesurer sélectivité et coût ; un port GPU de la preuve de groupe, ou un export compact recouvert et mesuré, serait nécessaire avant d'en déduire un gain de chaîne. La même discipline de mesure s'applique aux demi-scènes, quarts de scène et densités 1/2 et 1/4, brut et sans sol : ces coupes changent conjointement le nombre d'arêtes, de graines et de sites par cover.

Enfin, l'arène de plages S4.0 (§3 du plan) doit préciser l'unité de `(offset, compte)` : sa borne annoncée de 5,6 Go à K10 dépasse un décalage **en octets** sur u32, alors qu'un décalage **en plages de 8 octets** reste sous 2³² pour cette borne. Calculer et vérifier les sommes en u64, consulter la mémoire libre avant allocation et traiter explicitement la capacité insuffisante par repli exact ; aucune durée ni faisabilité G4 n'en est déduite ici.

## Porte de coût S4b et croissance réelle

La porte S4b du [plan](../docs/s4_conception_20260923/PLAN_S4.md), §5, compte le premier balayage, la **fraction** de graines admises au second, les insertions et les comparaisons de racines, puis applique le coût par voie/site mesuré en S4a. Une fraction non pondérée ne donne pas le travail du second balayage : il faut sommer, par arête et par graine admise, les `cover_sites` correspondants. Le coût q4 des racines, des tableaux triés et des ex æquo n'est pas celui de la puissance q3 ; calibrer ces postes sur un pilote appareil q4 avant de décider entre S4b et S4b′. Publier aussi les maxima et quantiles **par arête** de `graines × cover_sites`, du second passage, des ex æquo et des enregistrements, puis le nombre et la durée des replis CPU pour chaque limite mémoire. La capacité d'un tampon d'ex æquo ne doit jamais tronquer silencieusement un groupe : l'arête entière reste à refaire exactement sur CPU, comme le plan le prévoit.

Le budget nominal q3 des balayages complets est \(\sum_e \lceil g_e/32\rceil c_e\) pas de warp, où \(g_e\) est le nombre de graines et \(c_e\) la taille du cover ; les rejets anticipés peuvent réduire le travail effectivement exécuté. Le port GPU accélère ce travail sans en borner la croissance ; il faut publier le nominal **et l'effectif**, leur maximum par arête et le mur total avec transferts/replis, pour les mêmes demi-scènes, quarts de scène et densités. Le gain éventuel d'un certificat de groupe se juge sur ces masses **après** son coût de sélection et de preuve, pas seulement sur le nombre d'arêtes closes.

## Ce que le juge peut prouver avec son payload

Le [plan S4](../docs/s4_conception_20260923/PLAN_S4.md), §2, distingue correctement la porte d'objet **CPU** (IDs de coquille exacts) des enregistrements GPU, qui ne portent que taille et empreinte somme/xor de mélanges 64 bits ; il annonce aussi un préflight `--lanes-judge` comparant le multiensemble des arêtes décidées. Avec ce payload GPU, le préflight peut comparer **exactement les champs consommés par la tour** — clé, support, arité, profondeur et taille de coquille —, tandis que l'empreinte des IDs de coquille n'est qu'un contrôle à collisions possibles. Si la porte de port doit certifier « même objet q3/q4 » avec la **liste exacte** des IDs de coquille, prévoir une sortie d'IDs réservée au mode juge, ou un oracle hôte exhaustif relié aux IDs produits sur l'appareil. Il suffit sinon de nommer explicitement la portée plus étroite du juge GPU ; le recalcul du census de catalogue en aval ne transforme pas l'empreinte en preuve d'identité de la coquille.
