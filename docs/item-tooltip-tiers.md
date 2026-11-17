# Tier en el tooltip

La franja muestra RAREZA - TIER N, conservando los colores de la rareza.
La rareza depende de las opciones Excellent: 0 común, 1 raro, 2 épico,
3 o más legendario. Skill, Luck, Life, nivel y Ancient no aumentan la rareza.
El tier identifica el set/ítem y permanece igual para todas sus rarezas.
Dragon Knight es Tier 7; Titan y Brave son Tier 8.

Data/Custom/ParisItemTiers.json contiene la clasificación fija por ItemIndex
(grupo × 512 + número). Se carga al iniciar; reiniciar el cliente tras editar.
Un ítem sin clasificación conserva su rareza, sin inventar un tier.
La tabla procede de la clasificación del servidor Paris y usa el tier del
set (la variante más alta de su clasificación anterior normal/Excellent).
No modifica daño, defensa, drops ni opciones. El archivo debe acompañar al EXE.
Las actualizaciones de rareza enviadas por el servidor no borran esta tabla.
