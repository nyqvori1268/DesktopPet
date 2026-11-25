#include "custom_pet.h"
#include <objidl.h>
#include <gdiplus.h>
#include <memory>
#include <cstring>
namespace pet {
fs::path customDirectory(){auto p=appData()/L"custom-pets";fs::create_directories(p);return p;}
ImageClip decodeCustom(const fs::path&file){
 // GDI+ lives only during import/startup; the animation uses cached PARGB pixels.
 struct Runtime{ULONG_PTR token=0;Runtime(){Gdiplus::GdiplusStartupInput input;if(Gdiplus::GdiplusStartup(&token,&input,nullptr)!=Gdiplus::Ok)throw std::runtime_error("影像解碼器無法啟動");}~Runtime(){Gdiplus::GdiplusShutdown(token);}} runtime;
 auto bytes=readBytes(file,8*1024*1024);auto header=inspectImage(bytes);
 HGLOBAL global=GlobalAlloc(GMEM_MOVEABLE,bytes.size());if(!global)throw std::runtime_error("素材記憶體不足");
 void*locked=GlobalLock(global);if(!locked){GlobalFree(global);throw std::runtime_error("素材記憶體不足");}memcpy(locked,bytes.data(),bytes.size());GlobalUnlock(global);
 IStream*raw=nullptr;if(FAILED(CreateStreamOnHGlobal(global,TRUE,&raw))){GlobalFree(global);throw std::runtime_error("無法讀取素材");}
 struct Release{void operator()(IStream*p)const{if(p)p->Release();}};std::unique_ptr<IStream,Release>stream(raw);
 std::unique_ptr<Gdiplus::Image> image(Gdiplus::Image::FromStream(raw,FALSE));
 if(!image||image->GetLastStatus()!=Gdiplus::Ok)throw std::runtime_error("無法解碼 PNG／GIF");
 GUID format{};if(image->GetRawFormat(&format)!=Gdiplus::Ok||(!IsEqualGUID(format,Gdiplus::ImageFormatPNG)&&!IsEqualGUID(format,Gdiplus::ImageFormatGIF)))throw std::runtime_error("僅接受 PNG／GIF 實際格式");
 UINT w=image->GetWidth(),h=image->GetHeight();if(!w||!h||w!=header.width||h!=header.height)throw std::runtime_error("素材尺寸需為 1～2048 像素");
 UINT count=1;GUID dimension=Gdiplus::FrameDimensionTime;
 if(IsEqualGUID(format,Gdiplus::ImageFormatGIF))count=image->GetFrameCount(&dimension);
 if(count!=header.frames)throw std::runtime_error("GIF 影格資訊不一致");
 decodeSide(count);
 ImageClip result;result.width=int(w);result.height=int(h);result.frames.reserve(count);
 std::vector<unsigned char> properties;Gdiplus::PropertyItem*delays=nullptr;
 UINT propertySize=image->GetPropertyItemSize(PropertyTagFrameDelay);
 if(propertySize&&propertySize<=65536){properties.resize(propertySize);auto p=reinterpret_cast<Gdiplus::PropertyItem*>(properties.data());if(image->GetPropertyItem(PropertyTagFrameDelay,propertySize,p)==Gdiplus::Ok&&p->value&&p->type==PropertyTagTypeLong&&p->length>=count*sizeof(UINT))delays=p;}
 for(UINT index=0;index<count;++index){
  if(count>1&&image->SelectActiveFrame(&dimension,index)!=Gdiplus::Ok)throw std::runtime_error("GIF 影格解碼失敗");
  Gdiplus::Bitmap bitmap(result.width,result.height,PixelFormat32bppPARGB);
  {Gdiplus::Graphics graphics(&bitmap);graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);graphics.Clear(Gdiplus::Color(0,0,0,0));graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);if(graphics.DrawImage(image.get(),0,0,result.width,result.height)!=Gdiplus::Ok)throw std::runtime_error("素材繪製失敗");}
  Gdiplus::Rect rect(0,0,result.width,result.height);Gdiplus::BitmapData data{};
  if(bitmap.LockBits(&rect,Gdiplus::ImageLockModeRead,PixelFormat32bppPARGB,&data)!=Gdiplus::Ok)throw std::runtime_error("素材像素讀取失敗");
  ImageFrame frame;try{frame.pixels.resize(size_t(result.width)*result.height);for(int y=0;y<result.height;++y)memcpy(frame.pixels.data()+size_t(y)*result.width,static_cast<unsigned char*>(data.Scan0)+ptrdiff_t(y)*data.Stride,size_t(result.width)*4);}catch(...){bitmap.UnlockBits(&data);throw;}
  bitmap.UnlockBits(&data);frame.delay=delays?gifDelay(static_cast<UINT*>(delays->value)[index]):100;result.frames.push_back(std::move(frame));
 }
 return trimClip(std::move(result));
}
bool CustomPet::load(const Json&files,std::string*failure){
 for(size_t i=0;i<clips.size();++i){clips[i]=ImageClip{};auto name=files[customStates[i]].str();if(!customFileName(name))continue;try{auto file=customDirectory()/wide(name);rejectReparse(file);clips[i]=decodeCustom(file);}catch(const std::exception&e){if(failure)*failure=e.what();}}
 active=-1;return ready();
}
}
