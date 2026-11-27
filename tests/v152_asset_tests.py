"""Source/resource integrity checks, not a substitute for Windows GUI tests."""
import hashlib, struct, zlib, re
from pathlib import Path
root=Path(__file__).resolve().parent.parent
icon=(root/'assets/icons/pet.ico').read_bytes()
assert hashlib.sha256(icon).hexdigest()=='9c988a3f90eeaf4a7aa7c9d0d9d18cd04216becaccce0f5ea011bfcf62627c10', 'brand ICO differs from the supplied original'
assert struct.unpack_from('<HHH',icon)==(0,1,9)
sizes=[]
for index in range(9):
    width,height,colors,reserved,planes,bits,length,offset=struct.unpack_from('<BBBBHHII',icon,6+index*16)
    width,height=width or 256,height or 256;sizes.append(width)
    assert width==height and planes==1 and bits==32 and length and offset>=6+9*16 and offset+length<=len(icon)
    payload=icon[offset:offset+length]
    assert payload[:8]==b'\x89PNG\r\n\x1a\n'
    pos=8;ended=False
    while pos<len(payload):
        n=struct.unpack_from('>I',payload,pos)[0];tag=payload[pos+4:pos+8];data=payload[pos+8:pos+8+n]
        assert pos+12+n<=len(payload)
        assert zlib.crc32(tag+data)&0xffffffff==struct.unpack_from('>I',payload,pos+8+n)[0]
        if tag==b'IHDR':
            w,h,depth,color=struct.unpack_from('>IIBB',data)
            assert (w,h)==(width,height) and depth==8 and color==6, 'RGBA transparency/aspect must remain intact'
        pos+=12+n
        if tag==b'IEND':ended=True;break
    assert ended and pos==len(payload)
assert sizes==[16,20,24,32,40,48,64,128,256]
main=(root/'src/main.cpp').read_text();sprite=(root/'src/sprite.h').read_text();review=(root/'tests/animation_tests.cpp').read_text()
for text in [main,sprite,review]:
    assert not re.search(r'\bSprite\s+effect\b|\.heart\(|void\s+heart\(|extraEmote',text), 'runtime decoration path survived'
assert 'speech=' in main and 'UpdateLayeredWindow' in main and 'spring.step' in main and 'nameplate.place' in main
assert 'reminderTask().text.empty()' in main and 'c.timers.tick(now())' in main
assert 'SetWinEventHook(EVENT_SYSTEM_FOREGROUND' in main and 'SWP_NOACTIVATE' in main
assert 'foreground==previousForeground&&monitor==previousPetMonitor&&previousTop==c.top' in main
assert 'fullscreenOnPetMonitor' in main and 'WINEVENT_SKIPOWNPROCESS' in main
assert 'c.skip' not in main and 'skipButton' not in (root/'src/update_window.h').read_text()
for source in ['main.cpp','form.h','scale_window.h','timer_window.h','update_window.h','about_window.h']:
    assert 'brandWindow(' in (root/'src'/source).read_text(),source
assert 'brandIcon(px(48))' in (root/'src/about_window.h').read_text()
assert 'brandWindow(' in (root/'updater/main.cpp').read_text()
assert 'save(root/"assets/icons/pet.ico"' not in (root/'tools/export_assets.cpp').read_text()
cmake=(root/'CMakeLists.txt').read_text()
assert 'project(DesktopPet VERSION 1.5.2' in cmake and 'v152_modes_editors' in cmake and 'v152_native_ui' in cmake
assert '1 ICON "pet.ico"' in (root/'src/pet.rc.in').read_text()
print('PASS: original nine-size RGBA brand ICO hash/PNG CRC; runtime overlay removal; native icon routes; foreground-event gate; v1.5.2 test registration (Windows GUI not executed)')
