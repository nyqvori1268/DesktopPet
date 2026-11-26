# Original atlas generation specification

Generated with the built-in image generation tool for this task, with transparent_background=true. No online artwork or external character design was used. The retained PNGs are the original generation outputs; the deterministic build script performs alpha cleanup, tile extraction and palette conversion afterward.

Shared specification: Production-ready original Japanese kawaii pixel-art desktop pet sprite atlas. True transparent background; no text, labels, grid lines, borders, panels, scenery or cast shadows. Rounded chibi proportions, expressive glossy eyes with two tiny highlights, warm blush, clustered crisp pixels and soft multi-tone pixel shading, matching a premium cute dolphin mascot. Complete characters within regular cells, consistent character designs, scale and baseline across poses; no costumes or collars.

## cats.png

5 columns × 6 rows. Columns: orange striped cat with cream muzzle/belly; white calico with orange and charcoal patches; snowy white cat; charcoal black cat with visible eye highlights; gray-brown striped tabby. Pointed ears, short cat muzzle, paw pads, curved expressive tail. Rows: seated alert idle; same body blinking; curled-up asleep; seated yawn with paws near muzzle; forward stretching with forepaws extended and rear raised; belly up showing four paw pads. Preserve each exact coat across all six poses.

## dogs.png

5 columns × 6 rows. Columns: red-tan Shiba with cream muzzle/curled tail; black Shiba with tan eyebrows/cream muzzle/curled tail; white Shiba; orange-cream corgi with very short legs, big upright ears and long low body; beige dog with genuinely floppy rounded ears. Rows: seated idle; blinking; curled asleep; yawn; forward play bow; tummy-up with four paw pads. Keep each breed silhouette recognizably distinct across all rows.

## birds.png

5 columns × 6 rows. Columns: green/yellow-head barred budgie; blue/white-head barred budgie; yellow budgie; gray cockatiel with pale-yellow head, tall crest and orange cheeks; creamy pale-yellow cockatiel with crest and orange cheeks. Curved beak, two wings, long tail feathers, two tiny feet, no mammal limbs. Rows: standing wings-folded idle; blinking; head nestled into shoulder feathers asleep; small open-beak yawn; spread-wing stretch/tail fan; happy grounded flap and head tilt. No belly-up birds or perches.

## panda.png

2 columns × 3 rows, read left to right/top to bottom. One consistent black-white baby giant panda with round black ears, dark eye patches, white round face and chubby belly. Poses: seated idle; same body blinking; curled asleep; seated yawn/paws near muzzle; forward stretch; belly up with four round black paws visible. Luxurious rounded Q-version proportions and expressive face; no cat/dog/fox features.

Source SHA-256 and the preserved pet/coat index mapping are recorded in ../manifest.json. Source PNGs contain generation antialiasing; exported GIF/runtime resources deliberately use binary transparency and nearest-neighbor scaling to preserve pixel edges.
