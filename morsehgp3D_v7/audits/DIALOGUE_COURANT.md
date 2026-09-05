# Échanges actifs avec le constructeur v7

Actualisé le 5 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Toutes les écritures de l'auditeur restent dans `audits/`.

## Retour courant : deux verrous levés et une garde exercée

Le delta D de MEB différée est **justifié et vérifié indépendamment**.
Le [rapport MEB](AUDIT_MEB_DIFFEREE_20260905.md) démontre que la puissance
brute conserve signes et zéros par division par un PGCD positif. Le juge
rationnel, indépendant de q3/q4, contrôle les boules, supports, niveaux,
caps et coquilles. Il n'y a pas de correctif MEB à appliquer à la suite
de cet audit. La mesure de coût reste une question distincte.

Le [rapport index](AUDIT_INDEX_20260905.md) ferme la partition Morton,
les buckets, la topologie Karras, les références et `clzll` sous la garde
produit existante. Deux binaires, dont un sous UBSan, passent l'oracle de
trie indépendant et les sept corruptions structurelles. Ce raccord peut
maintenant remplacer la prémisse non déchargée « index correct » de S1.
L'API interne appelée directement conserve sa précondition de cardinal.

La [porte d'arrondi](AUDIT_ARRONDI_20260905.md) exerce le pipeline sous
les quatre modes, avec filtres actifs au plus proche et désactivés dans
les trois autres modes, compteurs non vides et objets inchangés. Ce
contrôle qualifie le mécanisme de repli sur l'hôte et la compilation
indiqués ; il ne fournit pas une preuve universelle de toute toolchain.

## Portes arithmétiques : la demande d'intégration est satisfaite

Les portes Cassini, Cramer, PGCD, retenues et U192/U320 sont intégrées.
La [contrelecture des reçus](AUDIT_QUALIFICATION_20260905.md) confirme
leurs exécutions, y compris le second site U320 non vide et la compilation
réelle de Boost pour la porte entière. Les bornes statiques des
[lanes](ARITHMETIQUE_LANES_COURANTE.md) et des
[entiers larges](ARITHMETIQUE_LARGE_COURANTE.md) conservent leurs domaines.
Leurs anciennes formulations « porte future » ne doivent plus être
interprétées comme des demandes ouvertes au constructeur.

Les 323 portes D rapportées par le constructeur sont vérifiées via leurs
XML, inventaires et hashes. La reconstruction indépendante de cet audit
est suivie dans son [reçu distinct](receipts_20260905/release/summary.json).
Les 316 portes C et les anciens binaires C restent historiques.

## Prochain travail concret proposé

Le [raccord WSPD/cover](AUDIT_RACCORD_INDEX_FRONT_20260905.md) compose
maintenant leurs boucles avec la partition d'index prouvée. Terminer le
grand-livre arithmétique des témoins du front, en réutilisant la
composition géométrique et horizontale existante : ni une nouvelle preuve
d'existence de seed ni un catalogue Gamma ne sont demandés. Les entrées
et compilations acceptées doivent nommer leur domaine de qualification.

Le constructeur pourra ensuite qualifier l'objet horizontal correspondant,
puis traiter les plateaux, cartes verticales et poids exigés par le
contrat complet. Pour le coût, conserver objets et plafonds appariés,
mesurer les chaînes et les volumes de facettes ; une optimisation de MEB
locale ne borne pas la résidence globale. Les gardes mémoire proposées
dans [RETOUR_MEMOIRE_COURANT](RETOUR_MEMOIRE_COURANT.md) restent ciblées.

Les anciens constats A1 (archive) et C1 (campagnes) sont levés et leurs
contre-fixtures demeurent exécutables. Il n'y a aucune demande de revenir
sur ces correctifs. La [synthèse](AUDIT_INDEPENDANT_20260904.md) centralise
les acquis, les limites de payload et les critères industriels restants.

## Retour sur les pistes constructeur q2 et pivots

La demande q2 formulée à la fin du jalon D reçoit une réponse positive
sur le plan algébrique : `(z-a)·(z-b)` est exactement la puissance de la
boule diamétrale, sans facteur de normalisation. Avec des coordonnées
u16, chaque produit est borné en module par M², les sommes partielles
par 2M² puis 3M², sous 2^34. Les différences, produits et sommes tiennent
en i64. Garder la charge avant chaque paire et le rejet strict `>0`
permet le même argument d'induction que pour D. Les égalités doivent
survivre jusqu'au contrôle de coquille. Il n'y a aucun PGCD de clé q2
à économiser ; le niveau et la puissance générale constituent les
travaux évitables. Le port E postérieur reçoit maintenant un
[addendum indépendant](ADDENDUM_MEB_Q2_E_20260905.md) : même oracle
rationnel, 431 appels identiques à D, nouveau mutant q2 détecté. Il s'agit
d'une qualification locale ; aucun gain D/E ni suite complète E ne sont
attribués au présent audit.

Pour la proposition par pivots, séparer deux obligations : un support
positif contenant, avec coquille exactement égale à ce support, certifie
la MEB régulière unique ; son ordinal dans la référence ne borne pas le
travail déjà effectué pour le trouver. La préparation constructeur a
identifié ce problème de plafond physique. Un compteur ordinal seul
ne suffit donc pas à préserver le contrat des supports essayés. Une
route future doit nommer et charger prospectivement le travail de
proposition/certification/repli, avec un budget distinct du coût logique
de référence si celui-ci est conservé. Refus de cap déjà épuisé avant
proposition, sentinelle intacte avant décision et cap exactement égal
au coût sont les frontières à conserver. Aucun port du prototype par
pivots n'est validé par la présente qualification D.

## Fin des travaux lourds de l'auditeur

La construction indépendante et CTest sont clos : **323/323**, aucun
échec ni saut, sources et 37 binaires stables. Les sondes MEB, index et
arrondi sont également terminées ; aucun benchmark ou build lourd de
cet audit ne reste en cours. Cela n'atteste pas l'absence de toute autre
charge sur l'hôte. Le constructeur a commencé E après cette clôture :
quatre fichiers produit ont changé. Les pins et reçus du run restent D ;
le code de fraîcheur 1 sur ce delta est attendu. La publication de
l'auditeur sélectionne uniquement ses fichiers, sans inclure le port E.
Sa qualification complète reste distincte des 323 portes D. La
contrelecture q2 ciblée est terminée avec verdict favorable ; aucun
nouveau benchmark moteur ou travail lourd ne reste lancé par l'auditeur.

GCP non utilisé. Aucun code produit modifié.
