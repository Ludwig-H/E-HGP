# Une équipe persistante pour le front et les branches du census q2

15 septembre 2026. Tranche17, exploration v8 hors registre,
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. Périmètre : flux q2 complet, pas q3/q4 ni tour HGP FULL.

## Ce qui change

La tranche16 savait confier les branches d'une même ancre à plusieurs
threads, mais créait une équipe pour chaque appel. Beaucoup de ces appels
étaient trop petits pour amortir cette création. Le nouveau point d'entrée
`run_wspd_q2_census_cooperative` crée **une seule équipe pour tout le front
et son census**. Un worker sans seed disponible peut prendre une branche
de census laissée par un autre worker encore occupé.

On conserve le découpage initial `Coarse` du front et le moteur synchrone
existant. Ce n'est pas encore le raccord au dispatcher `Donate` : son
critère de fin ne connaît pas les obligations du census. Les deux boucles
ne doivent pas être imbriquées en espérant que leurs réveils se coordonnent.
La nouvelle entrée est explicite ; le défaut existant n'est pas remplacé.

## Les objets et leur propriétaire

| Objet | Propriétaire et durée de vie | Ce qui est transmis |
|---|---|---|
| Nuage, arbre et permutation | Même index immuable partagé jusqu'aux jointures | Un propriétaire partagé, pas les coordonnées |
| Seeds du front | Plan global préparé une fois | Indice d'un job jamais exécuté |
| Gros census hors Pool | Continuation possédée, exclusive à son worker | Frère B encore non visité, compte/cursor/phase et B original |
| Petit census | Moteur privé réutilisé par worker | Rien : exécution synchrone |
| Rectangle Pool, y compris repli sans rejet | Plan préparé une fois dans son worker | Rien : plan et bandes restent synchrones |

`SharedBlocks` et `Individual` sont fixés dans cette API. Elle ne convertit
pas silencieusement Pairwise, les groupes conjoints ou les bandes Pool.
Une bande Pool reste une plage dans une permutation distincte de l'arbre B.
Ses témoins ne deviennent pas un crédit initial du census global.

Les continuations sont sélectionnées lorsque `|B| >= min_b_size`, uniquement
hors des rectangles sélectionnés par Pool. Les seuils choisissent une
méthode, jamais un plafond de points ou de recherche. En particulier,
`0 < pool_min_factor <= min_b_size` implique **zéro continuation** : tous
les B éligibles sont alors sur la voie Pool synchrone, repli compris.

## Fin et erreurs

Un même mutex protège la prochaine seed, la file et le nombre d'activités.
La file de census a priorité sur les seeds restantes. Un worker prenant
une seed reste actif pendant l'ensemble de ses callbacks et de ses racines
locales. Ces racines ne créent pas une seconde activité. Un enfant pris
dans la file devient, lui, une activité distincte.

La fin exige simultanément toutes les seeds attribuées, file vide et aucune
activité. Une offre publie un objet possédé avant que l'activité parente
puisse se terminer. File pleine, verrou occupé ou absence de demande :
le worker continue localement. Il n'attend jamais une place pour produire.
Les workers receveurs peuvent attendre la prochaine obligation ou la fin.
`donations_after_seeds_exhausted` signifie que toutes les seeds étaient
**attribuées** lors du don, pas que leur exécution était terminée.

Annulation et erreur réveillent les receveurs ; tous les threads sont joints
avant restitution de l'exception. Les émissions déjà faites ne sont pas
annulées. Les petits census, Pool et callbacks sont encore atomiques du
point de vue de ce scheduler : aucune borne de délai d'annulation n'est
promise. Les callbacks sont privés par slot, mais deux slots peuvent agir
simultanément. Leurs vues d'intérieur/coquille expirent au retour du callback.

## Bilan exact du travail

Chaque rectangle charge une fois sa masse candidate et son descripteur.
La factory d'une continuation standalone les charge aussi pour son propre
contrat : ces deux champs ne doivent donc **pas** être fusionnés à nouveau
dans le bilan du rectangle. Les véritables départs de racines, visites,
divisions, tests de témoins et collectes restent tous comptés.

L'historique d'une branche reste là où il a été payé. Un receveur peut donc
émettre plus de supports qu'il n'a lui-même déclaré de paires candidates ;
la partition candidates = acceptées + rejetées est globale, pas par worker.
Les sommes doivent retrouver exactement la référence Coarse, coquilles et
incidences de supports incluses. Aucun root_start n'est créé au détachement.

Les compteurs de continuations sont séparés : racines initiales, masses
initiales et terminées, fragments, dons, tentatives, attentes et transitions.
À la fin, masses initiales = masses terminées et fragments terminés = racines
initiales + dons. Les consultations et attentes ne sont pas des tests
géométriques ; elles restent visibles dans l'étude de croissance.

## Coût et mémoire : ce qui n'est pas résolu

La file alloue Q pointeurs propriétaires. Avec W workers, au plus Q+W
continuations internes sont vivantes ; l'enfant transitoire est construit
en réservant une place libre. Cette borne ne comprend ni plans Pool,
nuage/index, seeds, pile native, callbacks, métadonnées d'allocation ni
objets créés par l'utilisateur. Chaque continuation réserve encore49
cadres, soit6 272 octets de pile logique ; ce n'est pas un format GPU compact.

Le quantum limite le nombre de transitions entre deux offres, pas les
sorties ni le temps : la collecte d'un support et son callback sont une
transition atomique. La durée mur englobe l'équipe, les files, les objets
privés et leur destruction. La somme des durées workers inclut les attentes
et ne peut pas être soustraite au temps mur. Les temps Pool conservent
leur périmètre synchrone. Le coût d'allocation/détachement n'a pas de
chronomètre isolé ; il est payé dans la durée totale.

Les workers sont tous démarrés si des jobs existent, même avec moins de
seeds que de workers, pour permettre le partage du dernier census.
Avec un seul worker, l'exécution est inline et aucune offre n'est tentée.

Cette intégration ne réduit pas le nombre de tests géométriques : elle
cherche à mieux les répartir. Les coûts de file sont bornés par les pauses
et les dons par les divisions B, mais ces observations ne prouvent pas une
borne globale sous-quadratique du générateur. Une mauvaise granularité peut
coûter plus que le partage ne rapporte. Le profilage doit décider.

Suite : plans Pool parentaux possédés et curseurs de bandes, cursor de
rectangle pour répartir plusieurs ancres, représentation compacte d'une
branche, puis ordonnanceur commun aux continuations du front. Les objets
q3/q4 restent ceux de la [note dédiée](Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md) ;
ils ne doivent pas hériter de la règle particulière d'admission q2.

### Une autre granularité à traiter : les très petits rectangles

Les premières mesures8k s8/Pool64/minB16 ne déclenchent aucune continuation
sur les quatre familles. Ce n'est pas un défaut de complétude : il n'y a
pas de B dans la fenêtre choisie. Beaucoup de travail est dans des millions
de rectangles minuscules. Le don d'une branche d'un gros census ne peut
pas accélérer ces appels. Il faut donc comparer deux chantiers distincts :
les gros parents Pool/bandes, et des lots compacts de requêtes singleton.
Note du 17 septembre : les lots compacts ont été implémentés, mesurés et
fermés comme [résultat négatif](P0_LOTS_SINGLETON_Q2.md) par la tranche 19.

Pour une requête B singleton, aucune division de B n'est possible : une
pile de49 cadres n'est pas nécessaire pour conserver sa position Z. Une
future continuation spécialisée peut porter clé de boule, identifiants,
compte, curseur et contexte original. Il faut aussi conserver le **stade**
(entrée déjà payée ou non, témoins, émission) et tout certificat frère
encore dû : supprimer la pile ne permet pas de répéter ces opérations.
Le rang spatial de l'ancre reste distinct de son ID original pour les
tests structurels de Complement. Aucune taille binaire précise n'est
promise avant l'implémentation et la mesure de ce format.
Attention : un singleton issu d'une
division garde le B original et sa phase, pas un nouveau contexte de racine.
La collecte de toutes les coquilles et leur écriture restent à concevoir
et à payer. Ce format et son passage GPU ne sont pas implémentés ici ;
le constat oriente la suite sans annoncer un gain avant mesure.

Une voie CPU moins coûteuse à comparer d'abord est le **curseur d'ancres
possédé par rectangle**. Un worker cède une plage d'ancres non commencées,
et le receveur utilise son moteur privé réutilisable : pas de création de
continuation complète par petite ancre. Le parent paie une seule fois
descripteur et masse ; les plages disjointes paient leurs vraies racines.
Un parent Pool doit posséder son plan une seule fois, et les plages de bandes
gardent la permutation du plan. Le repli sans rejet peut conserver le
Shared exact tout en partageant ses ancres. Ne jamais reconstruire Pool
par plage. La présente entrée ne possède pas encore ces curseurs ; c'est
une proposition à confronter aux mêmes oracles, comptes et mesures.

## Qualification propre

Les [preuves propres](../receipts/q2_cooperative_20260915/README.md)
comprennent69 CTests Release/Clang ASan/UBSan et une gate Clang TSan PASS,235 appels
coopératifs contre91 Coarse et l'oracle exhaustif, puis174 mesures closes.
La reprise ASan/UBSan est close après redémarrage ; son premier essai
reste incomplet, conservé. Aucun résultat n'est acquis par héritage
de897085f8 ou de l'audit indépendant B.

Le travail géométrique et les sorties sont identiques ; aucun gain stable
de vitesse n'est établi. Pool64 donne zéro don sur les36 mesures de
croissance ; les rangées Pool0 exercent la file mais régressent dans17/18
observations. Les six postes principaux restent sous×3 sur les quatre
séries Pool64 ; F des rangées fait cependant×4 et les paires Pool des amas
jusqu'à×3,447. La masse candidate des rangées sans Pool presque quadruple,
sans développement systématique en paires : ne pas la confondre avec les
visites. Le bilan détaillé publie ces limites, pas une borne générale.

Builds désormais épinglés : `build/v8_cooperative_20260915`,
`build/v8_cooperative_sanitize_20260915`,
`build/v8_cooperative_tsan_clang_20260915`.
GCP non utilisé ; contrats FULL/G4 et massif toujours ouverts.
