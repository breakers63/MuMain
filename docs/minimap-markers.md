# Minimap markers

Every map uses the same marker colors: green for groups of monsters, purple for individual monsters, and blue for NPCs and guards. Portals retain their original arrow. Hovering a marker displays its name and, for monsters, configured quantity.

Markers follow the existing map rotation and world coordinates; resizing the window uses the existing map projection and clipping. Colors and circular borders are defined in the legacy and modern minimap styles; custom themes inherit these unless they override them.

The per-map Data/WorldN/minimap_markers.txt files are exported from the server configuration. Regenerate them when spawn locations change. The marker is placed on a valid tile near the center of its spawn area, not on a monster's live position. Event maps show configured locations, not whether the event is currently active. An absent or invalid export retains the original marker list. Up to 2048 markers, including preserved portals, can be shown per map.
