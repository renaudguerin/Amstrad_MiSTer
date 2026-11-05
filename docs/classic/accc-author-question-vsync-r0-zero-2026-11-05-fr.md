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

Un petit programme original prépare désormais cette mesure sans code SHAKER.
Il attend une VSYNC ordinaire, fixe R4=127/R9=31 pour éviter un nouveau
passage sur C4=R7 pendant la mesure, puis écrit R2 et R0. R7 reste à10.
Après320 NOP, puis deux intervalles de32 NOP, il lit PPI.B. Dans le rejeu
T80, les lectures commencent330/373/416µs après l'écriture de R0 : elles
suivent de plus de118µs le premier débordement de C0. Les cas R2 inaccessible
ont une HSYNC transitoire avant ce débordement, puis plus aucune dans les
64µs précédant la première lecture.

| R0 / R2 | Modèle et MiSTer c0939d8 | AmSpirit1.17.1 | CPC physique |
|---|---|---|---|
|63 /46, témoin ordinaire|111|111|À mesurer|
|1 /1|000|000|À mesurer|
|1 /63|000|000|À mesurer|
|0 /63|000|111|À mesurer|

Le diagnostic exécuté sur MiSTer donne les mêmes quatre lignes que notre
modèle, dans deux captures identiques. Le comportement AmSpirit ne correspond
donc pas simplement à une horloge
qui attendrait chaque HSYNC. Il reste compatible, entre autres, avec une
condition R0=0 ou une phase minimale C0=1 jamais atteinte à R0=0. Ces
résultats logiciels ne tranchent pas la règle physique et trois lectures
hautes ne prouvent pas une impulsion continuellement haute. Il faudrait
compléter la mesure par une observation directe HSYNC/VSYNC, puis un cas
de reprise de R0 si nécessaire. R0=R2=0/R3=1 reste exclu : le §15.3.2 p.150
indique une HSYNC infinie sur CRTC1.

Le disque privé `VSCLK_51d2a99f.dsk` se lance par `RUN"VSCLK.BIN` et affiche
les trois bits de chacun des quatre cas, sans verdict imposé. La mesure
physique reste à effectuer ; aucune modification RTL n'a été retenue.
[Programme, identités et qualification](../investigations/hardware-runs/shaker-crtc1-vsync-clock-probe-2026-11-05.md).

[Preuves, limites et provenance](../investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md).
