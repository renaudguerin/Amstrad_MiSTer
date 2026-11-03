# Question à l’auteur de l’ACCC sur la durée de la gestion de parité RFD

Brouillon du 3 novembre 2026, non envoyé. Il remplace celui du
[29 octobre](accc-author-questions-2026-10-29-fr.md), dont les deux questions
sont résolues par les essais MiSTer du
[rapport de durée de vie RFD](../investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md).
Références : ACCC v1.11 française.

---

**Objet : RFD sur CRTC 1, fin de la gestion de parité**

Bonjour,

Grâce à SHAKER et à vos photos de référence, notre modèle de CRTC 1 affiche
désormais correctement DSC4, Amazing et les pages RFD de SHAKER (B COPY et
D9 « RFD ROUND 2 »). Nous avons lu la section 11.6.1, page 90, ainsi : la
sauvegarde VMA′ échoue quand la parité courante du frame est paire, ce qui
correspond à la recette « IVM ON/OFF avant la RFD » de la section 11.6.2.
Le mot « impaire » de la section 11.6.1 désignerait alors le premier frame et
non la valeur du bit de parité. Est-ce bien votre intention ?

Une question reste ouverte. Page 91, l’état de gestion de parité activé par
une RFD « ne peut plus être désactivé tant que le frame n’est pas terminé ».
Les pages D9 G et H (IVM ON/OFF après la RFD) exigent pourtant qu’un IVM ON/OFF
placé après une sauvegarde VMA′ réussie, dans le même frame, ne bloque plus les
sauvegardes suivantes. Deux modèles reproduisent vos photos :

1. la gestion de parité s’arrête à la première sauvegarde VMA′ réussie ;
2. la parité est figée au moment où la RFD est déclenchée.

Lequel correspond au circuit ? Une séquence de SHAKER les distingue-t-elle déjà ?

Merci beaucoup pour ce travail et pour ces démos !

Bien cordialement,
