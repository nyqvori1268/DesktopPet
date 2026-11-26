"""Optional developer check; Pillow is not required by DesktopPet or CMake builds."""
import hashlib, json
from pathlib import Path
from PIL import Image, ImageSequence
ROOT=Path(__file__).resolve().parent.parent
folder=ROOT/'assets/renewed';records=json.loads((folder/'manifest.json').read_text())
assert len(records)==16
for record in records:
    assert hashlib.sha256((folder/record['source']).read_bytes()).hexdigest()==record['sourceSHA256']
    directory=folder/'gifs'/Path(record['file']).stem
    for action in ('idle','blink','sleep','yawn','stretch','belly','happy','pet'):
        image=Image.open(directory/(action+'.gif'))
        assert image.size==(128,128) and image.info.get('loop')==0 and image.info.get('background')==255
        assert image.n_frames>=2
        total=0
        for frame in ImageSequence.Iterator(image):
            total+=frame.info['duration'];rgba=frame.convert('RGBA')
            assert rgba.getpixel((0,0))[3]==0 and rgba.getpixel((127,127))[3]==0
            box=rgba.getbbox();assert box and box[0]>0 and box[1]>0 and box[2]<128 and box[3]<128
        assert total==(2400 if action=='sleep' else 1200)
print('PASS: 128 transparent, looped GIF clips; frame bounds, timing, all 16 coat indices and source hashes')
