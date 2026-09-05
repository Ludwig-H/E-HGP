# Prétest exact q2 avant matérialisation locale

Note du 5 septembre 2026, préparée avant port du candidat E. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [reçu q2](../receipts/meb_q2_review_20260905/README.md) qualifie un overlay local. Il ne transfère ni le résultat mono C/D précédent, ni la qualification complète D vers E. Le statut courant de la lignée reste dans [PASSATION](../PASSATION.md).

## Identité et largeur

Pour des positions a, b, z dans le profil u16, la boule diamétrale possède déjà la clé primitive A=1, B=−(a+b), C=a·b. Le rejet peut donc être décidé sans construire la clé ni le niveau :

$$P_{a,b}(z)=(z-a)\cdot(z-b)=z\cdot z-(a+b)\cdot z+a\cdot b.$$

C'est exactement la puissance de `q2_ball_key(a,b)`, sans facteur d'échelle, flottant ou approximation. Un résultat positif rejette ; zéro reste accepté, notamment pour les deux supports et pour toute coquille étrangère que le contrôle final devra refuser.

Avec M=65535, chaque différence est déjà calculée en i64 dans `P3`, dans [−M,M]. Chaque produit a un module au plus 4 294 836 225 ; les deux sommes partielles ont un module au plus 8 589 672 450 puis 12 884 508 675, strictement inférieur à 2^34. **Tous les intermédiaires**, pas seulement le résultat final, tiennent dans i64. La preuve concerne les différences de positions u16, pas un `p3_dot` sur des entiers arbitraires ni un index forgé manuellement.

## Conservation de la recherche et gain structurel

La charge du cap précède toujours chaque essai. Les boucles et l'ordre des paires restent inchangés. Un prétest rejeté rencontre exactement un site que le `accept` original aurait rejeté ; une paire retenue construit la même clé et le même niveau `promote_level(q2_exact_level(norm2(a-b)))`, puis appelle ce même `accept`. Par induction, la première paire retenue ou le passage à q3 survient au même compteur, avec les mêmes sorties. Les branches q3/q4 et le contrôle final de coquille restent inchangés.

La clé q2 était déjà primitive **sans PGCD**. Le travail évité sur les rejets est sa matérialisation, la réduction du niveau D²/4 et sa promotion, ainsi que le prédicat général i128 remplacé par le produit relatif i64. Une seule clôture de matérialisation est partagée avec le témoin de coût TESTING ; ni compteur public ni structure globale n'est ajouté. Aucun catalogue de cofaces ou mosaïque supplémentaire n'est construit.

Une paire retenue subit en revanche **une passe de prétest supplémentaire**, puis la passe de `accept` conservée : deux passes au total, pas deux prétests ajoutés. Le compilateur peut aussi avoir simplifié une partie du calcul initial. Aucun gain temporel universel n'est donc déduit du raisonnement ni du nombre de matérialisations logiques évitées.

## Portes et limites

Le test proposé étend la référence locale historique sans la remplacer : 170 scènes, ordre inversé, tous les caps jusqu'au coût plus un, sorties initialisées par sentinelles, égalité des statuts/raisons, des 13 statistiques et des champs littéraux de clé/niveau/support. Il ajoute des extrêmes au-delà de i32 dans les deux sens, des zéros étrangers, deux rejets de profil avant prédicat et un mutant `>0` vers `>=0` tué sur une paire q2 minimale. Le mutant eager conserve les objets mais réintroduit le coût logique rejeté.

La campagne de l'overlay produit 11 816 comparaisons : 668 succès, 12 dégénérescences, 11 136 refus cap. O3 et ASan/UBSan hors sandbox passent les huit argv attendus ; les huit échecs initiaux LeakSanitizer sous ptrace restent archivés, sans désactivation de la détection des fuites. Un différentiel séparé sans `MHGP7_TESTING` retrouve les 11 816 comparaisons. Ces portes sont locales, pas une preuve de publication transactionnelle ni d'exactitude globale.

La revue q2 antérieure conserve son sceau de correction, mais son microprobe historique ne conserve pas les bruts de run ni le code terminal : aucun temps D/E historique n'en est publiable. Le présent reçu ne réalise pas de nouveau microchronométrage et ne satisfait aucun contrat de latence ou de taille. Qualification intégrée et mesure de tour entière restent nécessaires après port.
