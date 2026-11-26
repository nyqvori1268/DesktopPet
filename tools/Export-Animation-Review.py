#!/usr/bin/env python3
"""Developer-only GIF gallery from checked-in source clips and shared native sampler.
First run animation_tests --export <raw-directory>. No network or app dependency.
"""
from pathlib import Path
import argparse,html,importlib.util,shutil,struct,sys
from PIL import Image,ImageDraw,ImageSequence
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('animation_builder',ROOT/'tools/Build-V150-Animations.py');builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
GROUPS=[(f'land-0-{i}',name,ROOT/f'assets/renewed/gifs/pet-0-{i}') for i,name in enumerate(['橘貓','三花貓','白貓','黑貓','虎斑貓'])]+[(f'land-1-{i}',name,ROOT/f'assets/renewed/gifs/pet-1-{i}') for i,name in enumerate(['赤柴','黑柴','白柴','柯基','米色垂耳犬'])]+[(f'land-3-{i}',name,ROOT/f'assets/renewed/gifs/pet-3-{i}') for i,name in enumerate(['綠色虎皮鸚鵡','藍色虎皮鸚鵡','黃色虎皮鸚鵡','灰色玄鳳','奶黃色玄鳳'])]+[('land-4-0','熊貓',ROOT/'assets/renewed/gifs/pet-4-0')]+[(f'marine-{i}',name,ROOT/f'assets/ocean/gifs/marine-{i}') for i,name in enumerate(['經典藍海豚','粉紅海豚','灰藍海豚','深藍鯨鯊','淺藍鯨鯊','灰藍鯨鯊'])]
LAND=('idle','blink','sleep','yawn','stretch','belly','happy','pet')
LABELS=dict(zip(LAND,['待機','眨眼','睡眠','哈欠','伸展','翻肚／拍翅','開心','撫摸']))|dict(cuddle='撒嬌',angry='生氣',heart='愛心',remind='提醒',roll='轉圈／悠閒翻身',bubbles='吐泡泡',special='俏皮搖擺／濾食')
def samples(path):
 image=Image.open(path);frames=[];times=[]
 for frame in ImageSequence.Iterator(image):frames.append(frame.convert('RGBA').copy());times.append(frame.info.get('duration',100))
 selected=[];total=sum(times)
 for f in range(12):
  t=f*total/12
  for frame,time in zip(frames,times):
   if t<time:selected.append(frame);break
   t-=time
 return selected

def build(raw,output):
 output.mkdir(parents=True,exist_ok=True);sections=[];count=0
 for group,name,folder in GROUPS:
  actions=builder.ACTIONS if group.startswith('marine') else LAND;cards=[];contact=Image.new('RGBA',(12*88+88,len(actions)*102),(215,218,225,255));draw=ImageDraw.Draw(contact)
  for row,action in enumerate(actions):
   source=output/'source-gifs'/group/(action+'.gif');source.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(folder/(action+'.gif'),source)
   data=(raw/group/(action+'.raw')).read_bytes();w,h,n,delay=struct.unpack_from('<IIII',data);assert w==h==144 and n==12 and len(data)==16+w*h*n*4
   frames=[Image.frombytes('RGBA',(w,h),data[16+f*w*h*4:16+(f+1)*w*h*4],'raw','BGRA') for f in range(n)];native=output/'native-sampler'/group/(action+'.gif');builder.gif(native,frames,delay);count+=1
   draw.text((3,row*102+26),action,fill='black')
   for col,image in enumerate(frames):contact.alpha_composite(image.resize((88,88),Image.Resampling.NEAREST),(88+col*88,row*102));draw.text((90+col*88,row*102+89),str(col),fill='black')
   cards.append(f'<article><h3>{html.escape(LABELS[action])} · {action}</h3><figure><img src="source-gifs/{group}/{action}.gif" width="128" height="128"><figcaption>128 px 來源 GIF</figcaption></figure><figure><img src="native-sampler/{group}/{action}.gif" width="144" height="144"><figcaption>共用原生取樣／預設幾何</figcaption></figure></article>')
  draw=ImageDraw.Draw(contact)
  for row,action in enumerate(actions):
   draw.text((3,row*102+26),action,fill='black')
   for col in range(12):draw.text((90+col*88,row*102+89),str(col),fill='black')
  (output/'contact-sheets').mkdir(exist_ok=True);contact.save(output/'contact-sheets'/(group+'.png'));sections.append(f'<details {"open" if group=="land-0-0" else ""}><summary>{html.escape(name)} ({group})</summary><div class="clips">'+''.join(cards)+'</div></details>')
 (output/'index.html').write_text('''<!doctype html><html lang="zh-Hant"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>DesktopPet 1.5.0 動畫驗收</title><style>body{max-width:1400px;margin:24px auto;padding:0 20px;font:16px system-ui;color:#283046;background:#f2f4f8}details{margin:16px 0;padding:16px;border-radius:14px;background:white}summary{font-size:20px;cursor:pointer}.clips{display:flex;flex-wrap:wrap;gap:14px}article{border:1px solid #c9cfdb;border-radius:12px;padding:12px}h3{font-size:16px}figure{display:inline-block;margin:6px;text-align:center}img{image-rendering:pixelated;background:repeating-conic-gradient(#e1e4e9 0% 25%,#fafafa 0% 50%) 0/16px 16px}figcaption{font-size:12px}</style><h1>DesktopPet 1.5.0 GIF 動畫驗收</h1><p>22 外觀，各段可直接播放。來源與離屏原生取樣為不同檔案；不是靜態拼圖。打開物種區段檢視，其他區段可收合。</p><p>native-sampler 由 C++ 主程式共用的 blitNativePet 與真實動作路由產生，使用預設 96 邏輯像素／144 畫布、一般效果與提醒跳躍；是隔離動作的離屏預覽，並非 Windows 視窗、DPI、透明度或狀態機完整實測。Windows 實機驗證仍待執行。</p>'''+''.join(sections)+'</html>',encoding='utf-8')
 (output/'README.txt').write_text('DesktopPet 1.5.0 animation review\nOpen index.html offline.\nsource-gifs: 128px original clips; all 22 appearances.\nnative-sampler: 144px offscreen C++ shared sampler and isolated runtime action routing.\ncontact-sheets: all twelve native preview frames, for stepwise review.\nNot a Windows GUI, DPI, CPU, memory or EXE size acceptance result.\n',encoding='utf-8')
 print(f'Exported {count} source GIFs + {count} native-sampler GIFs and 22 contact sheets.')
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('native_raw',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args();build(args.native_raw,args.output)
