#include "travel_data.h"
#include <string.h>
static uint16_t u16(const uint8_t *p){return (uint16_t)p[0] | (uint16_t)p[1]<<8;}
static uint32_t u32(const uint8_t *p){return (uint32_t)u16(p) | (uint32_t)u16(p+2)<<16;}
void travel_data_reset(travel_data_t *s){memset(s,0,sizeof *s);}
bool travel_data_accept(travel_data_t *s,const uint8_t *p,size_t len,uint32_t now){
    if(!p || len!=TRAVEL_FRAME_SIZE || p[0]!=1 || (p[1]&~23u) || p[16] || p[17] || p[18] || p[19])return false;
    travel_data_t next={.present=true,.speed_valid=(p[1]&1)!=0,.heading_valid=(p[1]&2)!=0,
        .accuracy_valid=(p[1]&4)!=0,.demo=(p[1]&16)!=0,.sequence=u16(p+2),.session=u32(p+4),
        .speed_cms=u16(p+8),.heading_cdeg=u16(p+10),.accuracy_dm=u16(p+12),.age_ms=u16(p+14),.received_ms=now};
    if(!next.session || next.age_ms>TRAVEL_MAX_AGE_MS ||
       (next.speed_valid ? next.speed_cms>10000 : next.speed_cms!=UINT16_MAX) ||
       (next.heading_valid ? next.heading_cdeg>=36000 : next.heading_cdeg!=UINT16_MAX) ||
       (next.accuracy_valid ? next.accuracy_dm==UINT16_MAX : next.accuracy_dm!=UINT16_MAX))return false;
    uint16_t delta=(uint16_t)(next.sequence-s->sequence);
    if(s->present && (next.session!=s->session || delta==0 || delta>=32768))return false;
    *s=next;return true;
}
travel_status_t travel_data_status(const travel_data_t *s,uint32_t now){
    if(!s->present)return TRAVEL_WAITING;
    uint32_t elapsed=now-s->received_ms;
    return elapsed>TRAVEL_MAX_AGE_MS || s->age_ms>TRAVEL_MAX_AGE_MS-elapsed ? TRAVEL_STALE : TRAVEL_LIVE;
}
bool travel_data_heading_valid(const travel_data_t *s){return s->heading_valid && s->speed_valid && s->speed_cms>=150;}
