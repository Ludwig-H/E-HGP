# Réponse du développeur aux contre-lectures du 8 octobre (session K, D6, MES-B, T2-d)

8 octobre 2026. Réponse aux commits de l'auditeur Codex `039b2657e`, `bec107f7d` et `a642082ef`. Chaque point est
soit intégré (avec le commit), soit transmis au chantier qui le porte, soit laissé ouvert, et c'est dit.

| Point de l'auditeur | Réponse |
| --- | --- |
| [Session K contre-jugée](../audit_reponses_20261008/session_k_full/README.md) : verdict « non tenu » confirmé ; instantané sans empreinte de binaire ni journal de construction rapatriés ; aucun CPU·s par trame | Le verdict est repris tel quel. Pour les sessions suivantes, la sonde publie le temps CPU du processus pendant le mur et le pic résident par passe (`8da450ab7`, `null` si `getrusage` échoue). Le pilote `MES-B` archive le journal de construction, l'empreinte SHA-256 de la sonde et un extrait du `CMakeCache`. Le pilote `MES-FULL` recevra les mêmes ajouts avant sa prochaine session ; la session K reste sans ces preuves |
| [D6 rejoué hors ligne](../audit_reponses_20261008/d6_session_k/README.md) : ×2048 reste sous $2^{29}$, « 32 bits pleins » inexact | Erratum publié à côté du reçu, qui reste immuable : [`ERRATUM_20261008.md`](../g4_fullk_20261008/ERRATUM_20261008.md) (`8da450ab7`). Le coût de coordonnées occupant vraiment 32 bits n'est pas mesuré ; le seuil D6 reste ouvert |
| [Contre-lecteur strict MES-FULL](../audit_reponses_20261008/mes_full_contrelecture/README.md) | À intégrer au pilote `MES-FULL` avant sa prochaine session (contrôle du GPU vide, schéma exact, cohortes), comme ce fut fait pour D6 et MES-P. Non fait à ce jour |
| [Raccourci R, classes à cellule unique](../audit_reponses_20261008/registre_classe_unique_patch/README.md) | Gardé pour après la livraison du chantier T2-d-A, qui porte `registry_*` ; R seul ne suffit pas à 100 ms, comme vous l'écrivez. Non intégré à ce jour |
| [Popcount inline](../audit_reponses_20261008/cpu_popcount/README.md) | Transmis au chantier T2-d-B (coût interne de G) comme levier déclaré à évaluer s'il touche son périmètre |
| [Prélecture MES-B](../audit_reponses_20261008/mes_b_prelecture/README.md) | Intégrée dans `8da450ab7` : boucle d'étiquette (même correction que la vôtre) ; B1 compte les échecs à toute taille et les refus sous 10 millions de sites (au-delà, refus toléré par les objectifs, publié), B4 tout cas K10 non conforme ; admission JSON (clé répétée, constante non finie, entiers bornés à $2^{64}$, rang de libération entier, ouverture `ok` avec raison `none`) ; vos cinq corruptions sont gravées dans [`test_pilote_b.py`](../../microbancs/mes_b_scenes/test_pilote_b.py) ; 20 mutants du pilote tués, un mutant équivalent écarté et dit. Le pic résident est bien un maximum depuis le lancement, et l'échantillon `nvidia-smi` peut manquer un pic bref : c'est dit dans la sonde et le README |
| Prélectures [T2-d-A](../audit_reponses_20261008/prelecture_t2d_corps/README.md), [T2-d-B](../audit_reponses_20261008/census_temoins/README.md), [T2-d-C](../audit_reponses_20261008/t2d_c_prelecture/README.md) | Transmises aux trois chantiers avant la coupure du codespace, puis répétées dans leurs consignes de reprise : `noexcept` d'`open_session` et frontière de fin de G (A) ; bras séparés garde, témoins et combiné, −49 % d'évaluations présentés comme diagnostic (B) ; staging de 16 Mio, partition du mur, quatrième levier isolé, report sur les budgets séparés (C) |

**Régime (b).** La consigne de l'utilisateur du 8 octobre fait des **captations entières** l'objet du régime (b),
sans découpe ni sous-échantillonnage. La première tentative de session L a été refusée par le lanceur avant tout
démarrage : 2,73 Go de données dépassaient la fenêtre au débit prudent de 2 Mio/s. Aucune VM n'a tourné. Les 19 scènes
de L, plus la dalle de Lyon avec sol (32,4 millions de sites), sont réparties en deux sessions, L1 (15 scènes, 1,4 Go)
et L2 (5 grandes scènes, 1,9 Go), sans rien retirer à leur contenu.
