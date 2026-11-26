#pragma once
#include "core.h"
namespace pet {
inline int hour24(int hour,bool pm){if(hour<1||hour>12)throw std::runtime_error(tr(TextId::InvalidTime));return hour%12+(pm?12:0);}
inline int hour12(int hour){if(hour<0||hour>23)throw std::runtime_error(tr(TextId::InvalidTime));return hour%12?hour%12:12;}
inline int wrapNumber(int value,int delta,int low,int high){if(low>high)throw std::runtime_error(tr(TextId::InvalidTime));long long size=static_cast<long long>(high)-low+1;auto n=(static_cast<long long>(value)-low+delta)%size;if(n<0)n+=size;return int(n+low);}
inline int durationInput(Mode mode,int hours,int minutes,int seconds){if(mode==Mode::Alarm||hours<0||hours>(mode==Mode::Interval?24:168)||minutes<0||minutes>59||seconds<0||seconds>59)throw std::runtime_error(tr(TextId::InvalidTime));int total=hours*3600+minutes*60+seconds;if(total<1||total>Timer::maxInterval(mode))throw std::runtime_error(tr(TextId::InvalidTime));return total;}
// Monday-first checkboxes map to the existing Sunday=bit0 storage, no schedule migration.
inline int weekdayBit(int mondayFirst){return mondayFirst>=0&&mondayFirst<7?1<<((mondayFirst+1)%7):0;}
inline constexpr int weekdayPreset(int preset){return preset==1?127:preset==2?62:preset==3?65:0;}
inline Seconds displayedRemaining(const Timer&t,Seconds time){if(t.pending||!t.running)return 0;if(t.paused)return t.snooze?t.snooze:t.remaining;return std::max<Seconds>(0,(t.snooze?t.snooze:t.due)-time);}
inline std::string clockText(Seconds seconds){seconds=std::max<Seconds>(0,seconds);auto two=[](Seconds n){return (n<10?"0":"")+std::to_string(n);};return two(seconds/3600)+":"+two(seconds/60%60)+":"+two(seconds%60);}
}
