#!/usr/bin/env python3
"""Rebuild ONLY v1.5 corrected cat yawn/belly clips and six expanded marine resources.
Pillow is developer-only. Authored 128px key atlases retain the original project palettes.
Legacy ocean files, other cat poses and all dog/bird/panda files are never written here.
"""
import hashlib, importlib.util, json, math, struct, sys, zlib
from pathlib import Path
from PIL import Image, ImageDraw
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('ocp',ROOT/'tools/Convert-Ocean.py');ocp=importlib.util.module_from_spec(spec);spec.loader.exec_module(ocp)
spec=importlib.util.spec_from_file_location('renewed',ROOT/'tools/Build-Renewed-Pets.py');renewed=importlib.util.module_from_spec(spec);spec.loader.exec_module(renewed)
ACTIONS=('idle','blink','sleep','cuddle','angry','heart','remind','roll','bubbles','special')
SIDE=128

def decode(data):
 magic,w,h,n=struct.unpack_from('<4sHHH',data);assert magic==b'OCP1';pos=10;frames=[];delays=[]
 for _ in range(n):
  delay,colors,size=struct.unpack_from('<HHI',data,pos);pos+=8;palette=[]
  for i in range(colors):
   b,g,r,a=struct.unpack_from('<BBBB',data,pos);pos+=4;palette.append((r,g,b,a))
  end=pos+size;indices=[]
  while pos<end:
   tag=data[pos];pos+=1;count=(tag&127)+1
   if tag&128:indices.extend([data[pos]]*count);pos+=1
   else:indices.extend(data[pos:pos+count]);pos+=count
  assert len(indices)==w*h;im=Image.new('RGBA',(w,h));im.putdata([palette[i] for i in indices]);frames.append(im);delays.append(delay)
 assert pos==len(data);return frames,delays

def encode(frames,delays):
 data=bytearray(struct.pack('<4sHHH',b'OCP1',SIDE,SIDE,len(frames)));checks=[]
 for frame,delay in zip(frames,delays):
  pixels=list(frame.get_flattened_data());palette=list(dict.fromkeys(pixels));assert len(palette)<=256
  mapping={color:i for i,color in enumerate(palette)};packed=ocp.encode(bytes(mapping[p] for p in pixels));data.extend(struct.pack('<HHI',delay,len(palette),len(packed)))
  for r,g,b,a in palette:assert a in (0,255);data.extend(struct.pack('<I',a<<24|r<<16|g<<8|b))
  data.extend(packed);checks.append(zlib.crc32(frame.tobytes('raw','BGRA')))
 return data,checks

def gif(path,frames,delay):
 path.parent.mkdir(parents=True,exist_ok=True);images=[]
 for frame in frames:
  pixels=list(frame.get_flattened_data());opaque=list(dict.fromkeys(p[:3] for p in pixels if p[3]));assert len(opaque)<=255
  lookup={rgb:i for i,rgb in enumerate(opaque)};image=Image.new('P',frame.size);image.putpalette(([v for rgb in opaque for v in rgb]+[0]*768)[:768]);image.putdata([lookup[p[:3]] if p[3] else 255 for p in pixels]);images.append(image)
 images[0].save(path,save_all=True,append_images=images[1:],duration=delay,loop=0,disposal=2,transparency=255,background=255,optimize=False)
 assert path.stat().st_size<100*1024

def smooth(a,b,value):
 t=max(0,min(1,(value-a)/(b-a)));return t*t*(3-2*t)

def warp(image,index,action,frame,cat=False):
 """Nearest-neighbor inverse skinning: local tail, flippers/paws, ears and belly.
 The head/core is anchored; no duplicate layer is left underneath a moved limb.
 """
 phase=frame*math.tau/12;s=math.sin(phase);c=math.cos(phase);out=Image.new('RGBA',image.size);src=image.load();dst=out.load()
 if cat:
  for y in range(128):
   for x in range(128):
    belly=smooth(55,95,y)*(1-smooth(85,108,x));tail=smooth(88,112,x)*smooth(60,88,y)
    paw=(1-smooth(50,62,abs(x-64)))*smooth(60,76,y)*(1-smooth(88,101,y)) if action=='yawn' else smooth(75,93,y)
    ear=1-smooth(24,40,y)
    xx=round(x-s*(.7*belly+1.3*tail+1.0*paw+.6*ear));yy=round(y-c*(.7*belly+.8*paw))
    if 0<=xx<128 and 0<=yy<128:dst[x,y]=src[xx,yy]
 else:
  power=.65 if action=='sleep' else 4.5 if action in ('angry','heart','remind') else 2.2
  for y in range(128):
   for x in range(128):
    tail=(1-smooth(30,54,x))*smooth(70,99,y) if index==0 else smooth(83,109,x)
    # Fin weights taper to zero at shoulders; fins remain continuously attached.
    fin=max(0,1-((x-47)/17)**2-((y-80)/16)**2)+max(0,1-((x-76)/16)**2-((y-81)/15)**2)
    fin*=smooth(64,78,y);belly=max(0,1-((x-63)/30)**2-((y-74)/21)**2)
    xx=round(x-power*s*tail-.55*c*fin);yy=round(y-power*c*tail-(.9 if action=='sleep' else 6 if action in ('heart','remind','angry','cuddle') else 3)*s*fin-.55*c*belly)
    if 0<=xx<128 and 0<=yy<128:dst[x,y]=src[xx,yy]
 return out

def face_only(base,expression,index):
 # The atlas blink/sleep drawings supply facial expressions. Keep the body,
 # spots, fin roots and tail pixels from idle, preventing pattern flicker.
 box=(43,35,99,68) if index==0 else (20,49,69,78) if index==2 else (18,44,76,79)
 out=base.copy();mask=Image.new('L',(128,128));ImageDraw.Draw(mask).ellipse(box,fill=255);out.paste(expression,(0,0),mask);return out

def marine_frames(poses,index,action):
 pose={'idle':0,'blink':0,'sleep':2,'cuddle':3,'angry':4,'heart':5,'remind':6,'roll':7,'bubbles':8,'special':7 if index<3 else 8}[action]
 if action=='blink':base=face_only(poses[0],poses[1],index)
 else:base=poses[pose]
 frames=[]
 for f in range(12):
  source=base
  if action=='idle' and f in (8,9):source=face_only(poses[0],poses[1],index)
  if action=='blink' and f not in (3,4,5,6):source=poses[0]
  im=warp(source,index,action,f)
  if action=='roll':im=im.rotate((f*30 if index<3 else 20*math.sin(f*math.tau/12)),resample=Image.Resampling.NEAREST,center=(64,64))
  if action=='special' and index<3:im=im.rotate(-28*math.sin(f*math.tau/12),resample=Image.Resampling.NEAREST,center=(64,64))
  draw=ImageDraw.Draw(im)
  if action in ('heart','cuddle'):
   for x,y in ((17,18),(99,15)):
    y+=round(math.sin(f*math.tau/12)*2);draw.polygon([(x,y+2),(x+2,y),(x+4,y+2),(x+6,y),(x+8,y+2),(x+8,y+4),(x+4,y+8),(x,y+4)],fill=(239,136,164,255))
  if action in ('bubbles','remind') or action=='special' and index>=3:
   for i in range(3):
    age=(f+i*4)%12;x=(95 if index==0 else 15)+i*5;y=65-age*3;r=1+age//6
    if age<10:draw.ellipse((x-r,y-r,x+r,y+r),outline=(169,216,235,255),width=1)
  frames.append(im)
 return frames

def build():
 folder=ROOT/'assets/renewed';manifest=json.loads((folder/'manifest.json').read_text());source=folder/'source/cat-corrections.png';atlas=Image.open(source).convert('RGBA')
 for coat in range(5):
  record=manifest[coat];path=folder/record['file'];poses,delays=decode(path.read_bytes());poses=poses[:6];delays=delays[:6]
  for row,pose in enumerate((3,5)):poses[pose]=atlas.crop((coat*128,row*128,(coat+1)*128,(row+1)*128))
  generated=[]
  for action,pose in (('yawn',3),('belly',5)):
   clip=[warp(poses[pose],coat,action,f,cat=True) for f in range(12)];generated+=clip;gif(folder/'gifs'/f'pet-0-{coat}'/(action+'.gif'),clip,100)
  data,checks=encode(poses+generated,delays+[100]*24);path.write_bytes(data);record.update(argbCRC32=checks,bytes=len(data),correctionSource='source/cat-corrections.png',correctionSourceSHA256=hashlib.sha256(source.read_bytes()).hexdigest(),animationClips={'yawn':[6,12,100],'belly':[18,12,100]})
 (folder/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
 folder=ROOT/'assets/ocean';records=[]
 for index in range(6):
  source=folder/f'source/marine-{index}.png';atlas=Image.open(source).convert('RGBA');poses=[atlas.crop((p%4*128,p//4*128,p%4*128+128,p//4*128+128)) for p in range(12)];images=[];delays=[];clips={}
  for action in ACTIONS:
   clip=marine_frames(poses,index,action);delay=(400 if index<3 else 600) if action=='sleep' else 150 if action=='remind' or index>=3 else 100
   clips[action]=[len(images),12,delay];images+=clip;delays.extend([delay]*12);gif(folder/'gifs'/f'marine-{index}'/(action+'.gif'),clip,delay)
  data,checks=encode(images,delays);path=folder/f'marine-{index}.ocp';path.write_bytes(data);records.append(dict(pet=5 if index<3 else 6,coat=index%3,resource=201+index,file=path.name,source=str(source.relative_to(folder)),sourceSHA256=hashlib.sha256(source.read_bytes()).hexdigest(),width=128,height=128,frames=len(images),clips=clips,argbCRC32=checks,bytes=len(data),decodedBytes=len(images)*128*128*4,legacyFile=f'ocean-{index}.ocp'))
 (folder/'animations.json').write_text(json.dumps(dict(schema=1,framesPerClip=12,actions=ACTIONS,pets=records),ensure_ascii=False,indent=2)+'\n')
 print('Updated only five cat OCPs / ten cat GIFs; built six marine OCPs / sixty GIFs.')
 print('Marine embedded bytes:',sum(r['bytes'] for r in records),'active ARGB bytes:',max(r['decodedBytes'] for r in records))
if __name__=='__main__':build()
