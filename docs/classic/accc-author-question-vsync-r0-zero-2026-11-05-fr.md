# CRTC1 : horloge du compteur de largeur VSYNC avec R0=0

Brouillon non envoyé, 5 novembre 2026. Cette question précise la question 22
sans retenir de nouvelle règle d'après le seul accord entre émulateurs.

Dans la version française 1.11, §14.2 p.133 fixe la largeur VSYNC des CRTC 1/2
à 16 lignes, indépendamment de R3h. Le §16.4.2 p.171 décrit le déclenchement et
la fin à la seizième ligne. Quel événement fait avancer ce compteur lorsque
R0=0 et qu'aucune HSYNC n'est produite ? La condition C0=R0 suffit-elle sans
restriction, ou faut-il un autre état ou événement horizontal ?

Le contexte est SHAKER 2.7, module A, AI/008D. La
[photographie du CPC](https://shaker.logonsystem.eu/images/cpc/CPC/AI_CRTC1_A.webp)
montre le bit 0 de PPI.B à 1 lors des quatre lectures des deux blocs R0=0.
AmSpirit 1.17.1 donne également 1111. Notre capture MiSTer en entrée directe
(208bacf) donne 1001 ; la nouvelle séquence A1 originale (c0939d8) donne
1000, comme CPCSyntaxError 0.12. Les blocs R0=1 donnent 1111. Quatre lectures
hautes ne prouvent pas que le signal physique reste continuellement haut.

Dans notre rejeu du programme original, R0=0 dure environ 18 µs, puis R0
est restauré à #2D. Les lectures commencent environ 5/23/62/66 µs après
l'écriture initiale. Notre compteur expire après 16 pas de 1 µs, sans HSYNC
ni nouveau déclenchement avant la quatrième lecture. Ce rejeu donne 1000,
comme la nouvelle séquence native ; sa quatrième lecture diffère de
l'ancienne entrée directe. Cela ne distingue pas encore l'effet de
l'historique, de l'état omis ou de la phase. Le rejeu qualifie l'expiration
du modèle, pas l'état interne ou la phase du CRTC physique.

Une mesure qui séparerait les hypothèses serait une VSYNC déjà commencée
avec des paramètres ordinaires, puis deux intervalles à R0=1/R3l=1 :
R2=1, puis R2 hors d'atteinte, sans nouveau passage sur C4=R7. Un troisième
cas aurait R0=0 et R2 hors d'atteinte. Il faudrait observer directement
HSYNC/VSYNC sur au moins 64 µs. R0=R2=0/R3=1 ne convient pas pour compter
des impulsions : le §15.3.2 p.150 indique une HSYNC infinie sur CRTC 1.
Ce protocole est proposé, pas exécuté ni utilisé comme oracle de test.

[Preuves, limites et provenance](../investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md).
