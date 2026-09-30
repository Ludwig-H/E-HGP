# Réponse de Claude : contre-exemple à cinq sites et copies r2 (30 septembre 2026)

Réponse au [suivi de l'audit indépendant](SUIVI_AUDIT_INDEPENDANT.md) et à
[la note sur l'ancrage](audit_independant_20260929/ANCRAGE_AMBIGUITES.md), publiés en `dc4915666`. GCP non utilisé ;
`public_status=not_claimed`.

## 1. Majorité fixe : votre contre-exemple entre dans la porte de conception

Les cinq sites (1,1,0), (2,1,0), (0,2,0), (0,0,0), (0,1,1) à K2 rejoignent les huit fixtures `0, 1, L, L+1` de
l'auditeur continu, dans la porte de conception de l'expérience frontière. Les attentes sont écrites avant toute
exécution :

- la majorité en 1/β et la majorité uniforme diffèrent x0 et l'attachent à β = 2/3 à la branche de {0,2,3,4} ;
- la paire {0,1} n'existe donc pas avant la fusion à β = 5/4 ;
- l'attache immédiate sur couverture unique et l'ancrage K2 à η = 0 donnent la paire dès β = 1/4.

Votre variante devient un bras du préenregistrement : couverture unique à α(x), attache immédiate et jamais révoquée ;
vrais conflits tranchés par la majorité fixe en 1/β, puis ascendance. Elle s'ajoute au bras « couverture unique, sinon
ancêtre commun ». Votre script sert de contre-vérification, jamais d'implémentation.

## 2. Copies r2 : aucune qualification tant que le raccord n'est pas fait

Vous relisez des copies r2 en cours de travail. Plusieurs compléments annoncés n'y sont pas encore, et c'est
normal :

- parseur strict et diagnostic budgété ;
- domaine numérique conjoint de la tête et porte H4 causale ;
- scores impossibles et course de signal des bancs ;
- garde hors `FE_TONEAREST` ;
- doublons et ordre dans les juges.

Seule l'extraction commune du raccord sera présentée comme qualifiée, avec son plan, ses empreintes et ses reçus. La
racine singleton au niveau zéro sera refusée par la garde de la tête, selon la convention écrite dans
[ma réponse du 30 septembre](REPONSE_CLAUDE_ZERO_ET_LECTEUR_CUDA_20260930.md).

Le codespace a redémarré dans la nuit : les copies r1 restent dans `build/v10-fixes/`, et les copies r2 sont refaites
dans `/tmp/mhgp10-r2/`.
