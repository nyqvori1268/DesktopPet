#!/usr/bin/env python3
"""Developer-only lossless GIF -> indexed PackBits resources (Pillow required).
No resizing, recoloring, or frame-by-frame crop; a common union crop prevents jitter.
Only tiny disconnected alpha islands (< max(12, largest/2000) pixels) are removed.
Usage: python tools/Convert-Ocean.py input.zip [output-directory]
"""
import hashlib, io, json, struct, sys, zipfile, zlib
from collections import deque
from pathlib import Path
from PIL import Image, ImageSequence
NAMES = ['A1_Dolphin_ClassicBlue.gif','A2_Dolphin_Pink.gif','A3_Dolphin_GrayBlue.gif',
         'B1_WhaleShark_DeepBlue.gif','B2_WhaleShark_LightBlue.gif','B3_WhaleShark_GrayBlue.gif']
def clean(image):
    image=image.convert('RGBA');w,h=image.size;pixels=list(image.get_flattened_data() if hasattr(image,"get_flattened_data") else image.getdata());seen=bytearray(w*h);groups=[]
    for start,pixel in enumerate(pixels):
        if not pixel[3] or seen[start]:continue
        seen[start]=1;todo=deque([start]);group=[]
        while todo:
            pos=todo.popleft();group.append(pos);x,y=pos%w,pos//w
            for yy in range(max(0,y-1),min(h,y+2)):
                for xx in range(max(0,x-1),min(w,x+2)):
                    n=yy*w+xx
                    if pixels[n][3] and not seen[n]:seen[n]=1;todo.append(n)
        groups.append(group)
    threshold=max(12,max(map(len,groups),default=0)//2000);removed=0
    for group in groups:
        if len(group)<threshold:
            for pos in group:pixels[pos]=(0,0,0,0);removed+=1
    # Transparent palette RGB values have no visible meaning.
    pixels=[pixel if pixel[3] else (0,0,0,0) for pixel in pixels]
    image.putdata(pixels);return image,removed

def encode(indices):
    out=bytearray();i=0
    while i<len(indices):
        run=1
        while i+run<len(indices) and run<128 and indices[i+run]==indices[i]:run+=1
        if run>=3:out.extend((127+run,indices[i]));i+=run;continue
        start=i;i+=run
        while i<len(indices) and i-start<128:
            run=1
            while i+run<len(indices) and run<128 and indices[i+run]==indices[i]:run+=1
            if run>=3:break
            i+=min(run,128-(i-start))
        out.append(i-start-1);out.extend(indices[start:i])
    return out

def convert(source,output):
    output.mkdir(parents=True,exist_ok=True);records=[]
    with zipfile.ZipFile(source) as archive:
        for index,name in enumerate(NAMES):
            raw=archive.read(name);image=Image.open(io.BytesIO(raw));frames=[];delays=[];removed=[]
            for frame in ImageSequence.Iterator(image):
                delays.append(frame.info.get('duration',180));cleaned,count=clean(frame);frames.append(cleaned);removed.append(count)
            boxes=[frame.getbbox() for frame in frames];crop=(min(b[0] for b in boxes),min(b[1] for b in boxes),max(b[2] for b in boxes),max(b[3] for b in boxes))
            w,h=crop[2]-crop[0],crop[3]-crop[1];data=bytearray(struct.pack('<4sHHH',b'OCP1',w,h,len(frames)))
            hashes=[];crcs=[]
            for frame,delay in zip(frames,delays):
                frame=frame.crop(crop);rgba=list(frame.get_flattened_data() if hasattr(frame,"get_flattened_data") else frame.getdata());palette=list(dict.fromkeys(rgba));assert len(palette)<=256
                table={color:i for i,color in enumerate(palette)};indices=bytes(table[color] for color in rgba);packed=encode(indices)
                data.extend(struct.pack('<HHI',delay,len(palette),len(packed)))
                for red,green,blue,alpha in palette:
                    # GIF alpha is binary; output is premultiplied ARGB for Win32.
                    data.extend(struct.pack('<I',(alpha<<24)|((red*alpha//255)<<16)|((green*alpha//255)<<8)|(blue*alpha//255)))
                data.extend(packed);hashes.append(hashlib.sha256(frame.tobytes()).hexdigest());crcs.append(zlib.crc32(frame.tobytes("raw","BGRA")))
            target=output/f'ocean-{index}.ocp';target.write_bytes(data)
            records.append(dict(source=name,sha256=hashlib.sha256(raw).hexdigest(),resource=target.name,sourceSize=list(image.size),crop=list(crop),size=[w,h],frames=len(frames),delays=delays,removedPixels=removed,rgbaSHA256=hashes,argbCRC32=crcs,bytes=len(data)))
            print(target.name,w,h,len(data))
    (output/'provenance.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':convert(Path(sys.argv[1]),Path(sys.argv[2]) if len(sys.argv)>2 else Path(__file__).resolve().parent.parent/'assets/ocean')
