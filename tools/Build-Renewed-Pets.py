#!/usr/bin/env python3
"""Build original generated pose atlases into indexed native resources and transparent GIFs.
Developer-only Pillow; uses the existing, unchanged OCP encoder/decoder protocol.
No palette-index/configuration changes. Re-run from any directory; deterministic output.
"""
import hashlib, importlib.util, json, math, struct, zlib, sys
from collections import deque
from pathlib import Path
from PIL import Image
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('ocean_encoder',ROOT/'tools/Convert-Ocean.py')
encoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(encoder)
SIDE=128
GROUPS=[('cats',0,5,6),('dogs',1,5,6),('birds',3,5,6),('panda',4,2,3)]
ACTIONS=['idle','blink','sleep','yawn','stretch','belly','happy','pet']
def atlas_tiles(atlas,columns,rows):
    # Use complete alpha components, not hard grid cuts: ears/tails/crests may
    # cross a nominal cell boundary in generated source sheets.
    atlas=atlas.convert('RGBA');w,h=atlas.size;raw=list(atlas.get_flattened_data())
    pixels=[(r,g,b,255) if a>=240 else (0,0,0,0) for r,g,b,a in raw]
    seen=bytearray(w*h);chosen={}
    for start,color in enumerate(pixels):
        if not color[3] or seen[start]:continue
        todo=deque([start]);seen[start]=1;component=[]
        while todo:
            pos=todo.popleft();component.append(pos);x,y=pos%w,pos//w
            for yy in range(max(0,y-1),min(h,y+2)):
                for xx in range(max(0,x-1),min(w,x+2)):
                    index=yy*w+xx
                    if pixels[index][3] and not seen[index]:seen[index]=1;todo.append(index)
        if len(component)<100:continue
        cx=sum(pos%w for pos in component)/len(component);cy=sum(pos//w for pos in component)/len(component)
        key=(min(columns-1,int(cx*columns/w)),min(rows-1,int(cy*rows/h)))
        if len(component)>len(chosen.get(key,[])):chosen[key]=component
    result={}
    for row in range(rows):
        for col in range(columns):
            component=chosen[(col,row)];left=min(pos%w for pos in component);right=max(pos%w for pos in component)+1;top=min(pos//w for pos in component);bottom=max(pos//w for pos in component)+1
            out=Image.new('RGBA',(right-left,bottom-top));data=[(0,0,0,0)]*((right-left)*(bottom-top))
            for pos in component:data[(pos//w-top)*(right-left)+(pos%w-left)]=pixels[pos]
            out.putdata(data);result[(col,row)]=out
    return result

def motion(poses,action,frame):
    t=frame/12*math.tau
    pose={'blink':1,'sleep':2,'yawn':3,'stretch':4,'belly':5,'happy':1,'pet':1}.get(action,0)
    if action=='idle' and frame==10:pose=1
    if action=='blink':pose=1 if 3<=frame<=5 else 0
    if action=='yawn':pose=3 if 3<=frame<=8 else 1 if frame in (2,9) else 0
    if action=='stretch':pose=4 if 2<=frame<=9 else 0
    if action=='belly':pose=5 if 2<=frame<=9 else 0
    image=poses[pose]
    height=SIDE+round(math.sin(t)*(1 if action=='sleep' else 2));width=SIDE+round(math.sin(t)*(1 if action in ('happy','pet') else 0))
    body=image.resize((width,height),Image.Resampling.NEAREST)
    canvas=Image.new('RGBA',(SIDE,SIDE));canvas.alpha_composite(body,((SIDE-width)//2+(0 if action=='sleep' else round(math.sin(t)*1.5)),SIDE-height))
    if action in ('happy','pet'):
        # Same small heart glyph as native Sprite::heart, no decoration on the pet itself.
        from PIL import ImageDraw
        d=ImageDraw.Draw(canvas);x,y=105,9+round(math.sin(t)*2)
        d.polygon([(x,y+3),(x+3,y),(x+6,y),(x+9,y+3),(x+12,y),(x+15,y),(x+18,y+3),(x+18,y+7),(x+9,y+15),(x,y+7)],fill=(239,136,164,255))
    return canvas

def gif_frame(image):
    rgb=image.convert('RGB').quantize(colors=255,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
    mask=image.getchannel('A').point(lambda a:255 if a<128 else 0)
    rgb.paste(255,mask);palette=rgb.getpalette() or [];palette=(palette+[0]*768)[:768];palette[765:768]=[0,0,0];rgb.putpalette(palette);rgb.info['transparency']=255
    return rgb

def build():
    folder=ROOT/'assets/renewed';records=[];thumbs=[];action_rows=[]
    for group,pet,columns,rows in GROUPS:
        source=folder/'source'/(group+'.png');atlas=Image.open(source).convert('RGBA');extracted=atlas_tiles(atlas,columns,rows)
        for coat in range(1 if pet==4 else 5):
            tiles=[]
            for pose in range(6):
                col,row=(pose%2,pose//2) if pet==4 else (coat,pose)
                tiles.append(extracted[(col,row)])
            ratio=116/max(max(tile.size) for tile in tiles)
            poses=[]
            for tile in tiles:
                tile=tile.resize((max(1,round(tile.width*ratio)),max(1,round(tile.height*ratio))),Image.Resampling.NEAREST)
                frame=Image.new('RGBA',(SIDE,SIDE));frame.alpha_composite(tile,((SIDE-tile.width)//2,122-tile.height))
                # A bounded palette is shared by native rendering and the GIF source poses.
                quant=frame.quantize(colors=128,method=Image.Quantize.FASTOCTREE,dither=Image.Dither.NONE).convert('RGBA')
                quant.putalpha(frame.getchannel('A'));quant.putdata([p if p[3] else (0,0,0,0) for p in quant.get_flattened_data()]);poses.append(quant)
            data=bytearray(struct.pack('<4sHHH',b'OCP1',SIDE,SIDE,6));checks=[]
            for frame in poses:
                pixels=[p if p[3] else (0,0,0,0) for p in frame.get_flattened_data()];palette=list(dict.fromkeys(pixels));assert len(palette)<=256
                mapping={color:i for i,color in enumerate(palette)};encoded=encoder.encode(bytes(mapping[color] for color in pixels))
                data.extend(struct.pack('<HHI',180,len(palette),len(encoded)))
                for r,g,b,a in palette:data.extend(struct.pack('<I',a<<24|r<<16|g<<8|b))
                data.extend(encoded);checks.append(zlib.crc32(frame.tobytes('raw','BGRA')))
            name=f'pet-{pet}-{coat}';(folder/(name+'.ocp')).write_bytes(data)
            out=folder/'gifs'/name;out.mkdir(parents=True,exist_ok=True)
            for action in ACTIONS:
                actual=action
                images=[gif_frame(motion(poses,actual,f)) for f in range(12)]
                images[0].save(out/(action+'.gif'),save_all=True,append_images=images[1:],duration=100 if action!='sleep' else 200,loop=0,disposal=2,transparency=255,background=255,optimize=False)
            records.append(dict(pet=pet,coat=coat,resource=301+len(records),file=name+'.ocp',source='source/'+group+'.png',sourceSHA256=hashlib.sha256(source.read_bytes()).hexdigest(),width=SIDE,height=SIDE,poses=['idle','blink','sleep','yawn','stretch','wing-flap' if pet==3 else 'belly'],argbCRC32=checks,bytes=len(data)))
            thumbs.append(poses[0])
            if (pet,coat) in ((0,0),(1,3),(3,3),(4,0)):action_rows.append(poses)
    (folder/'manifest.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    # Review at the actual 96 logical-pixel size; no scaling tricks in the preview.
    preview=Image.new('RGBA',(5*128,4*128),(240,237,232,255))
    for i,im in enumerate(thumbs):
        x,y=(i%5)*128,(i//5)*128
        preview.alpha_composite(im.resize((96,96),Image.Resampling.NEAREST),(x+16,y+16))
    (ROOT/'docs').mkdir(exist_ok=True);preview.save(ROOT/'docs/renewed-pets.png')
    actions=Image.new('RGBA',(6*128,4*128),(240,237,232,255))
    for row,poses in enumerate(action_rows):
        for col,action in enumerate(ACTIONS[:6]):actions.alpha_composite(motion(poses,action,4).resize((96,96),Image.Resampling.NEAREST),(col*128+16,row*128+16))
    actions.save(ROOT/'docs/renewed-actions.png')
    print(f'Built {len(records)} original coat-compatible appearances, {len(records)*8} GIF clips, {sum(r["bytes"] for r in records)} embedded bytes')
if __name__=='__main__':build()
