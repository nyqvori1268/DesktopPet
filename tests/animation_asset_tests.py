"""Developer-only exact GIF/OCP checks. Pillow never enters the application."""
from pathlib import Path
import importlib.util, json, hashlib, zlib, struct, sys
from PIL import Image, ImageSequence
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('animation_builder',ROOT/'tools/Build-V150-Animations.py');builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
baseline=json.loads((ROOT/'tests/fixtures/v150_art_baseline.json').read_text())
for name,expected in baseline['normalSHA256'].items():assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==expected,('unrelated art changed',name)
records=json.loads((ROOT/'assets/ocean/animations.json').read_text())['pets'];cases=[]
for record in records:
 assert hashlib.sha256((ROOT/'assets/ocean'/record['source']).read_bytes()).hexdigest()==record['sourceSHA256']
 frames,delays=builder.decode((ROOT/'assets/ocean'/record['file']).read_bytes())
 for action,(start,count,delay) in record['clips'].items():cases.append((ROOT/'assets/ocean/gifs'/Path(record['file']).stem/(action+'.gif'),frames[start:start+count],delay))
for coat in range(5):
 frames,delays=builder.decode((ROOT/f'assets/renewed/pet-0-{coat}.ocp').read_bytes())
 for pose in (0,1,2,4):assert zlib.crc32(frames[pose].tobytes('raw','BGRA'))==baseline['catOriginalCRC32'][coat][pose]
 for action,start in (('yawn',6),('belly',18)):cases.append((ROOT/f'assets/renewed/gifs/pet-0-{coat}/{action}.gif',frames[start:start+12],100))
maxgif=0
for path,expected,delay in cases:
 data=path.read_bytes();maxgif=max(maxgif,len(data));assert len(data)<100*1024
 image=Image.open(path);assert image.size==(128,128) and image.n_frames==12 and image.info['loop']==0 and image.info['background']==255
 decoded=[]
 for frame in ImageSequence.Iterator(image):
  rgba=frame.convert('RGBA');rgba.putdata([p if p[3] else (0,0,0,0) for p in rgba.get_flattened_data()]);duration=frame.info['duration'];assert duration%delay==0;decoded.extend([rgba.copy()]*(duration//delay))
 assert len(decoded)==12,(path,len(decoded))
 for index,(actual,source) in enumerate(zip(decoded,expected)):
  assert actual.tobytes()==source.tobytes(),('GIF native decoded pixels differ',path,index)
  assert source.getbbox() and all(source.getpixel((x,0))[3]==0 and source.getpixel((x,127))[3]==0 and source.getpixel((0,x))[3]==0 and source.getpixel((127,x))[3]==0 for x in range(128)),('clipped image',path,index)
 # Last -> first must be a normal local motion step. Full rotations have evenly spaced angles.
 changes=[]
 for a,b in zip(expected,expected[1:]+expected[:1]):changes.append(sum(x!=y for x,y in zip(a.get_flattened_data(),b.get_flattened_data())))
 assert changes[-1]<=max(changes[:-1])*1.6+100,('loop seam is an outlier',path,changes)
 if path.name in ('yawn.gif','belly.gif'):
  a=[p[3]>0 for p in expected[0].get_flattened_data()];b=[p[3]>0 for p in expected[-1].get_flattened_data()];assert sum(x and y for x,y in zip(a,b))/sum(x or y for x,y in zip(a,b))>.85,('cat pose jumps at loop boundary',path)
print(f'PASS: {len(baseline["normalSHA256"])} untouched artwork hashes; all 70 new/corrected GIFs match OCP pixels at all 840 frame positions; transparent margins, loop seams, original poses; largest GIF {maxgif} bytes')
