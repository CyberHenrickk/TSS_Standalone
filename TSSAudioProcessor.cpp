#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
struct Audio { unsigned channels=0, sampleRate=48000; std::vector<float> samples; };
static uint32_t u32(const uint8_t* p){return p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24);} 
static uint16_t u16(const uint8_t* p){return p[0]|(p[1]<<8);} 

static bool readWav(const fs::path& path, Audio& a, std::string& err) {
  std::ifstream f(path, std::ios::binary); if(!f){err="cannot open WAV";return false;}
  std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), {});
  if(b.size()<44 || std::string((char*)b.data(),4)!="RIFF" || std::string((char*)b.data()+8,4)!="WAVE"){err="invalid WAV";return false;}
  uint16_t fmt=0,bits=0; uint32_t rate=0; unsigned ch=0; size_t data=0, dataSize=0;
  size_t p=12; while(p+8<=b.size()){ uint32_t n=u32(b.data()+p+4); size_t q=p+8; if(q+n>b.size()) break;
    std::string id((char*)b.data()+p,4); if(id=="fmt "){ if(n<16){err="invalid fmt";return false;} fmt=u16(b.data()+q); ch=u16(b.data()+q+2); rate=u32(b.data()+q+4); bits=u16(b.data()+q+14); }
    if(id=="data"){data=q;dataSize=n;break;} p=q+n+(n&1); }
  if(!data||!ch||!rate||!(fmt==1||fmt==3)||!(bits==16||bits==24||bits==32)){err="WAV must be PCM/float 16, 24 or 32-bit";return false;}
  size_t bytes=bits/8, frames=dataSize/(bytes*ch); a.channels=ch;a.sampleRate=rate;a.samples.resize(frames*ch);
  for(size_t i=0;i<a.samples.size();++i){const uint8_t* x=b.data()+data+i*bytes; if(fmt==3&&bits==32){float v;memcpy(&v,x,4);a.samples[i]=v;} else if(bits==16){a.samples[i]=(int16_t)u16(x)/32768.f;} else if(bits==24){int32_t v=(x[0]|(x[1]<<8)|(x[2]<<16));if(v&0x800000)v|=0xff000000;a.samples[i]=v/8388608.f;} else {int32_t v=(int32_t)u32(x);a.samples[i]=v/2147483648.f;}}
  return true;
}
static void process(Audio& a){
  // Slightly stronger peak control than the previous test build, while
  // retaining enough headroom to avoid audible pumping or harsh limiting.
  const float threshold=0.56f, ratio=3.6f, attack=0.005f, release=0.12f, ceiling=0.88f;
  float env=0, gain=1; const float sr=(float)a.sampleRate;
  const float ac=std::exp(-1.f/(attack*sr)), rc=std::exp(-1.f/(release*sr));
  for(size_t i=0;i<a.samples.size();i+=a.channels){float peak=0;for(unsigned c=0;c<a.channels;c++)peak=std::max(peak,std::fabs(a.samples[i+c]));env=peak>env?ac*env+(1-ac)*peak:rc*env+(1-rc)*peak;
    float target=env>threshold?std::pow(threshold/env,ratio-1):1.f; gain=target<gain?ac*gain+(1-ac)*target:rc*gain+(1-rc)*target; for(unsigned c=0;c<a.channels;c++)a.samples[i+c]=std::clamp(a.samples[i+c]*gain, -ceiling, ceiling); }
  float peak=0;for(float v:a.samples)peak=std::max(peak,std::fabs(v)); if(peak>ceiling)for(float&v:a.samples)v*=ceiling/peak;
}
static bool writeWav(const fs::path& path,const Audio& a){std::ofstream f(path,std::ios::binary);if(!f)return false;uint32_t data=(uint32_t)(a.samples.size()*2), riff=36+data;uint16_t fmt=1,ch=(uint16_t)a.channels,bits=16;uint32_t rate=a.sampleRate,br=rate*ch*2;f.write("RIFF",4);f.write((char*)&riff,4);f.write("WAVEfmt ",8);uint32_t fsiz=16;f.write((char*)&fsiz,4);f.write((char*)&fmt,2);f.write((char*)&ch,2);f.write((char*)&rate,4);f.write((char*)&br,4);uint16_t ba=ch*2;f.write((char*)&ba,2);f.write((char*)&bits,2);f.write("data",4);f.write((char*)&data,4);for(float v:a.samples){int16_t s=(int16_t)std::lrint(std::clamp(v,-1.f,1.f)*32767.f);f.write((char*)&s,2);}return true;}
int main(int argc,char**argv){fs::path root=argc>1?fs::path(argv[1]):fs::current_path();fs::path in=root/"#music",out=root/"audioxl_output"/"sounds"/"TotentanzSoundSystem"/"music";fs::create_directories(out);if(!fs::exists(in)){std::cerr<<"ERROR: #music folder not found\n";return 1;}int count=0;for(auto&e:fs::directory_iterator(in)){if(!e.is_regular_file())continue;auto ext=e.path().extension().string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);if(ext!=".wav")continue;Audio a;std::string err;if(!readWav(e.path(),a,err)){std::cerr<<"ERROR: "<<e.path().filename().string()<<": "<<err<<"\n";return 1;}process(a);fs::path dst=out/e.path().filename();if(!writeWav(dst,a)){std::cerr<<"ERROR: cannot write "<<dst.string()<<"\n";return 1;}std::cout<<"Processed: "<<e.path().filename().string()<<" -> "<<dst.filename().string()<<"\n";count++;}if(!count){std::cerr<<"ERROR: no WAV files found in #music (other formats are ignored)\n";return 1;}std::cout<<"Processed "<<count<<" file(s).\n";return 0;}
